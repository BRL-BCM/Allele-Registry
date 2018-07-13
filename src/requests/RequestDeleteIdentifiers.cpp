#include "requests.hpp"


struct RequestDeleteIdentifiers::Pim
{
	std::string name;
};


RequestDeleteIdentifiers::~RequestDeleteIdentifiers()
{
	stopProcessingThread();
	delete pim;
}


RequestDeleteIdentifiers::RequestDeleteIdentifiers(std::string const & name)
: Request(documentType::activeGenomicVariant), pim(new Pim)
{
	pim->name = name;
}


void RequestDeleteIdentifiers::process()
{
	if (logLogin != "admin") {
		throw ExceptionAuthorizationError("You need to be admin to perform this operation");
	}

	std::vector<identifierType> idTypes;
	if (pim->name == "dbSNP") {
		idTypes.push_back(identifierType::dbSNP);
	} else if (pim->name == "ClinVar") {
		idTypes.push_back(identifierType::ClinVarAllele);
		idTypes.push_back(identifierType::ClinVarVariant);
		idTypes.push_back(identifierType::ClinVarRCV);
	} else {
		throw ExceptionIncorrectRequest("Unknown or unsupported externalRecords");
	}

	for (identifierType idType: idTypes) {
		allelesDb->deleteIdentifiers(idType);
	}

	setResponse("{}");
}

