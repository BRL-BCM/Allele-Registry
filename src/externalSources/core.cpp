#include "core.hpp"
#include <unordered_map>
#include <stdexcept>

namespace externalSources
{

	void label::fillWithLabels(std::array<std::string,256> & textLabels)
	{
		textLabels[label::_at_context] = "@context";
		textLabels[label::_at_id] = "@id";
		textLabels[label::guiLabel] = "guiLabel";
		textLabels[label::guiUrl] = "guiUrl";
	}

	void label::fillWithLabels(std::unordered_map<std::string,label> & text2id)
	{
		text2id["@context"] = label::_at_context;
		text2id["@id"] = label::_at_id;
		text2id["guiLabel"] = label::guiLabel;
		text2id["guiUrl"] = label::guiUrl;
	}

}
