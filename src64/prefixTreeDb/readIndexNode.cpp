#include <iostream>
#include <fstream>
#include <cstdlib>
#include <cstdint>
#include <limits>

#include <boost/crc.hpp>
#include <boost/thread.hpp>

inline unsigned CRC32(uint8_t const * buf, unsigned size)
{
	boost::crc_32_type result;
	result.process_bytes(buf, size);
	return result.checksum();
}

template<typename targetType = unsigned>
targetType readUnsignedInteger(uint8_t const *& ptr, unsigned bytesCount)
{
	targetType value = 0;
	for ( ;  bytesCount > 0;  --bytesCount ) {
		value *= 256;
		value += *ptr;
		++ptr;
	}
	return value;
}

void readPage(uint8_t const * ptr)
{
	unsigned crc32 = readUnsignedInteger(ptr,4);

	unsigned realCrc32 = CRC32(ptr, 3*512-4);

	unsigned indexNodeRevision = readUnsignedInteger<unsigned>(ptr,4);
	unsigned indexNodeLevel = readUnsignedInteger<unsigned>(ptr,4);
	uint64_t indexNodeLeftmostKey = readUnsignedInteger<uint64_t>(ptr,20);

	ptr += 256-32;

	unsigned levels[256];
//	unsigned leaves[256];
	for (unsigned i = 0; i < 256; ++i) levels[i] = readUnsignedInteger(ptr,1);
//	for (unsigned i = 0; i < 256; ++i) leaves[i] = readUnsignedInteger(ptr,4);
	unsigned const * leaves = reinterpret_cast<unsigned const*>(ptr);


	if (crc32 != realCrc32) {
		std::cout << "Incorrect CRC32!\n";
	} else {
		std::cout << "revision: " << indexNodeRevision << "\n";
		std::cout << "level   : " << indexNodeLevel << "\n";
		std::cout << "min key : " << indexNodeLeftmostKey << "\n";
		std::cout << "levels:\n";
		for (unsigned i = 0; i < 16; ++i) {
			for (unsigned j = 0; j < 16; ++j) std::cout << levels[i*16+j] << " ";
			std::cout << "\n";
		}
		std::cout << "leaves:\n";
		for (unsigned i = 0; i < 16; ++i) {
			for (unsigned j = 0; j < 16; ++j) {
				if (leaves[i*16+j] == std::numeric_limits<uint32_t>::max()) std::cout << "empty ";
				else std::cout << leaves[i*16+j] << " ";
			}
			std::cout << "\n";
		}
	}

	std::cout << "===================" << std::endl;
}


int main(int argc, char** argv)
{
	if (argc != 3) {
		std::cout << "Parameters: file_nume number_of_page" << std::endl;
		return 1;
	}

	unsigned pageId = atoi(argv[2]);

	std::ifstream file(argv[1]);

	file.seekg(pageId * 3 * 512);


	char buf[2*3*512];
	file.read(buf, 2 * 3 * 512);
	if (file.fail()) {
		std::cerr << "Cannot read 2 full pages!" << std::endl;
		return 2;
	}

	uint8_t const * ptr = reinterpret_cast<uint8_t*>(buf);

	readPage(ptr);
	readPage(ptr+3*512);

	return 0;
}
