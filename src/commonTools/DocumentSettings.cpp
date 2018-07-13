#include "DocumentSettings.hpp"
#include <boost/algorithm/string.hpp>
#include <stdexcept>

namespace jsonBuilder
{

	DocumentSettings DocumentSettings::parse(std::string const & fields)
	{
		DocumentSettings out;
		// parse fields
		std::vector<std::string> vs;
		if ( fields.empty() || fields.front() == '-' || fields.front() == '+' ) vs.push_back("default");
		for ( auto it1 = fields.begin();  it1 != fields.end(); ) {
			auto it2 = it1;
			++it2;
			while ( it2 != fields.end() && *it2 != '-' && *it2 != '+' ) ++it2;
			vs.push_back( std::string(it1,it2) );
			it1 = it2;
		}
		// interpret fields
		if (vs.front() == "none") {
			out.layout = empty;
		} else if (vs.front() == "all") {
			out.layout = full;
		} else if (vs.front() == "default") {
			out.layout = standard;
		} else {
			//TODO
			throw std::runtime_error("The first word was '" + vs.front() + "', but one of the following was expected: 'none', 'default', 'all'.");
		}
		for (unsigned i = 1; i < vs.size(); ++i) {
			Modifier const modType = (vs[i].front() == '+') ? add : remove;
			std::string const pPath = vs[i].substr(1);
			std::vector<std::string> path;
			boost::split( path, pPath, boost::is_any_of(".") );
			if (pPath == "" || path.empty()) throw std::runtime_error("Unknown expression: " + vs[i]); //TODO
			out.modificators.push_back( std::make_pair(modType,path) );
		}
		return out;
	}

	DocumentSettings DocumentSettings::extractSubdocument(std::string const & section)
	{
		DocumentSettings subDoc;
		subDoc.layout = this->layout;
		std::vector<std::pair<Modifier,std::vector<std::string>>> newMods;
		bool subDocIsNonEmpty = false;
		for (auto const & mw: this->modificators) {
			if (mw.second.size() > 1 && mw.second.front() == section) {
				subDoc.modificators.resize(subDoc.modificators.size()+1);
				subDoc.modificators.back().first = mw.first;
				subDoc.modificators.back().second.assign(mw.second.begin()+1, mw.second.end());
				if (mw.first == add) subDocIsNonEmpty = true;
			} else {
				newMods.push_back(mw);
				if ( mw.second.front() == section ) {
					subDoc.modificators.clear();
					if (mw.first == add) {
						subDoc.layout = full;
						subDocIsNonEmpty = true;
					} else {
						subDoc.layout = empty;
						subDocIsNonEmpty = false;
					}
				}
			}
		}
		this->modificators.swap(newMods);

		// make sure that section is visible if not empty
		if (subDocIsNonEmpty) {
			this->modificators.push_back(std::make_pair(add,std::vector<std::string>(1,section)));
		}
		return subDoc;
	}

}
