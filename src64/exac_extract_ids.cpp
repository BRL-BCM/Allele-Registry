#include <iostream>
#include <string>
#include <boost/lexical_cast.hpp>
#include "core/globals.hpp"

inline std::string::iterator nextColumn(std::string::iterator i)
{
	while (*i != '\t') ++i;
	return (++i);
}

inline std::string toStr(std::string::iterator i)
{
	std::string::iterator j = i;
	while (*j != '\t') ++j;
	return std::string(i,j);
}

char buf[32*1024];

int main()
{
	unsigned count = 0;

	bool inHeader = true;

	while (true)
	{
		std::string line = "";
		std::getline(std::cin, line);
		if (line == "") break;
		if (inHeader) {
			inHeader = (line.substr(0,11) != "#CHROM\tPOS\t");
			continue;
		}

		std::string::iterator iChr = line.begin();
		std::string::iterator iPos = nextColumn(iChr);
		std::string::iterator iRef = nextColumn(nextColumn(iPos));
		std::string::iterator iAlt = nextColumn(iRef);

		std::string chr = toStr(iChr);
		unsigned pos = boost::lexical_cast<unsigned>(toStr(iPos));
		std::string ref = toStr(iRef);

		while (*iAlt != '\t') {
			std::string::iterator i = iAlt;
			while (*i != '\t' && *i != ',') ++i;
			std::string alt(iAlt,i);
			iAlt = i;
			if (*iAlt == ',') ++iAlt;
			// process variant
			++count;
			std::cout << chr << "-" << pos << "-" << ref << "-" << alt << "\n";
		}
	}

	return 0;
}
