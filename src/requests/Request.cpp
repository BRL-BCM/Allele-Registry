#include "requests.hpp"
#include "hgvs.hpp"
#include "canonicalization.hpp"
#include "proteins.hpp"
#include "proteinVariantsToGenomic.hpp"
#include "../externalSources/externalSources.hpp"
#include <chrono>
#include <ctime>
#include <fstream>
#include "../commonTools/Stopwatch.hpp"


ResultSubset const ResultSubset::all = ResultSubset();

ReferencesDatabase const * Request::referencesDb = nullptr;
AllelesDatabase * Request::allelesDb = nullptr;
Configuration Request::configuration;

std::mutex Request::requestLog_access;
std::ofstream Request::requestLog;


void Request::initGlobalVariables(Configuration const & conf)
{
	configuration = conf;
	referencesDb = new ReferencesDatabase(conf.referencesDatabase_path);
	allelesDb = new AllelesDatabase(referencesDb, configuration);
	{ // ========= file to append logs
		std::string p = conf.allelesDatabase_path;
		if (p.back() != '/') p.push_back('/');
		p += "logRequests.txt";
		Request::requestLog.open(p, std::ios::app);
		if (! Request::requestLog.good()) throw std::runtime_error("Cannot open log file: " + p);
	} // ================================
	OutputFormatter::refDb = referencesDb;
	OutputFormatter::carURI = conf.alleleRegistryFQDN;
	externalSources::init(conf.externalSources_db);
}


errorType Request::lastErrorType()
{
	std::lock_guard<std::mutex> synchScope(this->chunksToSend_access);
	return error;
}


void Request::addChunkOfResponse(std::vector<Document> const & documents)
{
	bool const isJson = outputBuilder.prettyOrCompressedJson();
	unsigned iDoc = 0;
	unsigned docCount = 0;
	std::string chunk = "";
	bool requestTerminated = false;
	while (true) {
		if (killThread.load() && ! requestTerminated) {
			requestTerminated = true;
			if (isJson) chunk.append(",");
			chunk.append( outputBuilder.createOutput(DocumentError(errorType::RequestTerminated)) + "\n" );
		}
		if (chunk == "") {
			// create a new chunk
			docCount = 0;
			for ( ;  iDoc < documents.size() && chunk.size() < 16*1024*1024;  ++iDoc ) {  // TODO - hardcoded - size of output chunk
				if (isJson) chunk.append(",");
				chunk.append( outputBuilder.createOutput(documents[iDoc]) + "\n" );
				++docCount;
			}
			if (chunk == "") break; // no more data
		} else {
			// wait
			std::this_thread::yield();
			std::this_thread::sleep_for( std::chrono::milliseconds(10) );
			std::this_thread::yield();
		}
		{ // try to add the chunk to queue with stuff to send
			std::lock_guard<std::mutex> synchScope(this->chunksToSend_access);
			if (this->chunksToSend_finished) return; // there is nothing more to do
			if (! chunksToSend_started) {
				chunksToSend_started = true;
				if (isJson) chunk[0] = '[';
			}
			if (this->chunksToSend.size() < 8) { // TODO - set maximum number of chunks
				this->documentsCount += docCount;
				this->bytesCount += chunk.size();
				this->chunksToSend.push_back(std::move(chunk));
				chunk = "";
				if (requestTerminated) {
					if (isJson) this->chunksToSend.back().append("]");
					this->chunksToSend_finished = true;
					throw ExceptionRequestTerminated();
				}
			}
		}
	}
}


void Request::addChunkOfResponse(std::string && chunk)
{
	if (chunk == "") {
		std::lock_guard<std::mutex> synchScope(this->chunksToSend_access);
		this->chunksToSend_started = this->chunksToSend_finished = true;
		return;
	}
	bool requestTerminated = false;
	while (true) {
		if (killThread.load() && ! requestTerminated) {
			requestTerminated = true;
			chunk = "ERROR\t" + toString(errorType::RequestTerminated) + "\t" + description(errorType::RequestTerminated) + "\n";
		}
		if (chunk == "") return;
		{ // try to add the chunk to queue with stuff to send
			std::lock_guard<std::mutex> synchScope(this->chunksToSend_access);
			if (this->chunksToSend_finished) return; // there is nothing more to do
			if (! chunksToSend_started) {
				chunksToSend_started = true;
			}
			if (this->chunksToSend.size() < 8) { // TODO - set maximum number of chunks
				this->bytesCount += chunk.size();
				this->chunksToSend.push_back(std::move(chunk));
				chunk = "";
				if (requestTerminated) {
					this->chunksToSend_finished = true;
					throw ExceptionRequestTerminated();
				}
				return;
			}
		}
		// wait
		std::this_thread::yield();
		std::this_thread::sleep_for( std::chrono::milliseconds(10) );
		std::this_thread::yield();
	}
}


void Request::setResponse(Document const & doc)
{
	std::string chunk = outputBuilder.createOutput(doc);
	{
		std::lock_guard<std::mutex> synchScope(this->chunksToSend_access);
		if (chunksToSend_started) throw std::logic_error("Response was already prepared.");
		chunksToSend_started = chunksToSend_finished = true;
		this->bytesCount += chunk.size();
		++(this->documentsCount);
		this->chunksToSend.push_back(std::move(chunk));
		if (doc.isError()) error = doc.error().type;
	}
}


void Request::setResponse(std::string const & text)
{
	{
		std::lock_guard<std::mutex> synchScope(this->chunksToSend_access);
		if (chunksToSend_started) throw std::logic_error("Response was already prepared.");
		chunksToSend_started = chunksToSend_finished = true;
		this->chunksToSend.push_back(text);
	}
}


void Request::internalThread() noexcept
{
	try {
		bool const isJson = outputBuilder.prettyOrCompressedJson();
		Document doc;
		try {
			this->process();
			{
				std::lock_guard<std::mutex> synchScope(this->chunksToSend_access);
				if (! chunksToSend_started) {
					if (isJson) {
						chunksToSend.push_back("[]");
					}
				} else if (! chunksToSend_finished) {
					if (isJson) {
						if (chunksToSend.empty()) chunksToSend.push_back("]");
						else chunksToSend.back().push_back(']');
					}
				}
				chunksToSend_started = chunksToSend_finished = true;
			}
			return;  // success
		} catch (...) {
			doc = DocumentError::createFromCurrentException();
		}
		// try to finish response
		{
			std::lock_guard<std::mutex> synchScope(this->chunksToSend_access);
			if (chunksToSend_finished) return; // TODO - log it somewhere
			if (chunksToSend_started) {
				std::string row = isJson ? "," : "";
				row.append(outputBuilder.createOutput(doc) + "\n");
				if (isJson) row.append("]");
				chunksToSend.push_back(row);
				error = errorType::InternalServerError;
			} else {
				chunksToSend.push_back( outputBuilder.createOutput(doc) );
				error = doc.error().type;
			}
			chunksToSend_started = chunksToSend_finished = true;
		}
	} catch (...) {
		// total fail, just finish the response
		std::lock_guard<std::mutex> synchScope(this->chunksToSend_access);
		chunksToSend_started = chunksToSend_finished = true;
		error = errorType::InternalServerError;
	}
}


void Request::startProcessingThread(responseFormat respFormat, std::string const & respFields)
{
	if (this->processingThread.joinable() || this->chunksToSend_finished) throw std::logic_error("The request is being processed!");
	outputBuilder.setFormat(respFormat, respFields);
	this->processingThread = std::thread(&Request::internalThread, this);
}


void Request::stopProcessingThread() noexcept
{
	try {
		if (this->processingThread.joinable()) {
			killThread.store(true);
			processingThread.join();
		}
		{
			std::lock_guard<std::mutex> synchScope(this->chunksToSend_access);
			if (this->bytesCount > 0) {
				std::lock_guard<std::mutex> synchScope2(Request::requestLog_access);
				Request::requestLog << this->logTime << "\t" << this->documentsCount << "\t" << this->bytesCount << "\t";
				Request::requestLog << this->logLogin << "\t" << this->logMethod << "\t" << this->logRequest << std::endl;
			}
		}
	} catch (...) {
		// TODO - log it somewhere
	}
}

void checkNucleotideSequence(std::string const & s)
{
	for (auto c: s)
		if ( ! (c == 'A' || c == 'C' || c == 'G' || c == 'T') )
			throw std::runtime_error("Nucleotide sequence contains incorrect character: " + std::string(1,c)); // TODO
}
void checkAminoAcidSequence(std::string const & s)
{
	for (auto c: s) {
		switch (c) {
			case 'A':
			case 'C':
			case 'D':
			case 'E':
			case 'F':
			case 'G':
			case 'H':
			case 'I':
			case 'K':
			case 'L':
			case 'M':
			case 'N':
			case 'O':
			case 'P':
			case 'Q':
			case 'R':
			case 'S':
			case 'T':
			case 'U':
			case 'V':
			case 'W':
			case 'Y':
			case '*': break;
			default : throw std::runtime_error("Amino-acid sequence contains incorrect character: " + std::string(1,c)); // TODO
		}
	}
}

// map variant to the main genome
void Request::mapVariantsToMainGenome(std::vector<Document> & docs) const
{
	for (auto & doc: docs) {
		try {
			if (doc.isActiveGenomicVariant()) {
				DocumentActiveGenomicVariant & d = doc.asActiveGenomicVariant();
				ReferenceId const localRefId = d.mainDefinition.refId;
				std::string const localRefName = referencesDb->getNames(localRefId).front(); // TODO - choose the name somehow
				// map normalized form to the main genome
				NormalizedGenomicVariant mappedVariant;
				for (NormalizedSequenceModification const & localMod: d.mainDefinition.modifications) {
					GeneralSeqAlignment ga = referencesDb->getAlignmentFromMainGenome(localRefId, localMod.region);
					if ( ! ga.isPerfectMatch() ) throw ExceptionNoConsistentAlignment(localRefName, localMod.region);
					Alignment const a = ga.toAlignment();
					if ( ! a.isIdentical() ) throw ExceptionNoConsistentAlignment(localRefName, localMod.region); // TODO - define mapping tules
					if ( a.targetRegion() != localMod.region ) throw ExceptionNoConsistentAlignment(localRefName, localMod.region);
					if ( a.sourceRefId != mappedVariant.refId ) {
						if (mappedVariant.refId != ReferenceId::null) {
							throw ExceptionNoConsistentAlignment(localRefName, localMod.region, "Haplotype is aligned to more than one reference on the main genome.");
						}
						mappedVariant.refId = a.sourceRefId;
					}
					NormalizedSequenceModification mod = localMod;
					mod.region = a.sourceRegion();
					if (a.sourceStrandNegativ) {
						if (mod.category == variantCategory::duplication || mod.category == variantCategory::shiftableDeletion) {
							mod.originalSequence = rotateLeft(mod.originalSequence, mod.region.length() - mod.originalSequence.size());
						}
						convertToReverseComplementary(mod.insertedSequence);
						convertToReverseComplementary(mod.originalSequence);
					}
					mappedVariant.modifications.push_back(mod);
				}
				if (mappedVariant.refId == localRefId) continue;  // it is already on the main reference genome
				std::sort(mappedVariant.modifications.begin(), mappedVariant.modifications.end());
				// check if mapped changes are also canonical on main genome
				{
	//std::cout << "ZZZ " << mappedVariant.refId.value << " " << mappedVariant.modifications.front().region.toString() << " " << mappedVariant.modifications.front().originalSequence << std::endl;
					NormalizedGenomicVariant temp = canonicalizeGenomic(referencesDb, mappedVariant.leftAligned());
	//std::cout << "AAA " << temp.refId.value << " " << temp.modifications.front().region.toString() << " " << temp.modifications.front().originalSequence << std::endl;
					if (temp != mappedVariant) {
						if (temp.modifications.size() != mappedVariant.modifications.size()) {
							throw ExceptionNoConsistentAlignment(localRefName, "Different number of simple alleles after canonicalization");
						}
						for (unsigned i = 0; i < temp.modifications.size(); ++i) {
							if (temp.modifications[i] != mappedVariant.modifications[i]) {
								throw ExceptionNoConsistentAlignment(localRefName, mappedVariant.modifications[i].region, "Definition changes after canonicalization");
							}
						}
					}
				}
				d.mainDefinition = mappedVariant;
				// check, if the sequence is well defined
				for (auto const & m : d.mainDefinition.modifications) checkNucleotideSequence(m.insertedSequence);
			} else if (doc.isActiveProteinVariant()) {
				DocumentActiveProteinVariant const & d = doc.asActiveProteinVariant();
				// check, if the sequence is well defined
				for (auto const & m : d.mainDefinition.modifications) checkAminoAcidSequence(m.insertedSequence);
			}
		} catch (...) {
			doc = DocumentError::createFromCurrentException();
		}
	}
}

void print(std::ostream & out, PlainSequenceModification const & sm)
{
	out << sm.region.left() << "," << sm.region.right() << "," << sm.originalSequence << "," << sm.newSequence;
}

void Request::fillVariantsDetails( std::vector<Document> & docs) const
{
	jsonBuilder::DocumentSettings extSourcesConf = outputBuilder.externalSourcesConfiguration();

	bool const genomeBuilds = outputBuilder.returnsGenomeBuildsVariants();
	bool const genesRegions = outputBuilder.returnsGenesRegionsVariants();
	bool const transcript   = outputBuilder.returnsTranscriptsVariants ();
	bool const proteins     = outputBuilder.returnsProteinsVariants    ();
	bool const externalSources = ! extSourcesConf.isEmpty();

	if (! (genomeBuilds || genesRegions || transcript || proteins || externalSources) ) return;

	std::map<ReferenceId,std::vector<DocumentActiveProteinVariant*>> proteinVariantsToAnalyze;
	Stopwatch stopwatch;

	if (docs.size() > 1000) std::cout << "  - map variants across references ... " << std::flush;
	for (auto & doc: docs) {
		try {
			if ( (genomeBuilds || genesRegions || transcript) && doc.isActiveGenomicVariant() ) {
				DocumentActiveGenomicVariant & d = doc.asActiveGenomicVariant();
				// ============================== map from main genome to all possible references
				std::map< ReferenceId, std::vector<NormalizedSequenceModification> > allAlignments;
				allAlignments[d.mainDefinition.refId] = d.mainDefinition.modifications;
				for (NormalizedSequenceModification const & gmod: d.mainDefinition.modifications) {
					// TODO - get rid of that mess below
					// ------ AWFUL WORKAROUND FOR PURE INSERTIONS - the underlying alignments logic must be reviewed, especially for zero-length regions
					RegionCoordinates tempRegion = gmod.region;
					bool const awfulWorkaround = (tempRegion.length() == 0);
					if (awfulWorkaround) tempRegion.incRightPosition(1);
					// --------------------------------------
					std::vector<GeneralSeqAlignment> const alignments = referencesDb->getAlignmentsToMainGenome(d.mainDefinition.refId, tempRegion);
					for (GeneralSeqAlignment const& ga: alignments) {
						if (! ga.isPerfectMatch()) continue;
						Alignment const a = ga.toAlignment();
						NormalizedSequenceModification mod = gmod;
						mod.region = a.sourceRegion();
						if (awfulWorkaround) {
							if (a.sourceStrandNegativ) mod.region.incLeftPosition(1); else mod.region.decRightPosition(1);
						}
						if (a.sourceStrandNegativ) {
							convertToReverseComplementary(mod.originalSequence);
							convertToReverseComplementary(mod.insertedSequence);
						}
						allAlignments[a.sourceRefId].push_back(mod);
					}
				}
				// remove references, for which not all simple alleles were mapped
				{
					std::vector<ReferenceId> toDelete;
					for (auto & kv: allAlignments) {
						if (kv.second.size() < d.mainDefinition.modifications.size()) {
							toDelete.push_back(kv.first);
						} else {
							// TODO - check if canonicalized properly on target ref
							std::sort(kv.second.begin(), kv.second.end());
						}
					}
					for (ReferenceId & i: toDelete) allAlignments.erase(i);
				}
				// ========================= save to documents
				for (auto const & kv: allAlignments) {
					try {
						NormalizedGenomicVariant var;
						var.refId = kv.first;
						var.modifications = kv.second;
						PlainVariant plain = var.leftAligned();
						if (referencesDb->isSplicedRefSeq(plain.refId)) {
							if (! transcript) continue;
							VariantDetailsTranscript v;
							v.definition.refId = plain.refId;
							for (auto const & m: plain.modifications) {
								PlainSequenceModificationOnSplicedReference sp;
								sp.region = referencesDb->convertToSplicedRegion(plain.refId, m.region);
								sp.originalSequence = m.originalSequence;
								sp.newSequence = m.newSequence;
								v.definition.modifications.push_back(sp);
							}
							v.geneId = referencesDb->getMetadata(v.definition.refId).geneId;
							v.hgvsDefs = toHgvsModifications(referencesDb,var);
							calculateProteinVariation(referencesDb, var, v.proteinHgvsDef, v.proteinHgvsCanonical);
							d.definitionsOnTranscripts.push_back(v);
						} else if (referencesDb->getMetadata(plain.refId).genomeBuild == "") {
							if (! genesRegions) continue;
							VariantDetailsGeneRegion v;
							v.definition = plain;
							v.geneId = referencesDb->getMetadata(v.definition.refId).geneId;
							v.hgvsDefs = toHgvsModifications(referencesDb,var);
							d.definitionsOnGenesRegions.push_back(v);
						} else {
							if (! genomeBuilds) continue;
							VariantDetailsGenomeBuild v;
							v.definition = plain;
							v.build = parseReferenceGenome(referencesDb->getMetadata(v.definition.refId).genomeBuild);
							v.chromosome = referencesDb->getMetadata(v.definition.refId).chromosome;
							v.hgvsDefs = toHgvsModifications(referencesDb,var);
							d.definitionsOnGenomeBuilds.push_back(v);
						}
					} catch (ExceptionCoordinateOutsideReference const & e) {
						// ignore - it means that region reach bp outside the reference, so we cannot map this allele to this reference
					}
				}
			} else if (proteins && doc.isActiveProteinVariant()) {
				DocumentActiveProteinVariant & d = doc.asActiveProteinVariant();
				VariantDetailsProtein v;
				v.definition = d.mainDefinition.rightAligned();
				v.geneId = referencesDb->getMetadata(v.definition.refId).geneId;
				v.hgvsDefs = toHgvsModifications(referencesDb,d.mainDefinition);
				d.definitionOnProtein = v;
				proteinVariantsToAnalyze[v.definition.refId].push_back(&d);
			}
		} catch (...) {
			doc = DocumentError::createFromCurrentException();
		}
	}

	// =========================== calculate link from protein variants to genomic variants
	if (docs.size() > 1000) std::cout << stopwatch.save_and_restart_sec() << " sec\n" << "  - protein-related analysis ... " << std::flush;
	for (auto kv: proteinVariantsToAnalyze) {
		try {
			{ // ---- sort protein variant documents
				auto funcSortingProteinVariantDocuments = [](DocumentActiveProteinVariant const * d1, DocumentActiveProteinVariant const * d2)->bool
															{ return (d1->mainDefinition < d2->mainDefinition); };
				std::sort( kv.second.begin(), kv.second.end(), funcSortingProteinVariantDocuments );
			}
			// ---- get protein data
			unsigned const proteinLength = referencesDb->getSequenceLength(kv.first);
			std::string const proteinSequence = referencesDb->getSequence(kv.first, RegionCoordinates(0,proteinLength));
			std::vector<PlainVariant> proteinVariants;
			for (DocumentActiveProteinVariant * doc: kv.second) {
				proteinVariants.push_back( doc->mainDefinition.rightAligned() );
			}
			// ---- get transcript data
			ReferenceId const transcriptId = referencesDb->getTranscriptForProtein(kv.first);
			if (transcriptId == ReferenceId::null) continue;
			unsigned const transcriptStartCodon = referencesDb->getCDS(transcriptId).left();
			unsigned const transcriptLength = referencesDb->getMetadata(transcriptId).splicedLength;
			std::string const transcriptSequence = referencesDb->getSplicedSequence(transcriptId, RegionCoordinates(0,transcriptLength));
			// ---- convert region to main genome and query variants overlapping with transcript
			RegionCoordinates transcriptROI(transcriptStartCodon+3,transcriptLength);
			// we assume margin of 90bp before/after protein variants to check
			unsigned const marginToCheck = 80;
			unsigned leftBoundary  = kv.second.front()->definitionOnProtein.definition.modifications.front().region.left () * 3 + transcriptStartCodon;
			if (leftBoundary >= marginToCheck) {
				leftBoundary -= marginToCheck;
			} else {
				leftBoundary = 0;
			}
			unsigned const rightBoundary = kv.second.back ()->definitionOnProtein.definition.modifications.back ().region.right() * 3 + transcriptStartCodon + marginToCheck;
			if (transcriptROI.left()  < leftBoundary ) transcriptROI.setLeft (leftBoundary );
			if (transcriptROI.right() > rightBoundary) transcriptROI.setRight(rightBoundary);
			GeneralSeqAlignment const ga = referencesDb->getAlignmentFromMainGenome(transcriptId, referencesDb->convertToUnsplicedRegion(transcriptId,transcriptROI));
			std::vector<DocumentActiveGenomicVariant> variants;
			auto callback = [&variants](std::vector<Document> & docs, bool & lastCall)->void
					{
						for (Document & doc: docs) {
							if (doc.isActiveGenomicVariant()) variants.push_back(doc.asActiveGenomicVariant());
						}
					};
			for (auto const & e: ga.elements) {
				unsigned const chunkSize = 1024u*1024u;
				unsigned recordsToSkip = 0;
				allelesDb->queryVariants(callback, recordsToSkip, e.alignment.sourceRefId, e.alignment.sourceRegion().left(), e.alignment.sourceRegion().right(), chunkSize, 1000);
			}
			// ---- convert genome variants to transcript
			std::vector<PlainSequenceModification> transcriptVariants;
			for (auto & d: variants) {
				for (NormalizedSequenceModification const & gmod: d.mainDefinition.modifications) {
					// TODO - get rid of that mess below
					// ------ AWFUL WORKAROUND FOR PURE INSERTIONS - the underlying alignments logic must be reviewed, especially for zero-length regions
					RegionCoordinates tempRegion = gmod.region;
					bool const awfulWorkaround = (tempRegion.length() == 0);
					if (awfulWorkaround) tempRegion.incRightPosition(1);
					// --------------------------------------
					std::vector<GeneralSeqAlignment> const alignments = referencesDb->getAlignmentsToMainGenome(d.mainDefinition.refId, tempRegion);
					for (GeneralSeqAlignment const& ga: alignments) {
						if (! ga.isPerfectMatch()) continue;
						Alignment const a = ga.toAlignment();
						if (a.sourceRefId != transcriptId) continue;
						NormalizedSequenceModification mod = gmod;
						SplicedRegionCoordinates splicedRegion = referencesDb->convertToSplicedRegion(transcriptId, a.sourceRegion());
						if (splicedRegion.isIntronic()) continue;
						mod.region = splicedRegion.toRegion();
						if (awfulWorkaround) {
							if (a.sourceStrandNegativ) mod.region.incLeftPosition(1); else mod.region.decRightPosition(1);
						}
						if (a.sourceStrandNegativ) {
							convertToReverseComplementary(mod.originalSequence);
							convertToReverseComplementary(mod.insertedSequence);
						}
						transcriptVariants.push_back(mod.rightAligned());
					}
				}
			}
			// ---- match genomic variants
			// !!! NASTY WORKAROUND - we skip calculations when number of transcript variants exceeds 2K !!! - TODO
			if (transcriptVariants.size() > 2000) continue;
			std::vector< std::vector<PlainVariant> > solution;
//std::cout << "XXXXXX "  << transcriptVariants.size() << "\n" ;
//std::cout << transcriptSequence << "\n" << transcriptStartCodon;
//for (auto v: transcriptVariants) {
//	std::cout << "\t";
//	print(std::cout, v);
//}
//std::cout << "\n";
//print(std::cout, proteinVariants.front().modifications.front());
//for (unsigned i = 1; i < proteinVariants.size(); ++i) {
//	std::cout << "\t";
//	print(std::cout, proteinVariants[i].modifications.front());
//}
//std::cout << std::endl;
//auto xxx = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
//std::cout << "START="	<<	std::ctime(&xxx) << std::endl;
			solution = searchForMatchingTranscriptVariants(transcriptId, transcriptStartCodon, transcriptSequence, transcriptVariants, proteinSequence, proteinVariants);
//xxx = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
//std::cout << "END="	<<	std::ctime(&xxx) << std::endl;
			// ---- save solution
			for ( unsigned i = 0;  i < kv.second.size();  ++i ) {
				for (auto pv: solution[i]) {
					try {
						for (auto & x: pv.modifications) x.region = referencesDb->convertToUnsplicedRegion(pv.refId, x.region);
						NormalizedGenomicVariant nv = canonicalizeGenomic(referencesDb, pv);
						std::string hgvs = toHgvsModifications(referencesDb, nv);
						kv.second[i]->definitionOnProtein.hgvsMatchingTranscriptVariants.push_back(referencesDb->getNames(transcriptId).front() + ":" + hgvs);
					} catch (std::exception const & e) {
						std::cerr << e.what() << std::endl;
						// TODO
					}
				}
			}
		} catch (std::exception const & e) {
			std::cerr << e.what() << std::endl;
			// TODO
		}
	}

	// ========================== calculate external sources
	if (docs.size() > 1000) std::cout << stopwatch.save_and_restart_sec() << " sec\n" << "  - query external sources ... " << std::flush;
	if ( externalSources ) {
		std::vector<uint32_t> ids;
		ids.reserve(docs.size());
		for ( auto const & doc: docs ) {
			if (doc.isActiveGenomicVariant()) {
				ids.push_back(doc.asActiveGenomicVariant().caId.value);
			} else if (doc.isActiveProteinVariant()) {
				ids.push_back(doc.asActiveProteinVariant().caId.value);
			}
		}
		std::vector<std::string> jsonOutput;
		externalSources::queryLinksByIds( jsonOutput, extSourcesConf, outputBuilder.prettyJson(), ids );
		unsigned iCurrentOutput = 0;
		for ( auto & doc: docs ) {
			if (doc.isActiveGenomicVariant()) {
				doc.asActiveGenomicVariant().jsonExternalSources.swap(jsonOutput[iCurrentOutput++]);
			} else if (doc.isActiveProteinVariant()) {
				doc.asActiveProteinVariant().jsonExternalSources.swap(jsonOutput[iCurrentOutput++]);
			}
		}
	}
	if (docs.size() > 1000) std::cout << stopwatch.save_and_restart_sec() << " sec\n" << std::flush;
}

// returns: true - chunk returned, false - no more data
bool Request::nextChunkOfResponse(std::string & chunk)
{
	std::chrono::system_clock::time_point tsStartWaiting = std::chrono::system_clock::now();
	chunk = "";
	while (true) {
		{
			std::lock_guard<std::mutex> synchScope(this->chunksToSend_access);
			if (this->chunksToSend.empty()) {
				if (this->chunksToSend_finished) break;
				// continue to the next iteration if we can still wait, leave the function otherwise
				if ( std::chrono::system_clock::now() - tsStartWaiting > std::chrono::seconds(15) ) return true;
			} else {
				chunk = std::move(this->chunksToSend.front());
				this->chunksToSend.pop_front();
				return true;
			}
		}
		std::this_thread::yield();
		std::this_thread::sleep_for( std::chrono::milliseconds(10) );
		std::this_thread::yield();
	}
	return false;
}


std::string Request::getRequestToGUI() const
{
	throw ExceptionIncorrectRequest("Output of this request cannot be represented as HTML data");
}

