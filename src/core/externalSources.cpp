#include "externalSources.hpp"


void ExternalSource::registerNew(unsigned id, std::string const & name, std::string const & urlPatternAPI, std::string const & urlPatternGUI)
{
	if (fData.size() <= id) fData.resize(id+1,nullptr);
	ASSERT(fData[id] == nullptr);
	ASSERT(fName2Id.count(name) == 0);
	fData[id] = new ExternalSource(id,name,urlPatternAPI,urlPatternGUI);
	fName2Id[name] = id;
}
