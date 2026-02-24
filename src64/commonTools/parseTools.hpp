#ifndef COMMONTOOLS_PARSETOOLS_HPP_
#define COMMONTOOLS_PARSETOOLS_HPP_

#include <stdexcept>
#include <string>

inline std::string butTheCharacterWasFound(char const * ptr)
{
	if (*ptr == '\0') return " but the end of expression was reached";
	char buf[2];
	buf[0] = *ptr;
	buf[1] = '\0';
	return (" but the character '" + std::string(buf) + "' was found");
}

inline void parseNucleotideSequence(char const *& ptr, std::string & out)
{
	out = "";
	for ( ;  *ptr == 'A' || *ptr == 'C' || *ptr == 'G' || *ptr == 'T';  ++ptr ) out += std::string(ptr,ptr+1);
}


inline bool tryParse(char const *& ptr, unsigned & number)
{
	if (*ptr < '0' || *ptr > '9') return false;
	number = 0;
	do {
		number *= 10;
		number += (*ptr - '0');
		++ptr;
	} while (*ptr >= '0' && *ptr <= '9');
	return true;
}
inline void parse(char const *& ptr, unsigned & number)
{
	if (*ptr < '0' || *ptr > '9') {
		throw std::runtime_error("Integer was expected" + butTheCharacterWasFound(ptr));
	}
	number = 0;
	do {
		number *= 10;
		number += (*ptr - '0');
		++ptr;
	} while (*ptr >= '0' && *ptr <= '9');
}


inline bool tryParse(char const *& ptr, std::string const & val)
{
	for (unsigned i = 0; i < val.size(); ++i) {
		if (val[i] != ptr[i]) return false;
	}
	ptr += val.size();
	return true;
}
inline void parse(char const *& ptr, std::string const & val)
{
	for (unsigned i = 0; i < val.size(); ++i) {
		if (val[i] != *ptr) throw std::runtime_error("The character '" + val.substr(i,1) + "' was expected " + butTheCharacterWasFound(ptr));
		++ptr;
	}
}


#endif /* COMMONTOOLS_PARSETOOLS_HPP_ */
