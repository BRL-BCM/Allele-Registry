#include "requests.hpp"
#include "../externalSources/externalSources.hpp"


struct RequestModifyExternalSource::Pim
{
	unsigned fieldToModify;
	std::string srcName;
	std::string newValue;
};


RequestModifyExternalSource::~RequestModifyExternalSource()
{
	stopProcessingThread();
	delete pim;
}


RequestModifyExternalSource::RequestModifyExternalSource(std::string const & srcName, unsigned fieldToModify, std::string const & newValue)
: Request(documentType::activeGenomicVariant), pim(new Pim)
{
	pim->fieldToModify = fieldToModify;
	pim->srcName = srcName;
	pim->newValue = newValue;
}


void RequestModifyExternalSource::process()
{
	if (logLogin != "admin") {
		throw ExceptionAuthorizationError("You need to be admin to perform this operation");
	}
	std::string output;
	externalSources::modifySource(output,pim->srcName,pim->fieldToModify,pim->newValue);
	setResponse(output);
}

