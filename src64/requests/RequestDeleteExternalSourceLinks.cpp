#include "requests.hpp"
#include "../externalSources/externalSources.hpp"


struct RequestDeleteExternalSourceLinks::Pim
{
	std::string srcName;
};


RequestDeleteExternalSourceLinks::~RequestDeleteExternalSourceLinks()
{
	stopProcessingThread();
	delete pim;
}


RequestDeleteExternalSourceLinks::RequestDeleteExternalSourceLinks(std::string const & srcName)
: Request(documentType::activeGenomicVariant), pim(new Pim)
{
	pim->srcName = srcName;
}


void RequestDeleteExternalSourceLinks::process()
{
	if ( ! externalSources::authorization(logLogin,pim->srcName) ) {
		throw ExceptionAuthorizationError("You do not have privileges to modify links in the external source '" + pim->srcName + "'");
	}
	std::string output = "{}";
	externalSources::deleteLinks(pim->srcName);
	setResponse(output);
}


