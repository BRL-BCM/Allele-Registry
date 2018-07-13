#include <iostream>
#include <fstream>
#include <cstdlib>
#include <cstdint>
#include <limits>
#include <vector>
#include "../commonTools/bytesLevel.hpp"

unsigned const dataPageSize  =    256*1024;
unsigned const indexPageSize = 2*1024*1024;


struct IndexNodeEntry
{
	unsigned long long firstKey;
	unsigned long long lastKey;
	unsigned recordsCount;
	unsigned bytesCount;
	unsigned pageId;
};

struct IndexNode
{
	std::vector<IndexNodeEntry> entries;
	unsigned revision;
};


IndexNode readPage(uint8_t const * ptr, unsigned const keySize)
{
	unsigned crc32 = readUnsignedInteger<unsigned>(ptr,4);
	unsigned realCrc32 = CRC32(ptr, indexPageSize-4);
	IndexNode indexNode;
	indexNode.revision = readUnsignedInteger<unsigned>(ptr,4);

	if (crc32 != realCrc32) {
		std::cout << "Incorrect CRC32!\n";
	} else {
		std::cout << "revision: " << indexNode.revision << "\n";
		unsigned const maxRecordNumber = (indexPageSize - 8) / (2*keySize+7);
		for (unsigned i = 0; i < maxRecordNumber; ++i) {
			IndexNodeEntry e;
			e.firstKey = readUnsignedInteger<unsigned long long>(ptr,keySize);
			e.lastKey = readUnsignedInteger<unsigned long long>(ptr,keySize);
			unsigned long long recordsCount_bytesCount_pageId = readUnsignedInteger<7,unsigned long long>(ptr);
			e.pageId = recordsCount_bytesCount_pageId % (1u << 18);
			recordsCount_bytesCount_pageId >>= 18;
			e.bytesCount = recordsCount_bytesCount_pageId % (1u << 19);
			recordsCount_bytesCount_pageId >>= 19;
			e.recordsCount = recordsCount_bytesCount_pageId;
			if (e.pageId == 0) break;
			indexNode.entries.push_back(e);
			std::cout << "bin\t" << i << "\t[" << e.firstKey << "," << e.lastKey << "]\t" << e.recordsCount << "\t" << e.bytesCount << "\tpageId=" << e.pageId << "\n";
		}
	}

	std::cout << "===================" << std::endl;
	return indexNode;
}


unsigned neededBits(unsigned long long value)
{
	unsigned r = 1;
	for ( value >>= 1;  value;  value >>= 1 ) ++r;
	return r;
}


int main(int argc, char** argv)
{
	if (argc != 3 && argc != 4) {
		std::cout << "Parameters: file, keySize[, bin number]" << std::endl;
		return 1;
	}

	std::ifstream file(argv[1]);
	unsigned keySize = atoi(argv[2]);
	unsigned binIndex = 123456789;
	if (argc > 3) binIndex = atoi(argv[3]);

	char buf[2*indexPageSize];
	file.read(buf, 2 * indexPageSize);
	if (file.fail()) {
		std::cerr << "Cannot read 2 full pages!" << std::endl;
		return 2;
	}

	uint8_t const * ptr = reinterpret_cast<uint8_t*>(buf);

	IndexNode in1 = readPage(ptr, keySize);
	IndexNode in2 = readPage(ptr+indexPageSize, keySize);
	if (in1.entries.empty()) std::swap(in1, in2);
	if ( ! in2.entries.empty() && in1.revision < in2.revision ) std::swap(in1, in2);

	if (binIndex < in1.entries.size()) {
		std::cout << "Data Node from bin " << binIndex << std::endl;
		IndexNodeEntry & e = in1.entries[binIndex];
		unsigned const bytesPerKey = (neededBits(e.lastKey - e.firstKey)-1) / 8 + 1;

		file.seekg(e.pageId * dataPageSize);
		file.read(buf, dataPageSize);
		ptr = reinterpret_cast<uint8_t*>(buf);

		for ( unsigned iR = 0;  iR < e.recordsCount;  ++iR ) {
			unsigned long long const key = e.firstKey + readUnsignedInteger<unsigned long long>(ptr, bytesPerKey);
			unsigned const recordSize = readUnsignedIntVarSize<1,1,unsigned>(ptr);
			std::cout << iR << "\t" << key << "\t";
			for (unsigned j = 0; j < recordSize; ++j) {
				if (j) std::cout << " ";
				std::cout << std::hex << static_cast<unsigned>(*ptr) << std::dec;
				++ptr;
			}
			std::cout << "\n";
		}

	}

	return 0;
}
