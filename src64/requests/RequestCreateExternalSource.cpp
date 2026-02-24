#include "requests.hpp"
#include "../externalSources/externalSources.hpp"

struct RequestCreateExternalSource::Pim
{
	std::string srcName;
	std::string url;
	std::string paramType;
	std::string guiName;
	std::string guiLabel;
	std::string guiUrl;
};

RequestCreateExternalSource::~RequestCreateExternalSource()
{
	stopProcessingThread();
	delete pim;
}


RequestCreateExternalSource::RequestCreateExternalSource
(std::string const & name, std::string const & url, std::string const & paramType, std::string const & guiName, std::string const & guiLabel, std::string const & guiUrl)
: Request(documentType::activeGenomicVariant), pim(new Pim)
{
	pim->guiLabel = guiLabel;
	pim->guiName = guiName;
	pim->guiUrl = guiUrl;
	pim->paramType = paramType;
	pim->srcName = name;
	pim->url = url;
}


void RequestCreateExternalSource::process()
{
	if (logLogin != "admin") {
		throw ExceptionAuthorizationError("You need to be admin to perform this operation");
	}
	std::string output;
	externalSources::createSource(output,pim->srcName,pim->url,pim->paramType,pim->guiName,pim->guiLabel,pim->guiUrl);
	setResponse(output);
}

