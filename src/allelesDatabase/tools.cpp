
#include "tools.hpp"
#include <stdexcept>

// =================== amino-acid codes

inline unsigned aminoAcid2number(char const aa)
{
	switch (aa) {
		case 'A': return  0;
		case 'C': return  1;
		case 'D': return  2;
		case 'E': return  3;
		case 'F': return  4;
		case 'G': return  5;
		case 'H': return  6;
		case 'I': return  7;
		case 'K': return  8;
		case 'L': return  9;
		case 'M': return 10;
		case 'N': return 11;
		case 'O': return 12;
		case 'P': return 13;
		case 'Q': return 14;
		case 'R': return 15;
		case 'S': return 16;
		case 'T': return 17;
		case 'U': return 18;
		case 'V': return 19;
		case 'W': return 20;
		case 'Y': return 21;
		case '*': return 22;
		default: throw std::logic_error(std::string("Unknown amino-acid code: ") + aa);
	}
}

inline char number2aminoAcid(unsigned const aa)
{
	switch (aa) {
		case  0: return 'A';
		case  1: return 'C';
		case  2: return 'D';
		case  3: return 'E';
		case  4: return 'F';
		case  5: return 'G';
		case  6: return 'H';
		case  7: return 'I';
		case  8: return 'K';
		case  9: return 'L';
		case 10: return 'M';
		case 11: return 'N';
		case 12: return 'O';
		case 13: return 'P';
		case 14: return 'Q';
		case 15: return 'R';
		case 16: return 'S';
		case 17: return 'T';
		case 18: return 'U';
		case 19: return 'V';
		case 20: return 'W';
		case 21: return 'Y';
		case 22: return '*';
		default: throw std::logic_error("Unknown amino-acid numeric code");
	}
}

// save the sequence on LSBits, in the order from MS to LS
uint32_t convertGenomicToBinary(std::string const & seq)
{
	uint32_t value = 0;
	if (seq.size() > 16) throw std::logic_error("Sequence larger than 16 bp");
	for (char c: seq) {
		value <<= 2;
		switch (c) {
			case 'A': value += 0; break;
			case 'C': value += 1; break;
			case 'G': value += 2; break;
			case 'T': value += 3; break;
			default: throw std::logic_error("Incorrect genomic sequence: " + seq);
		}
	}
	return value;
}

// read the sequence from the unsigned integer, sequence is saved on LSBits, in the order from MS to LS
std::string convertBinaryToGenomic(uint32_t value, unsigned length)
{
	if (length > 16) throw std::logic_error("Sequence larger than 16 bp");
	std::string seq(length, 'A');
	while (length > 0) {
		--length;
		switch (value % 4) {
			case 0: seq[length] = 'A'; break;
			case 1: seq[length] = 'C'; break;
			case 2: seq[length] = 'G'; break;
			case 3: seq[length] = 'T'; break;
		}
		value >>= 2;
	}
	return seq;
}


// returns number of bytes needed to save aa sequence
unsigned lengthOfBinaryAminoAcidSequence(unsigned lengthInAA)
{
	unsigned r = (lengthInAA / 7) * 4;
	switch (lengthInAA % 7) {
		case 0: break;
		case 1: ++r; break;
		case 2:
		case 3: r+=2; break;
		case 4:
		case 5: r+=3; break;
		case 6: r+=4; break;
	}
	return r;
}

// save the sequence on LSBits, in the order from MS to LS
uint32_t convertProteinToBinary(std::string const & seq)
{
	uint32_t value = 0;
	if (seq.size() > 7) throw std::logic_error("Sequence larger than 7 aa");
	for (char c: seq) {
		value *= 23;
		value += aminoAcid2number(c);
	}
	return value;
}

// read the sequence from the unsigned integer, sequence is saved on LSBits, in the order from MS to LS
std::string convertBinaryToProtein(uint32_t value, unsigned length)
{
	if (length > 7) throw std::logic_error("Sequence larger than 7 aa");
	std::string seq(length, 'A');
	while (length > 0) {
		seq[--length] = number2aminoAcid(value%23);
		value /= 23;
	}
	return seq;
}

