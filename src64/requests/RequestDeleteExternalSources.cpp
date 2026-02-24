#include "requests.hpp"
#include "../externalSources/externalSources.hpp"


struct RequestDeleteExternalSources::Pim
{
	std::string srcName;
};


RequestDeleteExternalSources::~RequestDeleteExternalSources()
{
	stopProcessingThread();
	delete pim;
}


RequestDeleteExternalSources::RequestDeleteExternalSources(std::string const & name)
: Request(documentType::activeGenomicVariant), pim(new Pim)
{
	pim->srcName = name;
}


void RequestDeleteExternalSources::process()
{
	if (logLogin != "admin") {
		throw ExceptionAuthorizationError("You need to be admin to perform this operation");
	}
	std::string output;
	externalSources::deleteSource(output,pim->srcName);
	setResponse(output);
}

