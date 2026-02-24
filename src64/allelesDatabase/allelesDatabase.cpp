#include "allelesDatabase.hpp"
#include <sstream>
#include <algorithm>
#include <set>
#include <mutex>
#include <boost/thread.hpp>

#include "TableSequence.hpp"
#include "TableProteinSequence.hpp"
#include "TableGenomic64.hpp"
#include "TableProtein64.hpp"
#include "TableNamed.hpp"
#include "IndexGenomicComplex.hpp"
#include "IndexIdentifierCa.hpp"
#include "IndexIdentifierPa.hpp"
#include "IndexIdentifierUInt64.hpp"
#include "tools.hpp"

#include "../apiDb/db.hpp"
#include "../commonTools/Stopwatch.hpp"

#include <fstream>


struct AllelesDatabase::Pim
{
	std::atomic<uint64_t> nextCaId;
	TasksManager cpuTaskManager;
	TasksManager ioTasksManager;
	TableSequence tabSequence;
	TableProteinSequence tabSequenceProtein;
	TableGenomic  tabGenomic;
	TableProtein  tabProtein;
	TableNamed<RecordGenomicVariant>    tabNamed;
	TableNamed<RecordProteinVariant>    tabNamedProtein;
	//IndexGenomicComplex indexGenomicComplex;
	IndexIdentifierCa indexIdentifierCa;
	IndexIdentifierPa indexIdentifierPa;
	std::map<identifierType, IndexIdentifierUInt64*> indexIdentifierUInt64;
	ReferencesDatabase const * refDb;
	std::vector<uint64_t> genomicReferencesToKeyOffsets;
	Pim(Configuration const & conf, ReferencesDatabase const * pRefDb)
	: cpuTaskManager(conf.allelesDatabase_threads), ioTasksManager(conf.allelesDatabase_threads)
	, tabSequence(conf.allelesDatabase_path, conf.header.find("sequence")->second, &cpuTaskManager, &ioTasksManager, conf.allelesDatabase_cache_sequence)
	, tabSequenceProtein(conf.allelesDatabase_path, conf.header.find("sequence-protein")->second, &cpuTaskManager, &ioTasksManager, conf.allelesDatabase_cache_sequence)
	, tabGenomic(conf.allelesDatabase_path, conf.header.find("genomic")->second, &cpuTaskManager, &ioTasksManager, conf.allelesDatabase_cache_genomic, nextCaId)
	, tabProtein(conf.allelesDatabase_path, conf.header.find("protein")->second, &cpuTaskManager, &ioTasksManager, conf.allelesDatabase_cache_protein, nextCaId) // TODO - PaId
	, tabNamed(conf.allelesDatabase_path, "named", conf.header.find("named")->second, &cpuTaskManager, &ioTasksManager, conf.allelesDatabase_cache_named)
	, tabNamedProtein(conf.allelesDatabase_path, "named-protein", conf.header.find("named-protein")->second, &cpuTaskManager, &ioTasksManager, conf.allelesDatabase_cache_named_protein)
	//, indexGenomicComplex(conf.allelesDatabase_path, cpuTaskManager)
	, indexIdentifierCa(conf.allelesDatabase_path, conf.header.find("idCa")->second, &cpuTaskManager, &ioTasksManager, conf.allelesDatabase_cache_idCa)
	, indexIdentifierPa(conf.allelesDatabase_path, conf.header.find("idPa")->second, &cpuTaskManager, &ioTasksManager, conf.allelesDatabase_cache_idPa)
	, refDb(pRefDb)
	{
		//nextCaId = std::max((uint64_t)std::numeric_limits<uint32_t>::max(),std::max(indexIdentifierCa.getMaxIdentifier(), indexIdentifierPa.getMaxIdentifier())) + 1; // TODO - PaId
		nextCaId = std::max(indexIdentifierCa.getMaxIdentifier(), indexIdentifierPa.getMaxIdentifier()) + 1; // TODO - PaId
		indexIdentifierUInt64[identifierType::dbSNP         ] = new IndexIdentifierUInt64(conf.allelesDatabase_path, conf.header.find("idDbSnp")->second, &cpuTaskManager, &ioTasksManager, "DbSnp"         , conf.allelesDatabase_cache_idDbSnp);
		indexIdentifierUInt64[identifierType::ClinVarAllele ] = new IndexIdentifierUInt64(conf.allelesDatabase_path, conf.header.find("idClinVarAllele")->second, &cpuTaskManager, &ioTasksManager, "ClinVarAllele" , conf.allelesDatabase_cache_idClinVarAllele);
		indexIdentifierUInt64[identifierType::ClinVarVariant] = new IndexIdentifierUInt64(conf.allelesDatabase_path, conf.header.find("idClinVarVariant")->second, &cpuTaskManager, &ioTasksManager, "ClinVarVariant", conf.allelesDatabase_cache_idClinVarVariant);
		indexIdentifierUInt64[identifierType::ClinVarRCV    ] = new IndexIdentifierUInt64(conf.allelesDatabase_path, conf.header.find("idClinVarRCV")->second, &cpuTaskManager, &ioTasksManager, "ClinVarRCV"    , conf.allelesDatabase_cache_idClinVarRCV);
		// this is needed to convert ref+position to uniform 32-bit position value
		std::vector<unsigned> refsLengths = refDb->getMainGenomeReferencesLengths();
		genomicReferencesToKeyOffsets.resize(refsLengths.size(), 0);
		for (unsigned i = 1; i < genomicReferencesToKeyOffsets.size(); ++i) {
			genomicReferencesToKeyOffsets[i] = genomicReferencesToKeyOffsets[i-1] + (uint64_t)refsLengths[i-1];
		}
	}
	~Pim()
	{
		for (auto & kv: indexIdentifierUInt64) delete kv.second;
	}

	inline void proteinKey2Coordinates(uint64_t key, ReferenceId & refId, uint16_t & position) const
	{
		refId = refDb->getReferenceIdFromProteinAccessionIdentifier(key >> 16);
		position = key % (256*256);
	}
	inline void genomicKey2Coordinates(uint64_t key, ReferenceId & refId, uint64_t & position) const
	{
		auto it = std::upper_bound( genomicReferencesToKeyOffsets.begin(), genomicReferencesToKeyOffsets.end(), key );
		ASSERT( it != genomicReferencesToKeyOffsets.begin() );
		--it;
		position = key - *it;
		refId.value = it - genomicReferencesToKeyOffsets.begin();
	}
	inline void proteinCoordinates2Key(ReferenceId refId, uint16_t position, uint64_t & key) const
	{
		key = refDb->getMetadata(refId).proteinAccessionIdentifier;
		key <<= 16;
		key += position;
	}
	inline void genomicCoordinates2Key(ReferenceId refId, uint64_t position, uint64_t & key) const
	{
		if (refId.value >= genomicReferencesToKeyOffsets.size()) throw std::logic_error("Reference Id outside the main genome!");
		key = genomicReferencesToKeyOffsets[refId.value] + (uint64_t)position;
	}

//	// this is needed to convert ref+position to uniform 64-bit position value
//	std::vector<unsigned> refsLengths = pim->refDb->getMainGenomeReferencesLengths();
//	std::vector<unsigned> refsOffsets(refsLengths.size(), 0);
//	for (unsigned i = 1; i < refsOffsets.size(); ++i) refsOffsets[i] = refsOffsets[i-1] + refsLengths[i-1];
//	if (refId.value >= refsOffsets.size()) throw std::logic_error("Reference Id outside the main genome!");
//	from += refsOffsets[refId.value];
//	to   += refsOffsets[refId.value];

//	// this is needed to convert ref+position to uniform 64-bit position value
//	std::vector<unsigned> refsLengths = refDb->getMainGenomeReferencesLengths();
//	std::vector<unsigned> refsOffsets(refsLengths.size(), 0);
//	for (unsigned i = 1; i < refsOffsets.size(); ++i) refsOffsets[i] = refsOffsets[i-1] + refsLengths[i-1];

	void convertToVariantRecords
			( std::vector<Document> & pDefinitions
			, bool registerNewSequences
			, std::vector<RecordGenomicVariant*> & genomicRecords  // this vector is cleared at the beginning, subset of Documents being active genomic variants converted to records
			, std::vector<unsigned> & genomicRecordsIndices // this vector is cleared at the beginning, it contains corresponding indices from vector with Documents
			, std::vector<RecordProteinVariant*> & proteinRecords  // this vector is cleared at the beginning, subset of Documents being active genomic variants converted to records
			, std::vector<unsigned> & proteinRecordsIndices // this vector is cleared at the beginning, it contains corresponding indices from vector with Documents
			)
	{
		// Indices to return
		genomicRecordsIndices.clear();
		proteinRecordsIndices.clear();
		genomicRecordsIndices.reserve(pDefinitions.size());
		proteinRecordsIndices.reserve(pDefinitions.size());
		// vector with raw definitions
		std::vector<std::vector<BinaryNucleotideSequenceModification>> rawDefinitions;
		rawDefinitions.reserve(pDefinitions.size());
		// vector with sequences to query/register
		std::vector<std::string const *> seqToQuery;
		std::vector<uint32_t*> seqToQueryOut;
		// create vectors
		for ( unsigned iDoc = 0;  iDoc < pDefinitions.size();  ++iDoc ) {
			if ( pDefinitions[iDoc].isActiveProteinVariant() ) {
				// protein variation
				DocumentActiveProteinVariant const & doc = pDefinitions[iDoc].asActiveProteinVariant();
				NormalizedProteinVariant const & def = doc.mainDefinition;
				std::vector<BinaryAminoAcidSequenceModification> vMods(def.modifications.size());
				std::vector<std::string const *> seqProteinToQuery;
				std::vector<uint32_t*> seqProteinToQueryOut;
				for (unsigned i = 0; i < def.modifications.size(); ++i) {
					BinaryAminoAcidSequenceModification & r = vMods[i];
					NormalizedSequenceModification const & m = def.modifications[i];
					r.category = m.category;
					r.position = m.region.left();
					r.lengthBefore = m.region.length();
					if (r.category == variantCategory::shiftableDeletion || r.category == variantCategory::duplication) {
						r.lengthChangeOrSeqLength = m.lengthChange;
					} else {
						r.lengthChangeOrSeqLength = m.insertedSequence.size();
						if (r.lengthChangeOrSeqLength <= 7) {
							r.sequence = convertProteinToBinary(m.insertedSequence);
						} else {
							//throw std::logic_error("Insertion larger than 7 amino-acids are not supported!");
							seqProteinToQuery.push_back(&(m.insertedSequence));
							seqProteinToQueryOut.push_back(&(r.sequence));
						}
					}
				}
				{
					std::vector<uint64_t> seqProteinIds;
					if (registerNewSequences) {
						tabSequenceProtein.fetchAndAdd(seqProteinToQuery, seqProteinIds);
					} else {
						tabSequenceProtein.fetch(seqProteinToQuery, seqProteinIds);
					}
					for (unsigned i = 0; i < seqProteinIds.size(); ++i) *(seqProteinToQueryOut[i]) = seqProteinIds[i];
				}
				proteinRecordsIndices.push_back(iDoc);
				proteinRecords.push_back(new RecordProteinVariant(BinaryProteinVariantDefinition(refDb->getMetadata(def.refId).proteinAccessionIdentifier,vMods)));
				proteinRecords.back()->identifiers = BinaryIdentifiers(identifierType::PA, doc.identifiers.rawShort(), doc.identifiers.rawHgvs());
				proteinRecords.back()->identifiers.lastId() = doc.caId.value;
			} else if ( pDefinitions[iDoc].isActiveGenomicVariant() ) {
				// genomic variation
				NormalizedGenomicVariant const & def = pDefinitions[iDoc].asActiveGenomicVariant().mainDefinition;
				genomicRecordsIndices.push_back(iDoc);
				// definition
				rawDefinitions.push_back( std::vector<BinaryNucleotideSequenceModification>(def.modifications.size()) );
				for (unsigned i = 0; i < def.modifications.size(); ++i) {
					BinaryNucleotideSequenceModification & r = rawDefinitions.back()[i];
					NormalizedSequenceModification const & m = def.modifications[i];
					r.category = m.category;
					genomicCoordinates2Key(def.refId, m.region.left(), r.position);
					r.lengthBefore = m.region.length();
					if (r.category == variantCategory::shiftableDeletion || r.category == variantCategory::duplication) {
						r.lengthChangeOrSeqLength = m.lengthChange;
					} else {
						r.lengthChangeOrSeqLength = m.insertedSequence.size();
						if (r.lengthChangeOrSeqLength <= 16) {
							r.sequence = convertGenomicToBinary(m.insertedSequence);
						} else {
							seqToQuery.push_back(&(m.insertedSequence));
							seqToQueryOut.push_back(&(r.sequence));
						}
					}
				}
			}
		}
		// convert sequences
		{
			std::vector<uint64_t> seqIds;
			if (registerNewSequences) {
				tabSequence.fetchAndAdd(seqToQuery, seqIds);
			} else {
				tabSequence.fetch(seqToQuery, seqIds);
			}
			for (unsigned i = 0; i < seqIds.size(); ++i) *(seqToQueryOut[i]) = seqIds[i];
		}
		// copy data to output vectors (remove records with unknown sequences)
		genomicRecords.clear();
		genomicRecords.reserve(rawDefinitions.size());
		for ( unsigned iSrc = 0;  iSrc < rawDefinitions.size();  ++iSrc ) {
			// check for unknown sequences
			bool unknownSequences = false;
			for (auto const & m: rawDefinitions[iSrc]) if (m.lengthChangeOrSeqLength > 16 && m.sequence == tabSequence.unknownSequence) unknownSequences = true;
			if (unknownSequences) continue;
			// copy data to records
			unsigned const iTrg = genomicRecords.size();
			unsigned const iDoc = genomicRecordsIndices[iSrc];
			genomicRecordsIndices[iTrg] = genomicRecordsIndices[iSrc];
			genomicRecords.push_back( new RecordGenomicVariant( BinaryGenomicVariantDefinition(rawDefinitions[iSrc])) );
			genomicRecords.back()->identifiers = BinaryIdentifiers( identifierType::CA
												, pDefinitions[iDoc].asActiveGenomicVariant().identifiers.rawShort()
												, pDefinitions[iDoc].asActiveGenomicVariant().identifiers.rawHgvs() );
			genomicRecords.back()->identifiers.lastId() = pDefinitions[iDoc].asActiveGenomicVariant().caId.value;
		}
		genomicRecordsIndices.resize(genomicRecords.size());
	}


	void overwriteIdentifiers
		( std::vector<Document> & docs
		, std::vector<RecordGenomicVariant*> const & genomicRecords
		, std::vector<unsigned> const & genomicRecordsIndices
		, std::vector<RecordProteinVariant*> const & proteinRecords
		, std::vector<unsigned> const & proteinRecordsIndices
		)
	{
		// save data to documents
		for (unsigned i = 0; i < genomicRecords.size(); ++i) {
			unsigned const iDoc = genomicRecordsIndices[i];
			RecordGenomicVariant const * r = genomicRecords[i];
			docs[iDoc].asActiveGenomicVariant().caId.value = r->identifiers.lastId();
			docs[iDoc].asActiveGenomicVariant().identifiers = Identifiers( r->identifiers.rawShort(), r->identifiers.rawHgvs() );
		}
		// save data to documents
		for (unsigned i = 0; i < proteinRecords.size(); ++i) {
			unsigned const iDoc = proteinRecordsIndices[i];
			RecordProteinVariant const * r = proteinRecords[i];
			docs[iDoc].asActiveProteinVariant().caId.value = r->identifiers.lastId();
			docs[iDoc].asActiveProteinVariant().identifiers = Identifiers( r->identifiers.rawShort(), r->identifiers.rawHgvs() );
		}
	}


	void overwriteDefinitions
		( std::vector<Document> & docs
		, std::vector<RecordGenomicVariant*> const & genomicRecords
		, std::vector<unsigned> & genomicRecordsIndices
		)
	{
		// sequences to query
		std::vector<std::string*> seqToQueryOut;
		std::vector<uint64_t> seqToQueryIn;
		// vectors to return
		for (unsigned i = 0; i < genomicRecords.size(); ++i) {
			unsigned const iDoc = genomicRecordsIndices[i];
			RecordGenomicVariant const * r = genomicRecords[i];
			if (r == nullptr) {
				docs[iDoc] = DocumentError(errorType::NotFound);
			} else {
				ReferenceId refId;
				uint64_t position;
				auto key = (r->key<(uint64_t(1)<<63))?r->key:((r->key-(uint64_t(1)<<63))>>24);
				genomicKey2Coordinates(key, refId, position);
				docs[iDoc].asActiveGenomicVariant().mainDefinition.refId = refId;
				docs[iDoc].asActiveGenomicVariant().mainDefinition.modifications.clear();
				docs[iDoc].asActiveGenomicVariant().mainDefinition.modifications.resize(r->definition.raw().size());
				for ( unsigned j = 0; j < r->definition.raw().size(); ++j ) {
					BinaryNucleotideSequenceModification const & sm = r->definition.raw()[j];
					NormalizedSequenceModification & nsm = docs[iDoc].asActiveGenomicVariant().mainDefinition.modifications[j];
					nsm.category = sm.category;
					nsm.region.setLeftAndLength( position + (sm.position - key), sm.lengthBefore );
					if (sm.category == variantCategory::shiftableDeletion || sm.category == variantCategory::duplication) {
						nsm.lengthChange = sm.lengthChangeOrSeqLength;
					} else {
						if (sm.lengthChangeOrSeqLength > 16) {
							seqToQueryIn.push_back(sm.sequence);
							seqToQueryOut.push_back(&(nsm.insertedSequence));
						} else {
							nsm.insertedSequence = convertBinaryToGenomic(sm.sequence, sm.lengthChangeOrSeqLength);
						}
					}
				}
			}
		}
		// query sequences
		if (! seqToQueryIn.empty()) tabSequence.fetch(seqToQueryIn, seqToQueryOut);
		// fill original sequences
		for (auto & doc: docs) {
			if (! doc.isActiveGenomicVariant()) continue;
			NormalizedGenomicVariant & var = doc.asActiveGenomicVariant().mainDefinition;
			for (auto & m: var.modifications) {
				switch (m.category) {
					case variantCategory::nonShiftable:
					case variantCategory::shiftableInsertion:
						m.originalSequence = refDb->getSequence(var.refId, m.region);
						break;
					case variantCategory::duplication:
					case variantCategory::shiftableDeletion:
						m.originalSequence = refDb->getSequence(var.refId, RegionCoordinates( m.region.left(), m.region.left() + m.lengthChange ));
						break;
				}
			}
		}
	}

	void overwriteDefinitions
		( std::vector<Document> & docs
		, std::vector<RecordProteinVariant*> const & proteinRecords
		, std::vector<unsigned> & proteinRecordsIndices
		)
	{
		std::vector<std::string*> seqToQueryOut;
		std::vector<uint64_t> seqToQueryIn;
		// vectors to return
		for (unsigned i = 0; i < proteinRecords.size(); ++i) {
			unsigned const iDoc = proteinRecordsIndices[i];
			RecordProteinVariant const * r = proteinRecords[i];
			if (r == nullptr) {
				docs[iDoc] = DocumentError(errorType::NotFound);
			} else {
				docs[iDoc].asActiveProteinVariant().mainDefinition.refId = refDb->getReferenceIdFromProteinAccessionIdentifier(r->definition.proteinAccessionId());
				docs[iDoc].asActiveProteinVariant().mainDefinition.modifications.clear();
				docs[iDoc].asActiveProteinVariant().mainDefinition.modifications.resize(r->definition.raw().size());
				for ( unsigned j = 0; j < r->definition.raw().size(); ++j ) {
					BinaryAminoAcidSequenceModification const & sm = r->definition.raw()[j];
					NormalizedSequenceModification & nsm = docs[iDoc].asActiveProteinVariant().mainDefinition.modifications[j];
					nsm.category = sm.category;
					nsm.region.setLeftAndLength( sm.position, sm.lengthBefore );
					if (sm.category == variantCategory::shiftableDeletion || sm.category == variantCategory::duplication) {
						nsm.lengthChange = sm.lengthChangeOrSeqLength;
					} else {
						if (sm.lengthChangeOrSeqLength > 10000) {
							throw std::logic_error("sm.lengthChangeOrSeqLength > 10000");
						} else if (sm.lengthChangeOrSeqLength > 7) {
							seqToQueryIn.push_back(sm.sequence);
							seqToQueryOut.push_back(&(nsm.insertedSequence));
						} else {
							nsm.insertedSequence = convertBinaryToProtein(sm.sequence, sm.lengthChangeOrSeqLength);
						}
					}
				}
			}
		}
		if (! seqToQueryIn.empty()) tabSequenceProtein.fetch(seqToQueryIn, seqToQueryOut);
		// fill original sequences
		for (auto iDoc: proteinRecordsIndices) {
			Document & doc = docs[iDoc];
			if (! doc.isActiveProteinVariant()) continue;
			NormalizedProteinVariant & var = doc.asActiveProteinVariant().mainDefinition;
			for (auto & m: var.modifications) {
				switch (m.category) {
					case variantCategory::nonShiftable:
					case variantCategory::shiftableInsertion:
						m.originalSequence = refDb->getSequence(var.refId, m.region);
						break;
					case variantCategory::duplication:
					case variantCategory::shiftableDeletion:
						m.originalSequence = refDb->getSequence(var.refId, RegionCoordinates( m.region.left(), m.region.left() + m.lengthChange ));
						break;
				}
			}
		}
	}

	std::vector<Document> convertToDocuments(std::vector<RecordGenomicVariant*> const & varRecords)
	{
		std::vector<Document> docs(varRecords.size(), DocumentActiveGenomicVariant());
		std::vector<unsigned> indicies(varRecords.size());
		for (unsigned i = 0; i < varRecords.size(); ++i) indicies[i] = i;
		overwriteDefinitions(docs, varRecords, indicies);
		overwriteIdentifiers(docs, varRecords, indicies, std::vector<RecordProteinVariant*>(), std::vector<unsigned>());
		return docs;
	}

	std::vector<Document> convertToDocuments(std::vector<RecordProteinVariant*> const & varRecords)
	{
		std::vector<Document> docs(varRecords.size(), DocumentActiveProteinVariant());
		std::vector<unsigned> indicies(varRecords.size());
		for (unsigned i = 0; i < varRecords.size(); ++i) indicies[i] = i;
		overwriteDefinitions(docs, varRecords, indicies);
		overwriteIdentifiers(docs, std::vector<RecordGenomicVariant*>(), std::vector<unsigned>(), varRecords, indicies);
		return docs;
	}
};

AllelesDatabase::AllelesDatabase(ReferencesDatabase const * refDb, Configuration const & conf) : pim(new Pim(conf, refDb))
{
	std::cout << "AlleleDatabase created by " << boost::this_thread::get_id() << std::endl;
	// rebuild indexes if missing
	std::set<identifierType> idsToRebuild;
	for (auto e: pim->indexIdentifierUInt64)
		if (e.second->isNewDb())
			idsToRebuild.insert(e.first);
	if (pim->indexIdentifierCa.isNewDb())
		idsToRebuild.insert(identifierType::CA);
	if (pim->indexIdentifierPa.isNewDb())
		idsToRebuild.insert(identifierType::PA);
	rebuildIndexes(idsToRebuild);
}

AllelesDatabase::~AllelesDatabase()
{
	std::cout << "AlleleDatabase deleted by " << boost::this_thread::get_id() << std::endl;
	delete pim;
}

// =============== service
void AllelesDatabase::rebuildIndexes(std::set<identifierType> const & idsTypes)
{
	if (idsTypes.empty()) return;

	Stopwatch stopwatch;
	std::cout << "====================> Rebuild indexes - table genomic" << std::endl;
	auto visitor = [this,&stopwatch,&idsTypes](std::vector<RecordGenomicVariant*> & varRecords, bool lastCall)
	{
		std::cout << varRecords.size() <<  " records fetched in " << stopwatch.get_time_sec() << "s" << std::endl;
		stopwatch.restart();
		{ // short identifiers
			std::map<identifierType,std::vector<std::pair<uint64_t,RecordVariantPtr>>> ids;
			for (auto r: varRecords) {
				r->identifiers.saveShortIdsToContainer(ids, RecordVariantPtr(r));
			}
			for (auto const & kv: ids) {
				if (idsTypes.count(kv.first) && pim->indexIdentifierUInt64.count(kv.first)) {
					pim->indexIdentifierUInt64[kv.first]->addIdentifiers(kv.second);
				}
			}
		}
		// CA identifiers
		if (idsTypes.count(identifierType::CA)) {
			std::vector<RecordGenomicVariant const *> varRecords3(varRecords.begin(), varRecords.end());
			pim->indexIdentifierCa.addIdentifiers(varRecords3);
		}
		std::cout << "indexes updated in " << stopwatch.get_time_sec() << "s" << std::endl;
		stopwatch.restart();
	};
	unsigned skip = 0;
	pim->tabGenomic.query(visitor, skip, 0, std::numeric_limits<uint64_t>::max(), 16*1024*1024);

	std::cout << "====================> Rebuild indexes - table protein" << std::endl;
	auto visitor2 = [this,&stopwatch,&idsTypes](std::vector<RecordProteinVariant*> & varRecords, bool lastCall)
	{
		std::cout << varRecords.size() <<  " records fetched in " << stopwatch.get_time_sec() << "s" << std::endl;
		stopwatch.restart();
		{ // short identifiers
			std::map<identifierType,std::vector<std::pair<uint64_t,RecordVariantPtr>>> ids;
			for (auto r: varRecords) {
				r->identifiers.saveShortIdsToContainer(ids, RecordVariantPtr(r));
			}
			for (auto const & kv: ids) {
				if (idsTypes.count(kv.first) && pim->indexIdentifierUInt64.count(kv.first)) {
					pim->indexIdentifierUInt64[kv.first]->addIdentifiers(kv.second);
				}
			}
		}
		// PA identifiers
		if (idsTypes.count(identifierType::PA)) {
			std::vector<RecordProteinVariant const *> varRecords3(varRecords.begin(), varRecords.end());
			pim->indexIdentifierPa.addIdentifiers(varRecords3);
		}
		std::cout << "indexes updated in " << stopwatch.get_time_sec() << "s" << std::endl;
		stopwatch.restart();
	};
	skip = 0;
	pim->tabProtein.query(visitor2, skip, 0, std::numeric_limits<uint64_t>::max(), 16*1024*1024);

	std::cout << "====================> Rebuild indexes - COMPLETED" << std::endl;
}

std::vector<Document> AllelesDatabase::filterNamed(std::vector<Document> & docs, bool isExplicit) const {
	//return docs;

	std::vector<Document> out;
	out.reserve(docs.size());
	std::vector<uint64_t> caids;
	caids.reserve(docs.size());
	for(unsigned i=0;i<docs.size();i++) {
		CanonicalId caId;
		if (docs[i].isActiveGenomicVariant()) {
			caId = docs[i].asActiveGenomicVariant().caId;
			if (!caId.isNull()) {
				caids.push_back(caId.value);
			} else {
				caids.push_back(-1);
			}
		} else if (docs[i].isActiveProteinVariant()) {
			caids.push_back(-1);
		} else {
			caids.push_back(-1);
		}
	}
	std::vector<uint64_t> paids;
	paids.reserve(docs.size());
	for(unsigned i=0;i<docs.size();i++) {
		CanonicalId caId;
		if (docs[i].isActiveGenomicVariant()) {
			paids.push_back(-1);
		} else if (docs[i].isActiveProteinVariant()) {
			caId = docs[i].asActiveProteinVariant().caId;
			if (!caId.isNull()) {
				paids.push_back(caId.value);
			} else {
				paids.push_back(-1);
			}
		} else {
			paids.push_back(-1);
		}
	}
	std::vector<bool> filterca = pim->tabNamed.fetch(caids);
	std::vector<bool> filterpa = pim->tabNamedProtein.fetch(paids);
	for(unsigned i=0;i<docs.size();i++) {
		if(docs[i].isActiveGenomicVariant()) {
			if(filterca[i] == true) {
				docs[i].asActiveGenomicVariant().caId = CanonicalId();
			}
			if(isExplicit || filterca[i] == false) {
				out.push_back(docs[i]);
			}
		} else if(docs[i].isActiveProteinVariant()) {
			if(filterpa[i] == true) {
				docs[i].asActiveProteinVariant().caId = CanonicalId();
			}
			if(isExplicit || filterpa[i] == false) {
				out.push_back(docs[i]);
			}
		} else {
			out.push_back(docs[i]);
		}
	}
	return out;

}

// =============== fetch... methods - operate on a vector of documents (the size of the vector is not changed)
// fetch variants definitions basing on CA Id (no identifiers/revisions are filled)
void AllelesDatabase::fetchVariantsByCaPaIds(std::vector<Document> & docs)
{
	// find records to query
	std::vector<unsigned> caIndices, paIndices;
	std::vector<uint64_t> ca, pa;
	for (unsigned iDoc = 0; iDoc < docs.size(); ++iDoc) {
		if (docs[iDoc].isActiveGenomicVariant()) {
			DocumentActiveGenomicVariant const & doc = docs[iDoc].asActiveGenomicVariant();
			if (doc.caId.isNull()) continue;
			ca.push_back( doc.caId.value );
			caIndices.push_back(iDoc);
		} else if (docs[iDoc].isActiveProteinVariant()) {
			DocumentActiveProteinVariant const & doc = docs[iDoc].asActiveProteinVariant();
			if (doc.caId.isNull()) continue;
			pa.push_back( doc.caId.value );
			paIndices.push_back(iDoc);
		}
	}
	std::vector<RecordGenomicVariant*> genomicRecords = pim->indexIdentifierCa.fetchDefinitions(ca);
	std::vector<RecordProteinVariant*> proteinRecords = pim->indexIdentifierPa.fetchDefinitions(pa);

	pim->overwriteDefinitions(docs, genomicRecords, caIndices);
	pim->overwriteDefinitions(docs, proteinRecords, paIndices);
	for (auto r : genomicRecords) delete r;
	for (auto r : proteinRecords) delete r;
}

void AllelesDatabase::deleteVariantsByDefinition(std::vector<Document> & docs)
{
	// encode sequences in definitions
	std::vector<RecordGenomicVariant*> genomicRecords;
	std::vector<RecordProteinVariant*> proteinRecords;
	std::vector<unsigned> genomicRecordsIndices;
	std::vector<unsigned> proteinRecordsIndices;
	pim->convertToVariantRecords(docs, false, genomicRecords, genomicRecordsIndices, proteinRecords, proteinRecordsIndices);
	std::cout << "gen size:" << genomicRecords.size() << std::endl;
	std::cout << "prt size:" << proteinRecords.size() << std::endl;
	std::map<identifierType, std::vector<std::pair<uint64_t,RecordVariantPtr>>> deletedIdentifiersUint32;
	pim->tabGenomic.fetchAndFullDelete2(genomicRecords, deletedIdentifiersUint32);
	//pim->tabGenomic.deleteIdentifiers(genomicRecords, identifierType::CA);
	//pim->tabProtein.deleteIdentifiers(proteinRecords, identifierType::PA);
	pim->indexIdentifierCa.deleteIdentifiers(genomicRecords);
	pim->indexIdentifierPa.deleteIdentifiers(proteinRecords);
	for (auto r : genomicRecords) delete r;
	for (auto r : proteinRecords) delete r;
}

// fetch full variants info (with identifiers and revision) by definitions with optional changes in identifiers
void AllelesDatabase::fetchVariantsByDefinition(std::vector<Document> & docs)
{
	// encode sequences in definitions
	std::vector<RecordGenomicVariant*> genomicRecords;
	std::vector<RecordProteinVariant*> proteinRecords;
	std::vector<unsigned> genomicRecordsIndices;
	std::vector<unsigned> proteinRecordsIndices;
	pim->convertToVariantRecords(docs, false, genomicRecords, genomicRecordsIndices, proteinRecords, proteinRecordsIndices);
	pim->tabGenomic.fetch(genomicRecords);
	pim->tabProtein.fetch(proteinRecords);
	pim->overwriteIdentifiers(docs, genomicRecords, genomicRecordsIndices, proteinRecords, proteinRecordsIndices);
	for (auto r : genomicRecords) delete r;
	for (auto r : proteinRecords) delete r;
}

void AllelesDatabase::fetchVariantsByDefinitionAndAddIdentifiers(std::vector<Document> & docs, bool named, std::vector<std::pair<bool, uint64_t> > * newIds)
{
	std::vector<Document> newDocs;
	Stopwatch stopwatch;

	// encode sequences in definitions
	std::vector<RecordGenomicVariant*> genomicRecords;
	std::vector<RecordProteinVariant*> proteinRecords;
	std::vector<unsigned> genomicRecordsIndices;
	std::vector<unsigned> proteinRecordsIndices;
	pim->convertToVariantRecords(docs, true, genomicRecords, genomicRecordsIndices, proteinRecords, proteinRecordsIndices);
	std::cout << " (conv=" << stopwatch.save_and_restart_sec() << "s" << std::flush;

	std::map<identifierType, std::vector<std::pair<uint64_t,RecordVariantPtr>>> newIdentifiersUint32;
	pim->tabGenomic.fetchAndAdd(genomicRecords, newIdentifiersUint32);
	pim->tabProtein.fetchAndAdd(proteinRecords, newIdentifiersUint32);
	if(newIds != NULL) {
		for(auto x : newIdentifiersUint32[identifierType::CA]) {
			newIds->push_back(std::make_pair(true, x.first));
		}
		for(auto x : newIdentifiersUint32[identifierType::PA]) {
			newIds->push_back(std::make_pair(false, x.first));
		}
	}
	for(auto g : genomicRecords) {
		g->identifiers.saveShortIdsToContainer(newIdentifiersUint32,g);
	}
	std::cout << " fetch=" << stopwatch.save_and_restart_sec() << "s" << std::flush;

	std::vector<RecordGenomicVariant const *> newGenomicRecords;
	std::vector<RecordProteinVariant const *> newProteinRecords;
	for (auto const & e: newIdentifiersUint32[identifierType::CA]) {
		newGenomicRecords.push_back( e.second.asRecordGenomicVariantPtr() );
	}
	for (auto const & e: newIdentifiersUint32[identifierType::PA]) {
		newProteinRecords.push_back( e.second.asRecordProteinVariantPtr() );
	}
	newIdentifiersUint32.erase(identifierType::CA);
	newIdentifiersUint32.erase(identifierType::PA);

	// TODO pim->indexGenomicComplex.addComplexDefinitions(newGenomicRecords);
	// TODO - protein complex alleles
	std::cout << " complex=" << stopwatch.save_and_restart_sec() << "s" << std::flush;

	pim->indexIdentifierCa.addIdentifiers(newGenomicRecords);
	pim->indexIdentifierPa.addIdentifiers(newProteinRecords);
	std::cout << " CA/PA=" << stopwatch.save_and_restart_sec() << "s" << std::flush;

	for (auto const & kv: newIdentifiersUint32) {
		if (pim->indexIdentifierUInt64.count(kv.first)) {
			pim->indexIdentifierUInt64[kv.first]->addIdentifiers(kv.second);
		}
	}
	std::cout << " ids=" << stopwatch.save_and_restart_sec() << "s" << std::flush;

	pim->overwriteIdentifiers(docs, genomicRecords, genomicRecordsIndices, proteinRecords, proteinRecordsIndices);
	std::cout << " conv=" << stopwatch.save_and_restart_sec() << "s) " << std::flush;
	if(named) {
		pim->tabNamed.upset(newGenomicRecords);
		pim->tabNamedProtein.upset(newProteinRecords);
	} else {
		pim->tabNamed.downset(genomicRecords);
		pim->tabNamedProtein.downset(proteinRecords);
	}

	for (auto r : genomicRecords) delete r;
	for (auto r : proteinRecords) delete r;
}

void AllelesDatabase::fetchVariantsByDefinitionAndDeleteIdentifiers(std::vector<Document> & docs)
{
	// encode sequences in definitions
	std::vector<RecordGenomicVariant*> genomicRecords;
	std::vector<RecordProteinVariant*> proteinRecords;
	std::vector<unsigned> genomicRecordsIndices;
	std::vector<unsigned> proteinRecordsIndices;
	pim->convertToVariantRecords(docs, false, genomicRecords, genomicRecordsIndices, proteinRecords, proteinRecordsIndices);

	std::map<identifierType, std::vector<std::pair<uint64_t,RecordVariantPtr>>> deletedIdentifiersUint32;
	pim->tabGenomic.fetchAndDelete(genomicRecords, deletedIdentifiersUint32);
	pim->tabProtein.fetchAndDelete(proteinRecords, deletedIdentifiersUint32);

	for (auto const & kv: deletedIdentifiersUint32) {
		if (pim->indexIdentifierUInt64.count(kv.first)) {
			pim->indexIdentifierUInt64[kv.first]->deleteIdentifiers(kv.second);
		}
	}

	pim->overwriteIdentifiers(docs, genomicRecords, genomicRecordsIndices, proteinRecords, proteinRecordsIndices);

	for (auto r : genomicRecords) delete r;
	for (auto r : proteinRecords) delete r;
}

void AllelesDatabase::fetchVariantsByDefinitionAndDelete(std::vector<Document> & docs)
{
	// encode sequences in definitions
	std::vector<RecordGenomicVariant*> genomicRecords;
	std::vector<RecordProteinVariant*> proteinRecords;
	std::vector<unsigned> genomicRecordsIndices;
	std::vector<unsigned> proteinRecordsIndices;
	pim->convertToVariantRecords(docs, false, genomicRecords, genomicRecordsIndices, proteinRecords, proteinRecordsIndices);

	std::map<identifierType, std::vector<std::pair<uint64_t,RecordVariantPtr>>> deletedIdentifiersUint32;
	pim->tabGenomic.fetchAndFullDelete(genomicRecords, deletedIdentifiersUint32);
	pim->tabProtein.fetchAndFullDelete(proteinRecords, deletedIdentifiersUint32);
	pim->indexIdentifierCa.deleteIdentifiers(genomicRecords);
	pim->indexIdentifierPa.deleteIdentifiers(proteinRecords);

	for (auto const & kv: deletedIdentifiersUint32) {
		if (pim->indexIdentifierUInt64.count(kv.first)) {
			pim->indexIdentifierUInt64[kv.first]->deleteIdentifiers(kv.second);
		}
	}

	pim->overwriteIdentifiers(docs, genomicRecords, genomicRecordsIndices, proteinRecords, proteinRecordsIndices);

	for (auto r : genomicRecords) delete r;
	for (auto r : proteinRecords) delete r;
}


// =============== query... methods - returns vector of matching documents basing on given criteria
// query variants (with identifiers and revision) by identifiers (the vector of identifiers must be small)
void AllelesDatabase::queryVariants(callbackSendChunk callback, std::vector<std::pair<identifierType,uint64_t>> const & ids)
{
	std::map<identifierType, std::vector<uint64_t>> uint32Identifiers;
	for (auto const & vi: ids) uint32Identifiers[vi.first].push_back(vi.second);

	std::vector<RecordGenomicVariant*> recordsGenomic;
	std::vector<RecordProteinVariant*> recordsProtein;

	// query short identifiers
	for (auto const & kv: uint32Identifiers) {
		if ( pim->indexIdentifierUInt64.count(kv.first) == 0 ) continue;
		std::vector<std::vector<RecordVariantPtr>> temp2 = pim->indexIdentifierUInt64[kv.first]->queryDefinitions(kv.second);
		for (auto & temp: temp2) for (auto & r: temp) {
			if (r.isRecordGenomicVariantPtr()) {
				recordsGenomic.push_back(r.asRecordGenomicVariantPtr());
			} else {
				recordsProtein.push_back(r.asRecordProteinVariantPtr());
			}
		}
	}

	{ // query by CA id
		std::vector<RecordGenomicVariant*> temp = pim->indexIdentifierCa.fetchDefinitions(uint32Identifiers[identifierType::CA]);
		for (auto r: temp) if (r != nullptr) recordsGenomic.push_back(r);
	}

	{ // ========== genomic
		auto compRecords = [](RecordGenomicVariant* r1,RecordGenomicVariant* r2)->bool{ return (r1->definition == r2->definition); };
		auto recordsGenomicUnique = recordsGenomic;
		std::sort( recordsGenomicUnique.begin(), recordsGenomicUnique.end(), compRecords );
		auto itEnd = std::unique( recordsGenomicUnique.begin(), recordsGenomicUnique.end(), compRecords );
		recordsGenomicUnique.erase( itEnd, recordsGenomicUnique.end() );

		pim->tabGenomic.fetch(recordsGenomicUnique);
		std::vector<Document> docs = pim->convertToDocuments(recordsGenomicUnique);
		for (auto r: recordsGenomic) delete r;
		bool lastCall = false;
		callback(docs, lastCall);

		if (lastCall) {
			for (auto r: recordsProtein) delete r;
			return;
		}
	}

	{ // query by PA id
		std::vector<RecordProteinVariant*> temp = pim->indexIdentifierPa.fetchDefinitions(uint32Identifiers[identifierType::PA]);
		for (auto r: temp) if (r != nullptr) recordsProtein.push_back(r);
	}

	{ // ========== protein
		auto compRecords = [](RecordProteinVariant* r1,RecordProteinVariant* r2)->bool{ return (r1->definition == r2->definition); };
		auto recordsProteinUnique = recordsProtein;
		std::sort( recordsProteinUnique.begin(), recordsProteinUnique.end(), compRecords );
		auto itEnd = std::unique( recordsProteinUnique.begin(), recordsProteinUnique.end(), compRecords );
		recordsProteinUnique.erase( itEnd, recordsProteinUnique.end() );

		pim->tabProtein.fetch(recordsProteinUnique);
		std::vector<Document> docs = pim->convertToDocuments(recordsProteinUnique);
		for (auto r: recordsProtein) delete r;
		bool lastCall = true;
		callback(docs, lastCall);
	}
}


// query variants (with identifiers and revision) with given identifier
void AllelesDatabase::queryVariants(callbackSendChunk callback, unsigned & recordsToSkip, std::vector<identifierType> const & idTypes, unsigned hintQuerySize)
{
	bool finished = false;

	{ // ----- genomic
		auto visitor = [this,callback,idTypes,&finished](std::vector<RecordGenomicVariant*> & varRecords, bool & lastCall)
		{
			if ( ! idTypes.empty() ) {
				// filter by identifiers
				std::vector<RecordGenomicVariant*> varRecords2;
				varRecords2.reserve(varRecords.size());
				for (auto r: varRecords) {
					if (r->identifiers.hasOneOfIds(idTypes)) varRecords2.push_back(r);
					else delete r;
				}
				varRecords.swap(varRecords2);
			}
			std::vector<Document> docs = pim->convertToDocuments(varRecords);
			callback(docs, finished);
		};
		pim->tabGenomic.query(visitor, recordsToSkip, hintQuerySize);
	}

	if (finished) return;

	{ // ----- protein
		auto visitor = [this,callback,idTypes](std::vector<RecordProteinVariant*> & varRecords, bool & lastCall)
		{
			if ( ! idTypes.empty() ) {
				// filter by identifiers
				std::vector<RecordProteinVariant*> varRecords2;
				varRecords2.reserve(varRecords.size());
				for (auto r: varRecords) {
					if (r->identifiers.hasOneOfIds(idTypes)) varRecords2.push_back(r);
					else delete r;
				}
				varRecords.swap(varRecords2);
			}
			std::vector<Document> docs = pim->convertToDocuments(varRecords);
			callback(docs, lastCall);
		};
		pim->tabProtein.query(visitor, recordsToSkip, hintQuerySize);
	}
}


// query variants from given regions (with identifiers and revision)
void AllelesDatabase::queryVariants(callbackSendChunk callback, unsigned & recordsToSkip, ReferenceId refId, uint64_t from, uint64_t to, unsigned minChunkSize, unsigned hintQuerySize, bool keepFiltered)
{
	if (pim->refDb->isProteinReference(refId)) {
		uint64_t keyFrom, keyTo;
		pim->proteinCoordinates2Key(refId, from, keyFrom);
		pim->proteinCoordinates2Key(refId,   to, keyTo  );
		// query
		auto visitor = [this,callback,keepFiltered](std::vector<RecordProteinVariant*> & varRecords, bool & lastCall)
		{
			std::vector<Document> docs = pim->convertToDocuments(varRecords);
			callback(docs, lastCall);
		};
		pim->tabProtein.query(visitor, recordsToSkip, keyFrom, keyTo, minChunkSize, hintQuerySize, (keepFiltered)?NULL:&pim->tabNamedProtein);
	} else {
		uint64_t keyFrom, keyTo;
		pim->genomicCoordinates2Key(refId, from, keyFrom);
		pim->genomicCoordinates2Key(refId,   to, keyTo  );
		// query
		auto visitor = [this,callback,keepFiltered](std::vector<RecordGenomicVariant*> & varRecords, bool & lastCall)
		{
			std::vector<Document> docs = pim->convertToDocuments(varRecords);
			callback(docs, lastCall);
		};
		pim->tabGenomic.query(visitor, recordsToSkip, keyFrom, keyTo, minChunkSize, hintQuerySize, (keepFiltered)?NULL:&pim->tabNamed);
	}
}


// query all genomic variants
void AllelesDatabase::queryGenomicVariants(callbackSendChunk callback, unsigned & recordsToSkip, unsigned minChunkSize)
{
	auto visitor = [this,callback](std::vector<RecordGenomicVariant*> & varRecords, bool & lastCall)
	{
		std::vector<Document> docs = pim->convertToDocuments(varRecords);
		callback(docs, lastCall);
	};
	pim->tabGenomic.query(visitor, recordsToSkip, 0, std::numeric_limits<uint64_t>::max(), minChunkSize);
}


// query all protein variants
void AllelesDatabase::queryProteinVariants(callbackSendChunk callback, unsigned & recordsToSkip, unsigned minChunkSize)
{
	auto visitor = [this,callback](std::vector<RecordProteinVariant*> & varRecords, bool & lastCall)
	{
		std::vector<Document> docs = pim->convertToDocuments(varRecords);
		callback(docs, lastCall);
	};
	pim->tabProtein.query(visitor, recordsToSkip, 0, std::numeric_limits<uint64_t>::max(), minChunkSize);
}


void AllelesDatabase::deleteIdentifiers(identifierType idType, uint64_t from, uint64_t to)
{
	auto index = pim->indexIdentifierUInt64.find(idType);
	if (index == pim->indexIdentifierUInt64.end()) {
		throw std::logic_error("This identifiers cannot be deleted this way");
	}

	auto deleteFunc = [this,idType](std::vector<RecordVariantPtr> & variants, bool & lastCall)->void
	{
		// split records into genomic & protein
		std::vector<RecordGenomicVariant*> genomicVars;
		std::vector<RecordProteinVariant*> proteinVars;
		for (auto v: variants) {
			if (v.isRecordGenomicVariantPtr()) {
				genomicVars.push_back(v.asRecordGenomicVariantPtr());
			} else {
				proteinVars.push_back(v.asRecordProteinVariantPtr());
			}
		}
		// delete identifiers from variant's records
		std::sort(genomicVars.begin(), genomicVars.end(), [](RecordGenomicVariant const * a, RecordGenomicVariant const * b){return a->definition < b->definition;});
		uint64_t batch_size = 64*1024;
		for(uint64_t i=0;i<genomicVars.size();i+=batch_size) {
			std::vector<RecordGenomicVariant*> tmp;
			tmp.reserve(batch_size);
			for(int j=0;j<batch_size&&i+j<genomicVars.size();j++) {
				tmp.push_back(genomicVars[i+j]);
			}
			pim->tabGenomic.deleteIdentifiers(tmp, idType);
			std::vector<RecordGenomicVariant*>().swap(tmp);
		}
		pim->tabProtein.deleteIdentifiers(proteinVars, idType);
		for (auto r: genomicVars) delete r;
		for (auto r: proteinVars) delete r;
	};

	index->second->deleteEntries(deleteFunc, from, to, 512*1024);
}


