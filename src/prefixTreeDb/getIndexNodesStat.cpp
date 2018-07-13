#include <iostream>
#include <fstream>
#include <cstdlib>
#include <cstdint>
#include <limits>
#include <vector>

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

struct IndexPage {
	unsigned crc32;
	unsigned revision;
	unsigned level;
	unsigned minKey;
	unsigned levels[256];
	unsigned pages[256];
};

bool readPage(uint8_t const * ptr, IndexPage & page)
{
	page.crc32 = readUnsignedInteger(ptr,4);

	unsigned realCrc32 = CRC32(ptr, 3*512-4);

	page.revision = readUnsignedInteger<unsigned>(ptr,4);
	page.level = readUnsignedInteger<unsigned>(ptr,4);
	page.minKey = readUnsignedInteger<unsigned>(ptr,4);

	for (unsigned i = 0; i < 256; ++i) page.levels[i] = readUnsignedInteger(ptr,1);
	unsigned const * leaves = reinterpret_cast<unsigned const*>(ptr);
	for (unsigned i = 0; i < 256; ++i) page.pages[i] = leaves[i];

	return (page.crc32 == realCrc32);
}


int main(int argc, char** argv)
{
	if (argc != 2) {
		std::cout << "Parameters: index_file" << std::endl;
		return 1;
	}

	std::ifstream file(argv[1]);

	unsigned correctNodes = 0;
	unsigned brokenNodesInTheMiddle = 0;
	unsigned brokenNodesAtTheEnd = 0;
	unsigned dataNodes = 0;
	std::vector<unsigned> byLevel(4, 0);

	char buf[2 * 3 * 512];
	while (true) {
		if (file.eof()) break;
		file.read(buf, 2 * 3 * 512);
		if (file.fail()) {
			std::cerr << "Error: Cannot read 2 full pages!" << std::endl;
			break;
		}
		uint8_t const * ptr = reinterpret_cast<uint8_t*>(buf);
		IndexPage page, page2;
		bool crc1 = readPage(ptr, page);
		bool crc2 = readPage(ptr+3*512, page2);
		if (! crc1) {
			page = page2;
			if (! crc2) {
				++brokenNodesAtTheEnd;
				continue;
			}
		} else if (crc2) {
			if (page2.revision > page.revision) page = page2;
		}
		++correctNodes;
		brokenNodesInTheMiddle += brokenNodesAtTheEnd;
		brokenNodesAtTheEnd = 0;
		++(byLevel.at(page.level));
		unsigned lastDataNodesId = std::numeric_limits<uint32_t>::max();
		for (unsigned i = 0; i < 256; ++i) {
			if (page.pages [i] == std::numeric_limits<uint32_t>::max()) continue;
			if (page.levels[i] == 8) continue;
			if (page.pages [i] == lastDataNodesId) continue;
			lastDataNodesId = page.pages[i];
			++dataNodes;
		}
	}

	std::cout << "Correct nodes: " << correctNodes << std::endl;
	std::cout << "Broken nodes in the middle: " << brokenNodesInTheMiddle << std::endl;
	std::cout << "Broken nodes at the end: " << brokenNodesAtTheEnd << std::endl;
	std::cout << "Data nodes: " << dataNodes << std::endl;
	std::cout << "Nodes by Level:\n";
	for (unsigned i = 0; i < byLevel.size(); ++i) std::cout << " " << i << " -> " << byLevel[i] << std::endl;

	return 0;
}
