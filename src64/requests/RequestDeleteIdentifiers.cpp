#include "requests.hpp"


struct RequestDeleteIdentifiers::Pim
{
	std::string name;
	uint64_t from;
	uint64_t to;
};


RequestDeleteIdentifiers::~RequestDeleteIdentifiers()
{
	stopProcessingThread();
	delete pim;
}


RequestDeleteIdentifiers::RequestDeleteIdentifiers(std::string const & name, uint64_t from, uint64_t to)
: Request(documentType::activeGenomicVariant), pim(new Pim)
{
	pim->name = name;
	pim->from = from;
	pim->to = to;
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
		allelesDb->deleteIdentifiers(idType, pim->from, pim->to);
	}

	setResponse("{}");
}

