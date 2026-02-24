#include "requests.hpp"
#include "../externalSources/externalSources.hpp"


struct RequestQueryExternalSources::Pim
{
	std::string srcName;
};


RequestQueryExternalSources::~RequestQueryExternalSources()
{
	stopProcessingThread();
	delete pim;
}


RequestQueryExternalSources::RequestQueryExternalSources(std::string srcName)
: Request(documentType::activeGenomicVariant), pim(new Pim)
{
	pim->srcName = srcName;
}


void RequestQueryExternalSources::process()
{
	std::string output;
	externalSources::querySources(output,pim->srcName);
	setResponse(output);
}

