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

bool Request::allFinished() const {
	bool isFinished = true;
	for(int i=0;i<this->chunksToSend_finished.size();i++) {
		if(this->chunksToSend_finished[i] == false) {
			isFinished = false;
		}
	}
	return isFinished;
}

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
	std::lock_guard<std::mutex> synchScope(this->chunksToSend._mutex);
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
		}
		{ // try to add the chunk to queue with stuff to send
			this->chunksToSend._mutex.lock();
			bool isFinished = this->allFinished();
			this->chunksToSend._mutex.unlock();
			if(isFinished) return; // there is nothing more to do

			this->chunksToSend._mutex.lock();
			bool allNotStarted = true;
			for(int i=0;i<this->chunksToSend_started.size();i++) {
				if(this->chunksToSend_started[i] == true) {
					allNotStarted = false;
				}
			}
			if (allNotStarted) {
				for(int i=0;i<this->chunksToSend_started.size();i++) {
					this->chunksToSend_started[i] = true;
				}
				if (isJson) chunk[0] = '[';
			}
			this->chunksToSend._mutex.unlock();
		 
			this->chunksToSend._mutex.lock();
			this->documentsCount += docCount;
			this->bytesCount += chunk.size();
			this->chunksToSend._mutex.unlock();

			this->chunksToSend.push_back(std::move(chunk), [this](){return this->allFinished();} );

			chunk = "";
			if (requestTerminated) {
				if (isJson) {
					this->chunksToSend._mutex.lock();
					bool isEmpty = this->chunksToSend.empty();
					this->chunksToSend._mutex.unlock();
					if (isEmpty) {
						chunksToSend.push_back("]", [this](){return this->allFinished();} );
					} else {
						this->chunksToSend._mutex.lock();
						chunksToSend.back().push_back(']');
						this->chunksToSend._mutex.unlock();
					}
				}
				this->chunksToSend._mutex.lock();
				for(int i=0;i<this->chunksToSend_finished.size();i++) {
					this->chunksToSend_finished[i] = true;
				}
				this->chunksToSend.purge();
				this->chunksToSend._mutex.unlock();
				throw ExceptionRequestTerminated();
			}
			
		}
	}
}


void Request::addChunkOfResponse(std::string && chunk)
{
	if (chunk == "") {
		this->chunksToSend._mutex.lock();
		for(int i=0;i<this->chunksToSend_started.size();i++) {
			this->chunksToSend_started[i] = this->chunksToSend_finished[i] = true;
		}
		this->chunksToSend._mutex.unlock();
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
			this->chunksToSend._mutex.lock();
			bool isFinished = this->allFinished();
			this->chunksToSend._mutex.unlock();
			if(isFinished) return; // there is nothing more to do

			this->chunksToSend._mutex.lock();
			for(int i=0;i<this->chunksToSend_started.size();i++) {
				if (! this->chunksToSend_started[i]) {
					this->chunksToSend_started[i] = true;
				}
			}
			this->bytesCount += chunk.size();
			this->chunksToSend._mutex.unlock();
			this->chunksToSend.push_back(std::move(chunk), [this](){return this->allFinished();} );
			chunk = "";
			if (requestTerminated) {
				this->chunksToSend._mutex.lock();
				for(int i=0;i<this->chunksToSend_finished.size();i++) {
					this->chunksToSend_finished[i] = true;
				}
				this->chunksToSend.purge();
				this->chunksToSend._mutex.unlock();
				throw ExceptionRequestTerminated();
			}
			return;
		}
	}
}


void Request::setResponse(Document const & doc)
{
	std::string chunk = outputBuilder.createOutput(doc);
	{
		this->chunksToSend._mutex.lock();
		bool isStarted = false;
		for(int i=0;i<this->chunksToSend_started.size();i++) {
			if(this->chunksToSend_started[i] == true) {
				isStarted = true;
			}
		}
		this->chunksToSend._mutex.unlock();
		if(isStarted) throw std::logic_error("Response was already prepared.");
		this->chunksToSend._mutex.lock();
		for(int i=0;i<chunksToSend_started.size();i++) {
			chunksToSend_started[i] = chunksToSend_finished[i] = true;
		}
		this->bytesCount += chunk.size();
		++(this->documentsCount);
		this->chunksToSend._mutex.unlock();
		this->chunksToSend.push_back(std::move(chunk), [this](){return this->allFinished();} );
		this->chunksToSend._mutex.lock();
		if (doc.isError()) error = doc.error().type;
		this->chunksToSend._mutex.unlock();
	}
}


void Request::setResponse(std::string const & text)
{
	{
		this->chunksToSend._mutex.lock();
		bool isStarted = false;
		for(int i=0;i<this->chunksToSend_started.size();i++) {
			if(this->chunksToSend_started[i] == true) {
				isStarted = true;
			}
		}
		this->chunksToSend._mutex.unlock();
		if(isStarted) throw std::logic_error("Response was already prepared.");
		this->chunksToSend._mutex.lock();
		for(int i=0;i<chunksToSend_started.size();i++) {
			chunksToSend_started[i] = chunksToSend_finished[i] = true;
		}
		this->chunksToSend._mutex.unlock();
		this->chunksToSend.push_back(text, [this](){return this->allFinished();} );
	}
}


void Request::internalThread(int id) noexcept
{
	try {
		bool const isJson = outputBuilder.prettyOrCompressedJson();
		Document doc;
		try {
			this->process(); // can use id parameter to parallelize, but this is disabled for now
			{
				this->chunksToSend._mutex.lock();
				bool isStarted = chunksToSend_started[id];
				bool isFinished = false;
				int finishedCount = 0;
				for(int i=0;i<this->chunksToSend_finished.size();i++) {
					if(this->chunksToSend_finished[i] == true) {
						finishedCount++;
					}
				}
				isFinished = (this->chunksToSend_finished.size() - finishedCount) != 1;
				this->chunksToSend._mutex.unlock();
				if (! isStarted) {
					if (isJson) {
						chunksToSend.push_back("[]", [this](){return this->allFinished();} );
					}
				} else if (! isFinished) {
					if (isJson) {
						this->chunksToSend._mutex.lock();
						bool isEmpty = this->chunksToSend.empty();
						this->chunksToSend._mutex.unlock();
						if (isEmpty) {
							chunksToSend.push_back("]", [this](){return this->allFinished();} );
						} else {
							this->chunksToSend._mutex.lock();
							chunksToSend.back().push_back(']');
							this->chunksToSend._mutex.unlock();
						}
					}
				}
				this->chunksToSend._mutex.lock();
				chunksToSend_started[id] = chunksToSend_finished[id] = true;
				this->chunksToSend._mutex.unlock();
			}
			return;  // success
		} catch (...) {
			doc = DocumentError::createFromCurrentException();
		}
		// try to finish response
		{
			this->chunksToSend._mutex.lock();
			bool isStarted = chunksToSend_started[id];
			bool isFinished = chunksToSend_finished[id];
			this->chunksToSend._mutex.unlock();
			if (isFinished) return; // TODO - log it somewhere
			if (isStarted) {
				std::string row = isJson ? "," : "";
				row.append(outputBuilder.createOutput(doc) + "\n");
				if (isJson) row.append("]");
				chunksToSend.push_back(row, [this](){return this->allFinished();} );
				this->chunksToSend._mutex.lock();
				error = errorType::InternalServerError;
				this->chunksToSend._mutex.unlock();
			} else {
				chunksToSend.push_back( outputBuilder.createOutput(doc), [this](){return this->allFinished();} );
				this->chunksToSend._mutex.lock();
				error = doc.error().type;
				this->chunksToSend._mutex.unlock();
			}
			this->chunksToSend._mutex.lock();
			chunksToSend_started[id] = chunksToSend_finished[id] = true;
			this->chunksToSend._mutex.unlock();
		}
	} catch (...) {
		// total fail, just finish the response
		std::lock_guard<std::mutex> synchScope(this->chunksToSend._mutex);
		for(int i=0;i<chunksToSend_started.size();i++) {
			chunksToSend_started[i] = chunksToSend_finished[i] = true;
		}
		error = errorType::InternalServerError;
	}
}


void Request::startProcessingThread(responseFormat respFormat, std::string const & respFields)
{
	for(int t=0;t<MAX_PROCESSING_THREADS;t++) {
		this->chunksToSend_started.push_back(false);
		this->chunksToSend_finished.push_back(false);
	}
	outputBuilder.setFormat(respFormat, respFields);
	for(int t=0;t<MAX_PROCESSING_THREADS;t++) {
		this->processingThread.push_back(std::thread(&Request::internalThread, this, t));
	}
}


void Request::stopProcessingThread() noexcept
{
	try {
		this->chunksToSend._mutex.lock();
		for(int i=0;i<this->chunksToSend_started.size();i++) {
			this->chunksToSend_started[i] = this->chunksToSend_finished[i] = true;
		}
		this->chunksToSend._mutex.unlock();
		this->chunksToSend.purge();
		for(int i=0;i<this->processingThread.size();i++) {
			if (this->processingThread[i].joinable()) {
				killThread.store(true);
				processingThread[i].join();
			}
		}
		this->processingThread.clear();
		{
			std::lock_guard<std::mutex> synchScope(this->chunksToSend._mutex);
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
	//std::cout << "ZZZ " << mappedVariant.refId.value << " " << mappedVariant.modifications.front().region.toString() << " " << mappedVariant.modifications.front().originalSequence << std::endl;
					NormalizedGenomicVariant temp = canonicalizeGenomic(referencesDb, mappedVariant.align(fAlignment, referencesDb));
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
		if(!proteinHgvsDef.empty()) {
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
							calculateProteinVariation(referencesDb, var, v.proteinHgvsDef, v.proteinHgvsCanonical);

							if(v.proteinHgvsDef.size() > 0) { 
								for(auto & n: names) {
									if(referencesDb->getAllTranscriptsByNameExists(n)) {
										try {
											auto iter = referencesDb->getAllTranscriptsByName(n);
											std::stringstream ss;
											v.maneStatus = iter->second.second.maneStatus;
											v.maneVersion = referencesDb->getManeVersion();
											std::string clingenPreferredTitle = getClingenPreferredTitle(referencesDb, n, v.hgvsDefs, v.proteinHgvsDef, v.geneId, d);
											if(v.maneStatus.compare("MANE Select") == 0) {
												auto it = d.clingenPreferredTitle.begin();
												std::string toerase = "";
												while(it != d.clingenPreferredTitle.end()) {
													if((*it).substr(0,2) == "NC") {
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
								for (auto & n: names) if (n.substr(0,2) == "NC") {
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
						std::string hgvs = toHgvsModifications(referencesDb, nv, fAlignment);
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
			this->chunksToSend._mutex.lock();
			bool isFinished = this->allFinished();
			bool isEmpty = this->chunksToSend.empty();
			this->chunksToSend._mutex.unlock();
			if (isEmpty && isFinished) break;
			{
				chunk = this->chunksToSend.pop_front([this](){return this->allFinished();} );
				return true;
			}
		}
	}
	return false;
}


std::string Request::getRequestToGUI() const
{
	throw ExceptionIncorrectRequest("Output of this request cannot be represented as HTML data");
}

