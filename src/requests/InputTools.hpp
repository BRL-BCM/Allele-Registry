#ifndef INPUTTOOLS_HPP_
#define INPUTTOOLS_HPP_

#include "../core/globals.hpp"
#include "../core/identifiers.hpp"
#include <stdexcept>
#include <boost/algorithm/string.hpp>


inline uint32_t parseUInt32(std::string const & id, std::string const & prefixOfErrorMessage = "")
{
	uint32_t val = 0;
	uint32_t const maxPrefix    = std::numeric_limits<uint32_t>::max() / 10;
	uint32_t const maxLastDigit = std::numeric_limits<uint32_t>::max() % 10;
	for (char c: id) {
		if (c < '0' || c > '9') throw std::runtime_error(prefixOfErrorMessage + "Number must consist of digits only.");
		uint32_t const digit = (c-'0');
		if ( maxPrefix < val || (maxPrefix == val && maxLastDigit <= digit) ) { // last value is reserved for null-like states
			throw std::runtime_error(prefixOfErrorMessage + "Number must be smaller than " + boost::lexical_cast<std::string>(std::numeric_limits<uint32_t>::max()) + ".");
		}
		val *= 10;
		val += digit;
	}
	return val;
}

inline CanonicalId parseCA(std::string const & caId)
{
	CanonicalId ca;
	if (caId.size() < 3 || caId.substr(0,2) != "CA") {
		throw std::runtime_error("Canonical allele id must consist of prefix 'CA' and decimal number. Given value: '" + caId + "'.");
	}
	ca.value = parseUInt32(caId.substr(2), "Canonical allele id must consist of prefix 'CA' and decimal number. Given value: '" + caId + "'. ");
	return ca;
}

inline CanonicalId parsePA(std::string const & paId)
{
	CanonicalId pa;
	if (paId.size() < 3 || paId.substr(0,2) != "PA") {
		throw std::runtime_error("Protein allele id must consist of prefix 'PA' and decimal number. Given value: '" + paId + "'.");
	}
	pa.value = parseUInt32(paId.substr(2), "Protein allele id must consist of prefix 'PA' and decimal number. Given value: '" + paId + "'. ");
	return pa;
}


inline uint32_t parseShortId(identifierType tIdType, std::string s)
{
	if (tIdType == identifierType::dbSNP && s.substr(0,2) == "rs") s = s.substr(2);
	return parseUInt32(s, "Cannot parse " + toString(tIdType) + " identifier. ");
}

inline std::vector<uint32_t> parseVectorOfUints(std::string const & s)
{
	std::vector<std::string> v;
	boost::algorithm::split(v, s, boost::is_any_of(","));
	std::vector<uint32_t> r;
	for (auto s: v) r.push_back( parseUInt32(s, "Cannot parse RCV number: " + s) );
	return r;
}

#endif /* INPUTTOOLS_HPP_ */
