#include "requests.hpp"


RequestFetchAlleleById::RequestFetchAlleleById( std::string const & pId )
: Request(documentType::activeGenomicVariant), fId(pId)
{}


RequestFetchAlleleById::~RequestFetchAlleleById()
{
	stopProcessingThread();
}


void RequestFetchAlleleById::process()
{
	if (fId.size() < 3 || (fId.substr(0,2) != "CA" && fId.substr(0,2) != "PA")) {
		throw ExceptionIncorrectRequest("Canonical allele id must consist of prefix 'CA' or 'PA' and decimal number. Given value: '" + fId + "'.");
	}
	for (unsigned i = 2; i < fId.size(); ++i) if (fId[i] < '0' || fId[i] > '9') {
		throw ExceptionIncorrectRequest("Canonical allele id must consist of prefix 'CA' or 'PA' and decimal number. Given value: '" + fId + "'.");
	}
	std::vector<Document> documents;
	if (fId.substr(0,2) == "CA") {
		documents.push_back(DocumentActiveGenomicVariant());
		documents[0].asActiveGenomicVariant().caId = CanonicalId(boost::lexical_cast<uint32_t>(fId.substr(2)));
	} else {
		documents.push_back(DocumentActiveProteinVariant());
		documents[0].asActiveProteinVariant().caId = CanonicalId(boost::lexical_cast<uint32_t>(fId.substr(2)));
	}
	allelesDb->fetchVariantsByCaPaIds(documents);
	allelesDb->fetchVariantsByDefinition(documents);
	fillVariantsDetails(documents);
	setResponse(documents[0]);
}


std::string RequestFetchAlleleById::getRequestToGUI() const
{
	return ("/redmine/projects/registry/genboree_registry/by_caid?caid=" + fId);
}

