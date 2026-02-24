#include "requests.hpp"
#include "../externalSources/externalSources.hpp"


struct RequestModifyExternalSourceUser::Pim
{
	std::string srcName;
	std::string usrName;
	bool deleteRecord;
};


RequestModifyExternalSourceUser::~RequestModifyExternalSourceUser()
{
	stopProcessingThread();
	delete pim;
}


RequestModifyExternalSourceUser::RequestModifyExternalSourceUser(std::string const & srcName, std::string const & usrName, bool deleteRecord)
: Request(documentType::activeGenomicVariant), pim(new Pim)
{
	pim->deleteRecord = deleteRecord;
	pim->srcName = srcName;
	pim->usrName = usrName;
}


void RequestModifyExternalSourceUser::process()
{
	if (logLogin != "admin") {
		throw ExceptionAuthorizationError("You need to be admin to perform this operation");
	}
	std::string output;
	if (pim->deleteRecord) {
		externalSources::removeUserFromSource(output,pim->srcName,pim->usrName);
	} else {
		externalSources::assignUserToSource(output,pim->srcName,pim->usrName);
	}
	setResponse(output);
}

