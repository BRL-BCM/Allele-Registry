#include "variants.hpp"
#include <boost/lexical_cast.hpp>


std::string PlainSequenceModification::toString() const
{
	std::string s = "(";
	s += region.toString();
	s += "," + originalSequence;
	s += "," + newSequence;
	s += ")";
	return s;
}


std::string NormalizedSequenceModification::toString() const
{
	std::string s = "(";
	s += region.toString();
	s += "," + boost::lexical_cast<std::string>(static_cast<unsigned>(category));
	s += "," + boost::lexical_cast<std::string>(lengthChange);
	s += "," + insertedSequence;
	s += ")";
	return s;
}


std::string NormalizedGenomicVariant::toString() const
{
	std::string s = "{";
	s += boost::lexical_cast<std::string>(refId.value);
	s += ",[";
	for (unsigned i = 0; i < modifications.size(); ++i) {
		if (i > 0) s += ",";
		s += modifications[i].toString();
	}
	s += "]}";
	return s;
}


std::string NormalizedProteinVariant::toString() const
{
	std::string s = "{";
	s += boost::lexical_cast<std::string>(refId.value);
	s += ",[";
	for (unsigned i = 0; i < modifications.size(); ++i) {
		if (i > 0) s += ",";
		s += modifications[i].toString();
	}
	s += "]}";
	return s;
}
