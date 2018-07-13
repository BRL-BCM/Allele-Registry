#include "../commonTools/parseTools.hpp"
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <cstring>
#include <boost/lexical_cast.hpp>

#include "globals.hpp"


enum idTypeMyVariantInfo
{
	  deletion    // del
	, delins      // delins or ins
	, duplication // dup
	, snp         // X>Y
};

struct IdentifierMyVariantInfo
{
	Chromosome chromosome = Chromosome::chrUnknown;
	uint32_t position = 0;
	uint16_t length = 0;
	idTypeMyVariantInfo type;
	std::string sequence = "";
	bool hasLength = false;
	std::string toString() const
	{
		std::string s = "chr" + ::toString(chromosome) + ":g." + boost::lexical_cast<std::string>(position);
		if (length > 1) s += "_" + boost::lexical_cast<std::string>(position + length - 1);
		else if (length == 0) s += "_" + boost::lexical_cast<std::string>(position + 1);
		switch (type) {
			case deletion   : s += "del"    + sequence; break;
			case delins     : s += ((length == 0) ? ("ins") : ("delins")) + sequence; break;
			case duplication: s += "dup"    + sequence; break;
			case snp        : s += sequence.substr(0,1) + ">" + sequence.substr(1); break;
		}
		if (hasLength) {
			s += boost::lexical_cast<std::string>(length);
		}
		return s;
	}
};


int main(int argc, char ** argv)
{
	char buf[1024];

	unsigned parsedCount = 0;
	unsigned failsCount = 0;

	while (true) {
		std::string s = "";
		std::getline(std::cin, s);
		if (s == "") break;
		if (s.size() > 1023) throw std::runtime_error("Expression too long: " + s);
		strncpy(buf, s.data(), s.size());
		buf[s.size()] = '\0';
		char const * ptr = buf;


		// parsing expression
		try {
			IdentifierMyVariantInfo id;
			parse(ptr, "chr");
			parse(ptr, id.chromosome);
			parse(ptr, ":g.");
			parse(ptr, id.position);
			if (tryParse(ptr, "_")) {
				unsigned pos2;
				parse(ptr,pos2);
				if (pos2 <= id.position) throw std::runtime_error("Incorrect variation region");
				if (pos2 - id.position > 10000) throw std::runtime_error("Variation's region larger than 10kbp");
				id.length = pos2 - id.position + 1;
			} else {
				id.length = 1;
			}
			if (tryParse(ptr, "delins")) {
				id.type = delins;
			} else if (tryParse(ptr, "del")) {
				id.type = deletion;
			} else if (tryParse(ptr, "ins")) {
				if (id.length != 2) throw std::runtime_error("Incorrect variation region");
				id.length = 0;
				id.type = delins;
			} else if (tryParse(ptr, "dup")) {
				id.type = idTypeMyVariantInfo::duplication;
			} else if (id.length == 1 && (*ptr == 'A' || *ptr == 'C' || *ptr == 'G' || *ptr == 'T') ) {
				id.type = snp;
				id.sequence = std::string(1, *ptr); ++ptr;
				parse(ptr, ">");
				if (*ptr == 'A' || *ptr == 'C' || *ptr == 'G' || *ptr == 'T') {
					id.sequence += std::string(1, *ptr); ++ptr;
				} else {
					throw std::runtime_error("Incorrect SNP definition");
				}
			} else {
				throw std::runtime_error("Unknown expression type");
			}
			if (id.type == delins) {
				parseNucleotideSequence(ptr, id.sequence);
				if (id.sequence == "") throw std::runtime_error("Insertion without sequence");
			} else if (id.type == idTypeMyVariantInfo::duplication || id.type == deletion) {
				unsigned length;
				if ( tryParse(ptr, length) ) {
					if (length != id.length) throw std::runtime_error("Parsed length do not correspond to variant's region's length");
					id.hasLength = true;
				} else {
					parseNucleotideSequence(ptr, id.sequence);
					if (id.sequence != "" && id.sequence.size() != id.length) {
						throw std::runtime_error("Parsed original sequence do not correspond to variant's region's length");
					}
				}
			}
			if (*ptr != '\0') throw std::runtime_error("Unparsed suffix '" + std::string(ptr) + "'");
			if (id.toString() != s) throw std::runtime_error("Generated id different than original expression: " + id.toString());
			++parsedCount;
		} catch (std::runtime_error const & e) {
			std::cerr << "Cannot parse\t" << s << "\t: " << e.what() << "\n";
			++failsCount;
		}
	}

	std::cout << "all: " << (parsedCount + failsCount) << std::endl;
	std::cout << "parsed: " << parsedCount << std::endl;
	std::cout << "failures: " << failsCount << std::endl;

	return 0;
}
