#include "requests.hpp"
#include "../externalSources/externalSources.hpp"


struct RequestModifyLink::Pim
{
	bool deleteRecord;
	unsigned caId;
	std::string srcName;
	std::vector<std::string> params;
	bool protein;
};


RequestModifyLink::~RequestModifyLink()
{
	stopProcessingThread();
	delete pim;
}


RequestModifyLink::RequestModifyLink(bool deleteRecord, std::string const & caId, std::string const & srcName, std::vector<std::string> params)
: Request(documentType::activeGenomicVariant), pim(new Pim)
{
	std::unique_ptr<Pim> pimGuard(pim);

	pim->deleteRecord = deleteRecord;
	pim->params = params;
	pim->srcName = srcName;

	// parse CA id
	if (caId.size() < 3 || (caId.substr(0,2) != "CA" && caId.substr(0,2) != "PA")) {
		throw ExceptionIncorrectRequest("Canonical allele id must consist of prefix 'CA' or 'PA' and decimal number. Given value: '" + caId + "'.");
	}
	for (unsigned i = 2; i < caId.size(); ++i) if (caId[i] < '0' || caId[i] > '9') {
		throw ExceptionIncorrectRequest("Canonical allele id must consist of prefix 'CA' or 'PA' and decimal number. Given value: '" + caId + "'.");
	}
	pim->protein = (caId[0] == 'P');
	pim->caId = boost::lexical_cast<uint32_t>(caId.substr(2));
	pimGuard.release();
}


void RequestModifyLink::process()
{
	if ( ! externalSources::authorization(logLogin,pim->srcName) ) {
		throw ExceptionAuthorizationError("You do not have privileges to modify links in the external source '" + pim->srcName + "'");
	}
	if (pim->deleteRecord) {
		externalSources::deleteLinks( {pim->caId}, pim->srcName, {pim->params} );
	} else {
		externalSources::registerLinks( {pim->caId}, pim->srcName, {pim->params} );
	}

	std::vector<Document> documents;
	if (pim->protein) {
		documents.push_back(DocumentActiveProteinVariant());
		documents[0].asActiveProteinVariant().caId = CanonicalId(pim->caId);
	} else {
		documents.push_back(DocumentActiveGenomicVariant());
		documents[0].asActiveGenomicVariant().caId = CanonicalId(pim->caId);
	}
	allelesDb->fetchVariantsByCaPaIds(documents);
	allelesDb->fetchVariantsByDefinition(documents);
	fillVariantsDetails(documents);
	setResponse(documents[0]);
}

