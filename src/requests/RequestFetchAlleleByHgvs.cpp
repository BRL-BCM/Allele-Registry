#include "requests.hpp"
#include "hgvs.hpp"
#include "canonicalization.hpp"


RequestFetchAlleleByHgvs::RequestFetchAlleleByHgvs( std::string const & pId, bool registerNewAllele )
: Request(documentType::activeGenomicVariant), fHgvs(pId), fRegisterNewAllele(registerNewAllele)
{}


RequestFetchAlleleByHgvs::~RequestFetchAlleleByHgvs()
{
	stopProcessingThread();
}


void RequestFetchAlleleByHgvs::process()
{
	HgvsVariant hgvsVar(true);
	decodeHgvs(referencesDb, fHgvs, hgvsVar);
	std::vector<Document> documents(1);
	if (hgvsVar.isGenomic()) {
		documents[0] = DocumentActiveGenomicVariant(canonicalize(referencesDb, hgvsVar.genomic));
		mapVariantsToMainGenome(documents);
	} else {
		documents[0] = DocumentActiveProteinVariant(canonicalize(referencesDb, hgvsVar.protein));
	}
	if (fRegisterNewAllele) {
		allelesDb->fetchVariantsByDefinitionAndAddIdentifiers(documents);
	} else {
		allelesDb->fetchVariantsByDefinition(documents);
	}
	fillVariantsDetails(documents);
	setResponse(documents[0]);
}


std::string RequestFetchAlleleByHgvs::getRequestToGUI() const
{
	return ("/redmine/projects/registry/genboree_registry/allele?hgvs=" + fHgvs);
}

