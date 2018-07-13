#ifndef ALLELESDATABASE_TOOLS_HPP_
#define ALLELESDATABASE_TOOLS_HPP_

#include <cstdint>
#include <string>


// save the sequence on LSBits, in the order from MS to LS
uint32_t convertGenomicToBinary(std::string const & seq);

// read the sequence from the unsigned integer, sequence is saved on LSBits, in the order from MS to LS
std::string convertBinaryToGenomic(uint32_t value, unsigned length);

// returns number of bytes needed to save aa sequence
unsigned lengthOfBinaryAminoAcidSequence(unsigned lengthInAA);

// save the sequence on LSBits, in the order from MS to LS
uint32_t convertProteinToBinary(std::string const & seq);

// read the sequence from the unsigned integer, sequence is saved on LSBits, in the order from MS to LS
std::string convertBinaryToProtein(uint32_t value, unsigned length);

#endif /* ALLELESDATABASE_TOOLS_HPP_ */
