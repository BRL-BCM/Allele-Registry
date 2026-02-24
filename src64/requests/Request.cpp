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

sharedMemoryConsumer * Request::smConsumer = nullptr;
ReferencesDatabase const * Request::referencesDb = nullptr;
AllelesDatabase * Request::allelesDb = nullptr;
Configuration Request::configuration;

std::mutex Request::requestLog_access;
std::ofstream Request::requestLog;


void Request::initGlobalVariables(Configuration const & conf)
{
	configuration = conf;
	smConsumer = new sharedMemoryConsumer();
	referencesDb = new ReferencesDatabase(conf);
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
			}
			{
				if (requestTerminated) {
					if (isJson) this->chunksToSend.back().append("]");
					this->chunksToSend_finished = true;
					throw ExceptionRequestTerminated();
					//return;
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
					//return;
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
			this->cleanup();
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
	{
		std::lock_guard<std::mutex> synchScope(this->chunksToSend_access);
		if (this->processingThread != NULL) {
			if (this->processingThread->joinable() || this->chunksToSend_finished) throw std::logic_error("The request is being processed!");
		}
		outputBuilder.setFormat(respFormat, respFields);
	}
	auto newThread = new std::thread(&Request::internalThread, this);
	{
		std::lock_guard<std::mutex> synchScope(this->chunksToSend_access);
		this->processingThread = newThread;
	}
}


void Request::stopProcessingThread() noexcept
{
	try {
		{
			bool safe = false;
			{
				std::lock_guard<std::mutex> synchScope(this->chunksToSend_access);
				safe = this->processingThread != NULL && this->processingThread->joinable();
			}
			if(safe) {
				{
					std::lock_guard<std::mutex> synchScope(this->chunksToSend_access);
					killThread.store(true);
				}
				this->processingThread->join();
				{
					std::lock_guard<std::mutex> synchScope(this->chunksToSend_access);
					delete this->processingThread;
					this->processingThread = NULL;
				}
			}
		}
		{
			std::lock_guard<std::mutex> synchScope(this->chunksToSend_access);
			if (this->bytesCount > 0) {
				std::lock_guard<std::mutex> synchScope2(Request::requestLog_access);
				Request::requestLog << this->logTime << "\t" << this->documentsCount << "\t" << this->bytesCount << "\t";
				Request::requestLog << this->logLogin << "\t" << this->logMethod << "\t" << this->logRequest << std::endl;
			}
		}
	} catch (std::exception & ex) {
		std::cout << "stopProcessingThread exception: " << ex.what() << std::endl;
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
void Request::mapVariantsToMainGenome(std::vector<Document> & docs, alignment pAlignment) const
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
					// NOTE: leftAligned is important here!!!
					NormalizedGenomicVariant temp = canonicalizeGenomic(referencesDb, mappedVariant.align(alignment::leftAligned, referencesDb));
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

std::string getClingenPreferredTitle(ReferencesDatabase const * refDb, std::string name, std::string hgvsDefs, std::string proteinHgvsDef, unsigned geneId, DocumentActiveGenomicVariant & d) {
	// output string
	std::string clingenPreferredTitle = "";
	// This is an array of all MANE transcripts of which there are typically only two (one RefSeq and one EBI) BUT we can have more than one pair in a rare case.
	// The data here is all aggregated from each variants CAR document so it shouldn't be hard to have that at hand
	// transcript is an object that should contain title, proteinEffects, maneType, molecularConsequences, system, and maneVersion. They mostly come from CAR
	if(refDb->getAllTranscriptsByNameExists(name)) {
		auto iter = refDb->getAllTranscriptsByName(name);
		std::stringstream ss;
		ss << iter->second.second.refSeqNucleotideAccession;
		if(geneId != 0) {
			std::string geneSymbol = refDb->getGeneById(geneId).hgncSymbol;
			ss << "(" << geneSymbol << ")";
		}
		ss << ":";
		ss << hgvsDefs;
		if(!proteinHgvsDef.empty() && proteinHgvsDef.find(":n.") == std::string::npos) {
			ss << " (" << proteinHgvsDef.substr(proteinHgvsDef.find(':')+1) << ")";
		}
		clingenPreferredTitle = ss.str();
	}
	// This means our first option, relying on the maneTranscripts for the title didn't work (more than likely there weren't any MANE transcripts to use)
	if (clingenPreferredTitle.empty()) {
		// So we must instead look at the respective ClinVar esummary document for option 2!
		for(auto v : d.identifiers.getShortIds(identifierType::ClinVarAllele)) {
			Identifier_ClinVarAllele const & id = v.as_ClinVarAllele();
			if (! id.preferredName.empty()) clingenPreferredTitle = id.preferredName;
		}
		// Now we check to see if option 2 we just saw populated clingenPreferredTitle.
		if (clingenPreferredTitle.empty()) { // If not then we have to do option 3 now.
			// BUT WAIT, option 3 heavily relies on option 1! If we don't have a MANE transcript we can't find the canonical refSeq HGVS! So without us properly knowing what IS the canonical HGVS
			// via CAR, we can't really find clingenPreferredTitle from option 3. So this is something that needs to be discussed further.
			// So now lets check one more time to see if clingenPreferredTitle is empty.
			if (clingenPreferredTitle.empty()) { // if it is empty we have our last option (4) which relies on the respective CAR variant document
				//genomicAlleleObj = jsonPath.pick('genomicAlleles[*]', carDocument).referenceGenome === 'GRCh38' // Here we got the genomicAllele object which has a reference genome of 'GRCh38'
				//clingenPreferredTitle = genomicAlleleObj.hgvs[0] // We want the HGVS that starts with "NC_" nothing else (which is always typically the first one but should be validated still).
			}
		}
	}
	return clingenPreferredTitle;
}

struct BackLinkInfo {
	unsigned docId;
	unsigned transId;
	Document backDoc;
};

void Request::fillVariantsDetails( std::vector<Document> & docs, alignment pAlignment ) const
{
	if (killThread.load()) {
		// abort fillVariantsDetails, since request was externally terminated
		return;
	}
	jsonBuilder::DocumentSettings extSourcesConf = outputBuilder.externalSourcesConfiguration();

	bool const genomeBuilds = outputBuilder.returnsGenomeBuildsVariants();
	bool const genesRegions = outputBuilder.returnsGenesRegionsVariants();
	bool const transcript   = outputBuilder.returnsTranscriptsVariants ();
	bool const proteins     = outputBuilder.returnsProteinsVariants    ();
	bool const externalSources = ! extSourcesConf.isEmpty();

	if (! (genomeBuilds || genesRegions || transcript || proteins || externalSources) ) return;

	std::map<ReferenceId,std::vector<DocumentActiveProteinVariant*>> proteinVariantsToAnalyze;
	Stopwatch stopwatch;
	std::vector<BackLinkInfo> links;

	if (docs.size() > 1000) std::cout << "  - map variants across references ... " << std::flush;
	for (unsigned docId = 0; docId < docs.size(); docId++) {
		auto & doc = docs[docId];
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
						/*
						@bugfix: Author: Ronak Patel
						The identity check is added to take care of incorrect HGVS generation when the sequences don't match
						For example, if there is a A in position 40 of given sequence and altered to T the hgvs expression
						will be 40A>T, now if for that variant in a different refseq that aligns to this has T, the HGVS expression should be
						T40=, In the original implementation, this is not take care of. As a result, eroneous HGVS expressions were generated.
						As a fist fix with line below, the code will not produce the HGVS expression if the alignment is not identical
						TO DO: When it is not identical then generate appropriate HGVS.
						*/
						if (! ga.isIdentical() ) continue;
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
						PlainVariant plain = var.align(fAlignment, referencesDb);
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
							std::vector<std::string> names = referencesDb->getNames(v.definition.refId);
							for(auto & n: names) {
								if(referencesDb->getAllTranscriptsByNameExists(n)) {
									try {
										auto iter = referencesDb->getAllTranscriptsByName(n);
										if(v.geneId == 0) {
											v.geneId = iter->second.second.hgncId;
											break;
										}
									} catch ( std::exception const & e ) {
										std::cerr << e.what() << std::endl;
									}
								}
							}
							v.hgvsDefs = toHgvsModifications(referencesDb,var,fAlignment);
							calculateProteinVariation(referencesDb, var, v.hgvsDefs, v.proteinHgvsDef, v.proteinHgvsCanonical, 1000);

							if(v.proteinHgvsDef.size() > 0) {
								for(auto & n: names) {
									if(referencesDb->getAllTranscriptsByNameExists(n)) {
										try {
											auto iter = referencesDb->getAllTranscriptsByName(n);
											std::stringstream ss;
											v.maneStatus = iter->second.second.maneStatus;
											v.maneVersion = referencesDb->getManeVersion();
											std::string proteinDef = v.proteinHgvsDef;
											proteinDef = (v.proteinHgvsCanonical!=""&&v.proteinHgvsCanonical.size()<=proteinDef.size())?v.proteinHgvsCanonical:proteinDef;
											std::string clingenPreferredTitle = getClingenPreferredTitle(referencesDb, n, v.hgvsDefs, proteinDef, v.geneId, d);
											if(v.maneStatus.compare("MANE Select") == 0) {
												auto it = d.clingenPreferredTitle.begin();
												std::string toerase = "";
												while(it != d.clingenPreferredTitle.end()) {
													if((*it).substr(0,2) == "NC" || (*it).substr(0,2) == "NT" || (*it).substr(0,2) == "NW") {
														toerase = *it;
														break;
													}
													++it;
												}
												if(!toerase.empty()) {
													d.clingenPreferredTitle.erase(toerase);
												}
												d.clingenPreferredTitle.insert(clingenPreferredTitle);
											}
											v.maneRefSeqProtein = iter->second.second.refSeqProteinAccession + ":" + v.proteinHgvsDef.substr(v.proteinHgvsDef.find(':')+1);
											v.maneRefSeqNucleotide = iter->second.second.refSeqNucleotideAccession + ":" + v.hgvsDefs;
											v.maneEnsemblProtein = iter->second.second.ensemblProteinAccession + ":" + v.proteinHgvsDef.substr(v.proteinHgvsDef.find(':')+1);
											v.maneEnsemblNucleotide = iter->second.second.ensemblNucleotideAccession + ":" + v.hgvsDefs;
										} catch ( std::exception const & e ) {
											std::cerr << e.what() << std::endl;
										}
									}
								}

								try {
									HgvsVariant hgvsVar(true);
									decodeHgvs(referencesDb, v.proteinHgvsDef, hgvsVar);
									if(hgvsVar.isGenomic() == false) {
										DocumentActiveProteinVariant pd(canonicalize(referencesDb, hgvsVar.protein));
										BackLinkInfo info;
										info.docId = docId;
										info.transId = d.definitionsOnTranscripts.size();
										info.backDoc = pd;
										links.push_back(info);
									}
								} catch (...) {}

							}
							d.definitionsOnTranscripts.push_back(v);
						} else if (referencesDb->getMetadata(plain.refId).genomeBuild == "") {
							if (! genesRegions) continue;
							VariantDetailsGeneRegion v;
							v.definition = plain;
							v.geneId = referencesDb->getMetadata(v.definition.refId).geneId;
							v.hgvsDefs = toHgvsModifications(referencesDb,var,fAlignment);
							d.definitionsOnGenesRegions.push_back(v);
						} else {
							if (! genomeBuilds) continue;
							VariantDetailsGenomeBuild v;
							v.definition = plain;
							v.build = parseReferenceGenome(referencesDb->getMetadata(v.definition.refId).genomeBuild);
							v.chromosome = referencesDb->getMetadata(v.definition.refId).chromosome;
							v.hgvsDefs = toHgvsModifications(referencesDb,var,fAlignment);
							if(d.clingenPreferredTitle.empty()) {
								std::vector<std::string> names = OutputFormatter::refDb->getNames(v.definition.refId);
								std::string seq = "";
								for (auto & n: names) if (n.substr(0,2) == "NC" || n.substr(0,2) == "NT" || n.substr(0,2) == "NW") {
									seq = n;
									break;
								}
								if(referencesDb->getMetadata(plain.refId).genomeBuild.compare("GRCh38") == 0) d.clingenPreferredTitle.insert(seq + ":" + v.hgvsDefs);
							}
							d.definitionsOnGenomeBuilds.push_back(v);
						}
					} catch (ExceptionCoordinateOutsideReference const & e) {
						// ignore - it means that region reach bp outside the reference, so we cannot map this allele to this reference
					}
				}

			} else if (proteins && doc.isActiveProteinVariant()) {
				//throw ExceptionDisabledError("Protein requests are disabled for performance reasons at this time.");
				DocumentActiveProteinVariant & d = doc.asActiveProteinVariant();
				VariantDetailsProtein v;
				v.definition = d.mainDefinition.rightAligned();
				v.geneId = referencesDb->getMetadata(v.definition.refId).geneId;
				v.hgvsDefs = toHgvsModifications(referencesDb,d.mainDefinition,fAlignment);
				d.definitionOnProtein = v;
				proteinVariantsToAnalyze[v.definition.refId].push_back(&d);
			}
		} catch (...) {
			doc = DocumentError::createFromCurrentException();
		}
	}


	//struct BackLinkInfo {
	//	unsigned docId;
	//	unsigned transId;
	//	Document backDoc;
	//};

	if(links.size()>0) {
		std::vector<Document> ldoc;
		ldoc.reserve(links.size());
		for(auto & x : links) {
			ldoc.push_back(x.backDoc);
		}
		allelesDb->fetchVariantsByDefinition(ldoc);
		for(unsigned i=0; i<links.size(); i++) {
			try {
				if(ldoc[i].isActiveProteinVariant()) {
					CanonicalId id = ldoc[i].asActiveProteinVariant().caId;
					if(id.value < std::numeric_limits<uint64_t>::max()) {
						unsigned docId = links[i].docId;
						if(docId < docs.size()) {
							if(docs[docId].isActiveGenomicVariant()) {
								auto & d = docs[docId].asActiveGenomicVariant();
								if(links[i].transId < d.definitionsOnTranscripts.size()) {
									d.definitionsOnTranscripts[links[i].transId].caId = id;
								}
							}
						}
					}
				}
			} catch (...) {}
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
			unsigned const marginToCheck = 10;
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
			std::vector<std::pair<unsigned, MatchingTranscriptInfo> > transcriptVariants;
			std::mutex m_;
			auto m__ = &m_;
			std::vector<std::thread*> threadVec(1);
			std::vector<std::vector<DocumentActiveGenomicVariant> > params(1);
			for (int i_=0; i_<variants.size(); i_++) {
				params[i_%params.size()].push_back(variants[i_]);
			}

			for (int i_=0; i_<threadVec.size(); i_++) {
				threadVec[i_] = new std::thread([this, m__, transcriptId](std::vector<DocumentActiveGenomicVariant> const & v, std::vector<DocumentActiveProteinVariant*> const & pp, std::vector<std::pair<unsigned, MatchingTranscriptInfo> > & t) -> void{
					for(auto & d : v) {
						std::map< ReferenceId, std::vector<NormalizedSequenceModification> > allAlignments;
						allAlignments[d.mainDefinition.refId] = d.mainDefinition.modifications;
						for (NormalizedSequenceModification const & gmod: d.mainDefinition.modifications) {
							RegionCoordinates tempRegion = gmod.region;
							bool const awfulWorkaround = (tempRegion.length() == 0);
							if (awfulWorkaround) tempRegion.incRightPosition(1);
							// --------------------------------------
							std::vector<GeneralSeqAlignment> const alignments = referencesDb->getAlignmentsToMainGenome(d.mainDefinition.refId, tempRegion);
							for (GeneralSeqAlignment const& ga: alignments) {
								if (! ga.isPerfectMatch()) continue;
								if (! ga.isIdentical() ) continue;
								Alignment const a = ga.toAlignment();
								if (a.sourceRefId != transcriptId) continue;
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

						for (auto const & al: allAlignments) {
							try {
								NormalizedGenomicVariant var;
								var.refId = al.first;
								var.modifications = al.second;
								PlainVariant plain = var.align(fAlignment, referencesDb);
								if (referencesDb->isSplicedRefSeq(plain.refId)) {
									VariantDetailsTranscript u;
									u.hgvsDefs = toHgvsModifications(referencesDb,var,fAlignment);

									calculateProteinVariation(referencesDb, var, u.hgvsDefs, u.proteinHgvsDef, u.proteinHgvsCanonical, 50);
									for(int i=0;i<pp.size();i++) {
										std::string toMatch = pp[i]->definitionOnProtein.hgvsDefs;
										if(u.proteinHgvsCanonical.find(toMatch) != std::string::npos) {
											MatchingTranscriptInfo info;
											std::stringstream ss;
											ss << referencesDb->getNames(transcriptId).front() << ":" << u.hgvsDefs;
											info.transcriptHgvs = ss.str();
											info.proteinHgvs = u.proteinHgvsCanonical;
											info.caId = d.caId;
											m__->lock();
											t.push_back(std::make_pair(i,info));
											m__->unlock();
										}
									}

								}
							} catch ( ... ) {}
						}

					}
				}, std::ref(params[i_]), std::ref(kv.second), std::ref(transcriptVariants));
			}
			for(int i_=0; i_<threadVec.size(); i_++) {
				threadVec[i_]->join();
				delete threadVec[i_];
			}

			auto comparator = [](auto const & d1, auto const & d2)->bool
														{ return (d1.second < d2.second); };
			std::sort( transcriptVariants.begin(), transcriptVariants.end(), comparator );


			for(int i=0;i<kv.second.size();i++) {
				for(auto pv : transcriptVariants) {
					if(pv.first == i) {
						kv.second[i]->definitionOnProtein.hgvsMatchingTranscriptVariants.push_back(pv.second);
					}
				}
			}

/*
			// ---- match genomic variants
			// !!! NASTY WORKAROUND - we skip calculations when number of transcript variants exceeds 2K !!! - TODO
			//if (transcriptVariants.size() > 2000) continue;
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
			solution = searchForMatchingTranscriptVariants(transcriptId, transcriptStartCodon, transcriptSequence, transcriptVariants, proteinSequence, proteinVariants, solutionSizeLimit);
//xxx = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
//std::cout << "END="	<<	std::ctime(&xxx) << std::endl;
			// ---- save solution
			for ( unsigned i = 0;  i < kv.second.size();  ++i ) {
				for (auto pv: solution[i]) {
					try {
						for (auto & x: pv.modifications) x.region = referencesDb->convertToUnsplicedRegion(pv.refId, x.region);
						NormalizedGenomicVariant nv = canonicalizeGenomic(referencesDb, pv);
						std::string hgvs = toHgvsModifications(referencesDb, nv, fAlignment);
						kv.second[i]->definitionOnProtein.hgvsMatchingTranscriptVariants.push_back(referencesDb->getNames(transcriptId).front() + ":" + hgvs);
					} catch (std::exception const & e) {
						//std::cerr << e.what() << std::endl;
						// TODO
					}
				}
			}
*/

		} catch (std::exception const & e) {
			//std::cerr << e.what() << std::endl;
			// TODO
		}
	}


	// ========================== calculate external sources
	if (docs.size() > 1000) std::cout << stopwatch.save_and_restart_sec() << " sec\n" << "  - query external sources ... " << std::flush;
	if ( externalSources ) {
		std::vector<uint64_t> ids;
		ids.reserve(docs.size());
		for ( auto const & doc: docs ) {
			if (doc.isActiveGenomicVariant()) {
				if (!doc.asActiveGenomicVariant().caId.isNull()) {
					ids.push_back(doc.asActiveGenomicVariant().caId.value);
				}
			} else if (doc.isActiveProteinVariant()) {
				if (!doc.asActiveProteinVariant().caId.isNull()) {
					ids.push_back(doc.asActiveProteinVariant().caId.value);
				}
			}
		}
		std::vector<std::string> jsonOutput;
		externalSources::queryLinksByIds( jsonOutput, extSourcesConf, outputBuilder.prettyJson(), ids );
		unsigned iCurrentOutput = 0;
		for ( auto & doc: docs ) {
			if (doc.isActiveGenomicVariant()) {
				if (!doc.asActiveGenomicVariant().caId.isNull()) {
					doc.asActiveGenomicVariant().jsonExternalSources.swap(jsonOutput[iCurrentOutput++]);
				}
			} else if (doc.isActiveProteinVariant()) {
				if (!doc.asActiveProteinVariant().caId.isNull()) {
					doc.asActiveProteinVariant().jsonExternalSources.swap(jsonOutput[iCurrentOutput++]);
				}
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

