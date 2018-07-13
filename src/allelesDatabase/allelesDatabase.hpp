#ifndef ALLELESDATABASE_HPP_
#define ALLELESDATABASE_HPP_

#include "../core/document.hpp"
#include "../referencesDatabase/referencesDatabase.hpp"
#include <functional>

class AllelesDatabase
{
private:
	struct Pim;
	Pim * pim;
public:
	typedef std::function<void(std::vector<Document> &, bool & lastCall)> callbackSendChunk;
	AllelesDatabase(ReferencesDatabase const * refDb, Configuration const & conf);
	~AllelesDatabase();
	// =============== service
	void rebuildIndexes(std::set<identifierType> const & idsTypes);
	// =============== fetch... methods - operate on a vector of documents (the size of the vector is not changed)
	// fetch variants definitions basing on CA/PA Id (no identifiers/revisions are filled)
	// document with non-existing CA/PA ID are converted to DocumentError with NOT_FOUND error
	void fetchVariantsByCaPaIds(std::vector<Document> &);
	// fetch full variants info (with identifiers and revision) by definitions
	// for unmatched documents CA/PA ID & identifiers are set to null
	void fetchVariantsByDefinition(std::vector<Document> &); // TODO - allow for removing duplicates?
	// fetch full variant info and add new variants and/or identifiers
	// if in given document a variant definition does not match to CA/PA ID, CA/PA ID and identifiers are set to null
	void fetchVariantsByDefinitionAndAddIdentifiers(std::vector<Document> &);
	// fetch full variant info and add remove given identifiers (not CA/PA ID)
	// if in given document a variant definition does not match to CA/PA ID, CA/PA ID and identifiers are set to null
	void fetchVariantsByDefinitionAndDeleteIdentifiers(std::vector<Document> &);
	// remove variants
	void fetchVariantsByDefinitionAndDelete(std::vector<Document> &);
	// =============== query... methods - returns vector of matching documents basing on given criteria
	// query variants (with identifiers and revision) by short identifiers (the vector of identifiers must be small)
	void queryVariants(callbackSendChunk, std::vector<std::pair<identifierType,uint32_t>> const & ids);
	// query variants (with identifiers and revision) with at least one identifier of given type, if idType is empty, all variants are returned
	void queryVariants(callbackSendChunk, unsigned & recordsToSkip, std::vector<identifierType> const & idType = std::vector<identifierType>(), unsigned hintQuerySize = std::numeric_limits<unsigned>::max());
	// query variants from given regions (with identifiers and revision)
	void queryVariants(callbackSendChunk, unsigned & recordsToSkip            // parameter to skip first n records, it is decreased accordingly
						, ReferenceId refId
						, uint32_t from = 0                                   // region to query -from
						, uint32_t to = std::numeric_limits<uint32_t>::max()  // region to query - to
						, unsigned minChunkSize = 32*1204                     // min number of records to return in single callback
						, unsigned hintQuerySize = std::numeric_limits<unsigned>::max() // this is for prioritization, currently based on number of records (it would be better to put priority here)
						);
	// query all genomic variants
	void queryGenomicVariants(callbackSendChunk, unsigned & recordsToSkip, unsigned minChunkSize = 32*1204);
	// query all protein variants
	void queryProteinVariants(callbackSendChunk, unsigned & recordsToSkip, unsigned minChunkSize = 32*1204);
	// ================ delete an interval of keys from an index
	// delete interval of short identifiers
	void deleteIdentifiers(identifierType, uint32_t from = 0, uint32_t to = std::numeric_limits<uint32_t>::max());
};

#endif /* ALLELESDATABASE_HPP_ */
