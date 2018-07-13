#ifndef CORE_EXTERNALSOURCES_HPP_
#define CORE_EXTERNALSOURCES_HPP_

#include <unordered_map>
#include <vector>
#include <string>
#include "../commonTools/assert.hpp"


class ExternalSource {
private:
	static std::unordered_map<std::string,unsigned> fName2Id;
	static std::vector<ExternalSource*> fData;
	ExternalSource(unsigned pId, std::string const & pName, std::string const & pUrlPatternAPI, std::string const & pUrlPatternGUI)
	: id(pId), name(pName), urlPatternAPI(pUrlPatternAPI), urlPatternGUI(pUrlPatternGUI)
	{}
public:
	static void registerNew(unsigned id, std::string const & name, std::string const & urlPatternAPI, std::string const & urlPatternGUI);
	static inline ExternalSource const * getById(unsigned id)
	{
		ASSERT(id < fData.size() && fData[id] != nullptr);
		return fData[id];
	}
	static inline ExternalSource const * getByName(std::string const & name)
	{
		std::unordered_map<std::string,unsigned>::const_iterator it = fName2Id.find(name);
		ASSERT(it != fName2Id.end());
		return fData[it->second];
	}
	unsigned    id;
	std::string name;
	std::string urlPatternAPI;
	std::string urlPatternGUI;
};


#endif /* CORE_EXTERNALSOURCES_HPP_ */
