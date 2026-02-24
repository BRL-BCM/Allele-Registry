#ifndef CONVERT_HPP
#define CONVERT_HPP

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <fcntl.h>
#include <iostream>
#include <iomanip>
#include <vector>
#include <set>
#include <map>
#include <sstream>
#include "Bin.hpp"
#include "Scheduler.hpp"
#include "DataNode.hpp"
#include "../commonTools/Header.hpp"
#include "../commonTools/parseHeader.h"
#include "../commonTools/bytesLevel.hpp"
#include "../commonTools/assert.hpp"
#include "../apiDb/TasksManager.hpp"

namespace flatDb {

	template< typename T >
	std::string int_to_hex( T i )
	{
	  std::stringstream stream;
	  stream << "0x" 
	         << std::setfill ('0') << std::setw(sizeof(T)*2) 
	         << std::hex << i;
	  return stream.str();
	}

	template <unsigned revision, typename key> class Version;

	template <typename key> 
	class Version<0, key> 
	{
	public:

		void load ( typename flatDb::SchedulerT<key> * scheduler // scheduler
							, std::vector<BinT<key> > const & indexEntries // complete set of index node entries
							, std::set<uint64_t> const & loadSet // page ids to load
							, std::map<uint64_t, typename DataNodeT<key>::SP> & dataEntries
							)
		{
			for(auto bin : indexEntries) {
				if(loadSet.find(bin.pageId) != loadSet.end()) {
					dataEntries[bin.pageId] = DataNodeT<key>::createFromStorage(scheduler,bin.pageId,bin);
				}
			}
		}

		void read(uint8_t const * inPtr, Header const & inHeader, std::vector<BinT<key> > & entries) {
			typedef BinT<key> inBin;
			uint64_t inIndexPageSize = inHeader.indexNodePages * inHeader.pageSize * 1024L;
			unsigned const maxNumberOfDataNodes = (inIndexPageSize - 8) / (2 * inHeader.keySize + 7);
			uint8_t const * p = reinterpret_cast<uint8_t const *>(inPtr);
			// parse
			unsigned const crc32 = readUnsignedInteger<4,unsigned>(p);
			if ( crc32 != CRC32(p,inIndexPageSize-4)) {
				//throw std::runtime_error("Incorrect CRC32 for index page");
			}
			unsigned const revision = readUnsignedInteger<4,unsigned>(p);
			for (unsigned i = 0; i < maxNumberOfDataNodes; ++i) {
				inBin bin;
				bin.firstKey = readUnsignedInteger<key>(p, inHeader.keySize);
				bin.maxKeyOffset = readUnsignedInteger<key>(p, inHeader.keySize) - bin.firstKey;
				uint64_t recordsCount_bytesCount_pageId = readUnsignedInteger<7,uint64_t>(p);
				unsigned const pageId = recordsCount_bytesCount_pageId % (1u << 18);
				if (pageId == 0) break;
				bin.pageId = pageId;
				recordsCount_bytesCount_pageId >>= 18;
				bin.bytesCount = recordsCount_bytesCount_pageId % (1u << 19);
				recordsCount_bytesCount_pageId >>= 19;
				bin.recordsCount = recordsCount_bytesCount_pageId;
				entries.push_back( bin );
			}
		}

		void write(uint8_t * outPtr, Header const & outHeader, std::vector<BinT<key> > & entries) {
			uint64_t outIndexPageSize = outHeader.indexNodePages * outHeader.pageSize * 1024L;
			unsigned const maxNumberOfDataNodes = (outIndexPageSize - 8) / (2 * outHeader.keySize + 7);
			// write page but CRC
			uint8_t * pBegin = reinterpret_cast<uint8_t*>(outPtr);
			uint8_t * p = pBegin + 4;
			writeUnsignedInteger<4>(p, outHeader.revision);
			for (auto bin: entries) {
				writeUnsignedInteger(p, bin.firstKey, outHeader.keySize);
				writeUnsignedInteger(p, bin.lastKey(), outHeader.keySize);
				uint64_t recordsCount_bytesCount_pageId = bin.recordsCount; // 19 bits
				recordsCount_bytesCount_pageId <<= 19;
				recordsCount_bytesCount_pageId += bin.bytesCount; // 19 bits
				recordsCount_bytesCount_pageId <<= 18;
				recordsCount_bytesCount_pageId += bin.pageId; // 18 bits
				writeUnsignedInteger<7>(p, recordsCount_bytesCount_pageId);
			}
			std::memset(p, 0, pBegin + outIndexPageSize - p);

			// write CRC32
			unsigned const crc32 = CRC32( pBegin+4, outIndexPageSize-4 );
			writeUnsignedInteger<4>(pBegin, crc32);
		}

	};

	template <typename key> 
	class Version<1, key> 
	{
	public:
		void load ( typename flatDb::SchedulerT<key> * scheduler // scheduler
							, std::vector<BinT<key> > const & indexEntries // complete set of index node entries
							, std::set<uint64_t> const & loadSet // page ids to load
							, std::map<uint64_t, typename DataNodeT<key>::SP> & dataEntries
							)
		{
			for(auto bin : indexEntries) {
				if(loadSet.find(bin.pageId) != loadSet.end()) {
					dataEntries[bin.pageId] = DataNodeT<key>::createFromStorage(scheduler,bin.pageId,bin);
				}
			}
		}

		void read(uint8_t const * inPtr, Header const & inHeader, std::vector<BinT<key> > & entries) {
			typedef BinT<key> inBin;
			uint64_t inIndexPageSize = inHeader.indexNodePages * inHeader.pageSize * 1024L;
			unsigned const maxNumberOfDataNodes = (inIndexPageSize - 8) / (2 * inHeader.keySize + 3 * 4);
			uint8_t const * p = reinterpret_cast<uint8_t const *>(inPtr);
			// parse
			unsigned const crc32 = readUnsignedInteger<4,unsigned>(p);
			if ( crc32 != CRC32(p,inIndexPageSize-4)) {
				//throw std::runtime_error("Incorrect CRC32 for index page");
			}
			unsigned const revision = readUnsignedInteger<4,unsigned>(p);
			uint64_t count = 0;
			uint64_t runningLast = 0;
			std::cout << "read index file" << std::endl;
			for (unsigned i = 0; i < maxNumberOfDataNodes; ++i) {
				inBin bin;
				bin.firstKey = readUnsignedInteger<key>(p, inHeader.keySize);
				bin.maxKeyOffset = readUnsignedInteger<key>(p, inHeader.keySize) - bin.firstKey;
				unsigned const pageId = readUnsignedInteger<unsigned>(p, 4);
				if (pageId == 0) break;
				bin.pageId = pageId;
				bin.bytesCount = readUnsignedInteger<unsigned>(p, 4);
				bin.recordsCount = readUnsignedInteger<unsigned>(p, 4);
				if(runningLast > 0 && runningLast >= bin.firstKey) {
					std::cout << "ERROR: " << runningLast << ">=" << bin.firstKey << std::endl;
					entries.back().limitKey = bin.firstKey;
				}
				std::cout << count << "\t" << bin.firstKey << "\t" << bin.lastKey() << "\t" << inHeader.keySize << "\t" << bin.pageId << std::endl;
				std::cout << count << "\t" << int_to_hex<uint64_t>(bin.firstKey) << "\t" << int_to_hex<uint64_t>(bin.lastKey()) << "\t" << inHeader.keySize << "\t" << int_to_hex<uint32_t>(bin.pageId) << "\t" << int_to_hex<uint32_t>(bin.bytesCount) << "\t" << int_to_hex<uint32_t>(bin.recordsCount) << std::endl;
				count++;
				runningLast = bin.lastKey();
				entries.push_back( bin );
			}
		}

		void write(uint8_t * outPtr, Header const & outHeader, std::vector<BinT<key> > & entries) {
			uint64_t outIndexPageSize = outHeader.indexNodePages * outHeader.pageSize * 1024L;
			unsigned const maxNumberOfDataNodes = (outIndexPageSize - 8) / (2 * outHeader.keySize + 3 * 4);
			// write page but CRC
			uint8_t * pBegin = reinterpret_cast<uint8_t*>(outPtr);
			uint8_t * p = pBegin + 4;
			writeUnsignedInteger<4>(p, outHeader.revision);
			std::cout << "write" << std::endl;
			std::cout << "revision:" << outHeader.revision << "\t" << int_to_hex<uint32_t>(outHeader.revision) << std::endl;
			uint64_t count = 0;
			for (auto bin: entries) {
				writeUnsignedInteger(p, bin.firstKey, outHeader.keySize);
				writeUnsignedInteger(p, bin.lastKey(), outHeader.keySize);
				std::cout << count << "\t" << bin.firstKey << "\t" << bin.lastKey() << "\t" << outHeader.keySize << "\t" << bin.pageId << std::endl;
				std::cout << count << "\t" << int_to_hex<uint64_t>(bin.firstKey) << "\t" << int_to_hex<uint64_t>(bin.lastKey()) << "\t" << outHeader.keySize << "\t" << int_to_hex<uint32_t>(bin.pageId) << "\t" << int_to_hex<uint32_t>(bin.bytesCount) << "\t" << int_to_hex<uint32_t>(bin.recordsCount) << std::endl;
				count++;
				writeUnsignedInteger(p, bin.pageId, 4); // 4 bytes
				writeUnsignedInteger(p, bin.bytesCount, 4); // 4 bytes
				writeUnsignedInteger(p, bin.recordsCount, 4); // 4 bytes
			}
			std::memset(p, 0, pBegin + outIndexPageSize - p);

			// write CRC32
			unsigned const crc32 = CRC32( pBegin+4, outIndexPageSize-4 );
			writeUnsignedInteger<4>(pBegin, crc32);
		}

	};

	template <typename key> 
	class Version<2, key> 
	{
	public:
		void load ( typename flatDb::SchedulerT<key> * scheduler // scheduler
							, std::vector<BinT<key> > const & indexEntries // complete set of index node entries
							, std::set<uint64_t> const & loadSet // page ids to load
							, std::map<uint64_t, typename DataNodeT<key>::SP> & dataEntries
							)
		{
			for(auto bin : indexEntries) {
				if(loadSet.find(bin.pageId) != loadSet.end()) {
					dataEntries[bin.pageId] = DataNodeT<key>::createFromStorage(scheduler,bin.pageId,bin);
				}
			}
		}

		void read(uint8_t const * inPtr, Header const & inHeader, std::vector<BinT<key> > & entries) {
			typedef BinT<key> inBin;
			uint64_t inIndexPageSize = inHeader.indexNodePages * inHeader.pageSize * 1024L;
			unsigned const maxNumberOfDataNodes = (inIndexPageSize - 8) / (2 * inHeader.keySize + 3 * 8);
			uint8_t const * p = reinterpret_cast<uint8_t const *>(inPtr);
			// parse
			unsigned const crc32 = readUnsignedInteger<4,unsigned>(p);
			if ( crc32 != CRC32(p,inIndexPageSize-4)) {
				//throw std::runtime_error("Incorrect CRC32 for index page");
			}
			unsigned const revision = readUnsignedInteger<4,unsigned>(p);
			for (unsigned i = 0; i < maxNumberOfDataNodes; ++i) {
				inBin bin;
				bin.firstKey = readUnsignedInteger<key>(p, inHeader.keySize);
				bin.maxKeyOffset = readUnsignedInteger<key>(p, inHeader.keySize) - bin.firstKey;
				unsigned const pageId = readUnsignedInteger<uint64_t>(p, 8);
				if (pageId == 0) break;
				bin.pageId = pageId;
				bin.bytesCount = readUnsignedInteger<uint64_t>(p, 8);
				bin.recordsCount = readUnsignedInteger<uint64_t>(p, 8);
				entries.push_back( bin );
			}
		}

		void write(uint8_t * outPtr, Header const & outHeader, std::vector<BinT<key> > & entries) {
			uint64_t outIndexPageSize = outHeader.indexNodePages * outHeader.pageSize * 1024L;
			unsigned const maxNumberOfDataNodes = (outIndexPageSize - 8) / (2 * outHeader.keySize + 3 * 8);
			// write page but CRC
			uint8_t * pBegin = reinterpret_cast<uint8_t*>(outPtr);
			uint8_t * p = pBegin + 4;
			writeUnsignedInteger<4>(p, outHeader.revision);
			for (auto bin: entries) {
				writeUnsignedInteger(p, bin.firstKey, outHeader.keySize);
				writeUnsignedInteger(p, bin.lastKey(), outHeader.keySize);
				writeUnsignedInteger(p, bin.pageId, 8); // 8 bytes
				writeUnsignedInteger(p, bin.bytesCount, 8); // 8 bytes
				writeUnsignedInteger(p, bin.recordsCount, 8); // 8 bytes
			}
			std::memset(p, 0, pBegin + outIndexPageSize - p);

			// write CRC32
			unsigned const crc32 = CRC32( pBegin+4, outIndexPageSize-4 );
			writeUnsignedInteger<4>(pBegin, crc32);
		}

	};

	template<typename inKey, typename outKey>
	class Verify
	{
	public:
		typedef BinT<inKey> inBin;
		typedef BinT<outKey> outBin;
		void verify ( std::string const & in_header  // should be *.yaml format
								, std::string const & out_header // should be *.yaml format
								, std::string const & in_file    // read only *.index input index file
								, std::string const & out_file   // output *.index file
								, std::set<uint64_t> const & loadSet    // page ids to load
								, bool fullDiagnostic = false
								, bool fix = false
								) const
		{
			ASSERT(in_header != out_header); // dont overwrite input, write to separete file please
			ASSERT(in_file != out_file); // dont overwrite input, write to separate file please
			Header const inHeader(in_header);
			Header const outHeader(out_header);
			// check if key type matches what is specified in header
			ASSERT(inHeader.keySize == sizeof(inKey));
			ASSERT(outHeader.keySize == sizeof(outKey));
			int flags = O_RDWR | O_CREAT | O_NOATIME;
			int inIndexFile = 0;
			int inDataFile = 0;
			inIndexFile = open(in_file.c_str(), flags, S_IRUSR | S_IWUSR);
			inDataFile = open(in_header.c_str(), flags, S_IRUSR | S_IWUSR);
			// check if file exists and/or can be opened
			if (inIndexFile < 0) {
				throw std::runtime_error("Cannot open the index file: " + in_file);
			}
			if (inDataFile < 0) {
				throw std::runtime_error("Cannot open the data file: " + in_header);
			}
			struct stat buf;
			// check if file size can be checked
			if (fstat(inIndexFile,&buf) != 0) {
				throw std::runtime_error("Cannot check the size of the file. Error returned by fstat(): " + in_file);
			}
			off_t file_size = buf.st_size;
			// check if size is empty
			ASSERT(file_size > 0);
			std::cout << "input file size(" << in_file << "):" << file_size << std::endl;
			std::cout << "2x" << inHeader.indexNodePages << "*(" << inHeader.pageSize << "x1024)=" << (2L*inHeader.indexNodePages*inHeader.pageSize*1024L) << std::endl;
			if(file_size != 0) {
				ASSERT((2L * inHeader.indexNodePages * inHeader.pageSize * 1024L) == file_size);
			}

			int outIndexFile = 0;
			outIndexFile = open(out_file.c_str(), flags, S_IRUSR | S_IWUSR);
			if (outIndexFile < 0) {
				throw std::runtime_error("Cannot open the outdex file: " + out_file);
			}

			uint64_t inIndexPageSize = inHeader.indexNodePages * inHeader.pageSize * 1024L;
			uint64_t inDatSize = 2L * inIndexPageSize;
			uint8_t * inPtr = new uint8_t[inDatSize];
			uint8_t * inPtr2 = &inPtr[inIndexPageSize];
			memset(inPtr, 0, inDatSize);
			pread(inIndexFile, inPtr, inDatSize, 0);

			uint64_t outIndexPageSize = outHeader.indexNodePages * outHeader.pageSize * 1024L;
			uint64_t outDatSize = 2L * outIndexPageSize;
			uint8_t * outPtr = new uint8_t[outDatSize];
			uint8_t * outPtr2 = &outPtr[outIndexPageSize];
			memset(outPtr, 0, outDatSize);

			std::vector<inBin> indexEntries;
			std::vector<inBin> indexEntries2;

			TasksManager ioTaskManager (64);
			TasksManager cpuTaskManager (64);
			int globalKeySize = inHeader.keySize;
			int numPages = inHeader.indexNodePages;
			int cacheSizeInMegabytes = 1280/2;
			std::string dbFile = in_header;
			typename RecordT<inKey>::tCreateRecordFunction callbackCreateRecord;
			flatDb::StorageWithCache * storage = new flatDb::StorageWithCache(dbFile, 256*1024, cacheSizeInMegabytes);  // page size = 256 KB
			std::cout << "isNew:" << (storage->numberOfPages() == 0) << std::endl;
			typename flatDb::SchedulerT<inKey> * scheduler = new flatDb::SchedulerT<inKey>( globalKeySize, numPages, &cpuTaskManager, &ioTaskManager, storage, callbackCreateRecord ); // index page size = 2 MB
			std::map<uint64_t, typename DataNodeT<inKey>::SP> dataEntries; // data entries

			switch(inHeader.revision) {
				case 0: {
					Version<0, inKey> inVersion;
					inVersion.read(inPtr, inHeader, indexEntries);
					inVersion.read(inPtr2, inHeader, indexEntries2);
					inVersion.load(scheduler, indexEntries, loadSet, dataEntries);
					break;
				}
				case 1: {
					Version<1, inKey> inVersion;
					inVersion.read(inPtr, inHeader, indexEntries);
					inVersion.read(inPtr2, inHeader, indexEntries2);
					inVersion.load(scheduler, indexEntries, loadSet, dataEntries);
					break;
				}
				case 2: {
					Version<2, inKey> inVersion;
					inVersion.read(inPtr, inHeader, indexEntries);
					inVersion.read(inPtr2, inHeader, indexEntries2);
					inVersion.load(scheduler, indexEntries, loadSet, dataEntries);
					break;
				}
				default: {
					throw std::runtime_error("unsupported revision: " + inHeader.revision);
				}
			}

			uint64_t inPageSize = inHeader.pageSize * 1024L;
			uint8_t * pagePtr = new uint8_t[inPageSize * 2]; // 2X for error tolerance
			for(auto e : dataEntries) {
				std::cout << "pageSize: " << inPageSize << std::endl;
				memset(pagePtr, 0, 2*inPageSize);
				pread(inDataFile, pagePtr, inPageSize, e.second->pageId * inPageSize);
				e.second->diagnosticReadRawRecordsFromPage(pagePtr);
			}

			std::map<uint64_t, typename DataNodeT<inKey>::DataNodeStatus> statusMap;

			if(fullDiagnostic) {
				std::cout << "$$$$$$$$$$$$$$$$$ FULL DIAGNOSTIC $$$$$$$$$$$$$$$$$" << std::endl;
				uint64_t count = 0;
				for(auto bin : indexEntries) {
					count++;
					memset(pagePtr, 0, 2*inPageSize);
					pread(inDataFile, pagePtr, inPageSize,bin.pageId * inPageSize);
					typename DataNodeT<inKey>::SP dataNode = DataNodeT<inKey>::createFromStorage(scheduler,bin.pageId,bin);
					typename DataNodeT<inKey>::DataNodeStatus status = dataNode->diagnosticStatusReadRawRecordsFromPage(inPageSize, pagePtr);
					std::cout << (100.0*(float)count/indexEntries.size()) << "%\t" << status.pageId << "\t" << storage->numberOfPages() << "\t" << ((status.pageId < storage->numberOfPages())?"OK":"ERROR") << "\t" << status.sorted_records << "\t" << status.normal_record_sizes << "\t" << status.stays_within_page << "\t" << status.sane_initialization << "\t" << "res:" << (status.sorted_records && status.normal_record_sizes && status.stays_within_page && status.sane_initialization) << std::endl;
					if((status.sorted_records && status.normal_record_sizes && status.stays_within_page && status.sane_initialization) == false) {
						statusMap[bin.pageId] = status;
					}
				}
				std::cout << "$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$" << std::endl;
				std::cout << "***************************************************" << std::endl;
				std::map<uint64_t, uint64_t> pageIdCount;
				for(auto e : indexEntries) {
					if(pageIdCount.find(e.pageId) != pageIdCount.end()) {
						pageIdCount[e.pageId] ++;
					} else {
						pageIdCount[e.pageId] = 1;
					}
				}
				std::map<uint64_t, uint64_t> pageIdCount2;
				for(auto e : indexEntries2) {
					if(pageIdCount2.find(e.pageId) != pageIdCount2.end()) {
						pageIdCount2[e.pageId] ++;
					} else {
						pageIdCount2[e.pageId] = 1;
					}
				}
				for(auto e : pageIdCount) {
					if(e.second != 1) std::cout << "DUPLICATE" << std::endl;
					std::cout << "1\t" << e.first << "\t" << e.second << "\t" << storage->numberOfPages() << std::endl;
				}
				for(auto e : pageIdCount2) {
					if(e.second != 1) std::cout << "DUPLICATE" << std::endl;
					std::cout << "2\t" << e.first << "\t" << e.second << "\t" << storage->numberOfPages() << std::endl;
				}
				std::cout << "***************************************************" << std::endl;

			}

			if(fullDiagnostic && fix) {

				std::vector<outBin> outEntries;
				std::vector<outBin> outEntries2;


				int ind = 0;
				for(auto e : indexEntries) {
					outBin b;
					b.firstKey = e.firstKey;
					b.maxKeyOffset = e.maxKeyOffset;
					b.recordsCount = e.recordsCount;
					b.bytesCount = e.bytesCount;
					b.pageId = e.pageId;
					if(statusMap.find(b.pageId) != statusMap.end()) {
						b.maxKeyOffset = statusMap[b.pageId].maxKeyOffset;
						b.recordsCount = statusMap[b.pageId].recordsCount;
						b.bytesCount = statusMap[b.pageId].bytesCount;
					}
					outEntries.push_back(b);
					ind++;
				}
				ind = 0;
				for(auto e : indexEntries2) {
					outBin b;
					b.firstKey = e.firstKey;
					b.maxKeyOffset = e.maxKeyOffset;
					b.recordsCount = e.recordsCount;
					b.bytesCount = e.bytesCount;
					b.pageId = e.pageId;
					if(statusMap.find(b.pageId) != statusMap.end()) {
						b.maxKeyOffset = statusMap[b.pageId].maxKeyOffset;
						b.recordsCount = statusMap[b.pageId].recordsCount;
						b.bytesCount = statusMap[b.pageId].bytesCount;
					}
					outEntries2.push_back(b);
					ind++;
				}

				switch(outHeader.revision) {
					case 0: {
						Version<0, outKey> outVersion;
						outVersion.write(outPtr, outHeader, outEntries);
						outVersion.write(outPtr2, outHeader, outEntries2);
						break;
					}
					case 1: {
						Version<1, outKey> outVersion;
						outVersion.write(outPtr, outHeader, outEntries);
						outVersion.write(outPtr2, outHeader, outEntries2);
						break;
					}
					case 2: {
						Version<2, outKey> outVersion;
						outVersion.write(outPtr, outHeader, outEntries);
						outVersion.write(outPtr2, outHeader, outEntries2);
						break;
					}
					default: {
						throw std::runtime_error("unsupported revision: " + outHeader.revision);
					}
				}

				std::cout << "entries:" << outEntries.size() << std::endl;
				uint64_t max_pageId = 0;
				for(auto e : outEntries) {
					max_pageId = (max_pageId > e.pageId) ? max_pageId : e.pageId;
				}
				std::cout << "entries max pageId:" << max_pageId << std::endl;

				std::cout << "entries2:" << outEntries2.size() << std::endl;
				uint64_t max_pageId2 = 0;
				for(auto e : outEntries2) {
					max_pageId2 = (max_pageId2 > e.pageId) ? max_pageId2 : e.pageId;
				}
				std::cout << "entries2 max pageId:" << max_pageId2 << std::endl;

				pwrite(outIndexFile, outPtr, outDatSize, 0);
				std::cout << "output file : " << out_file << std::endl;
				std::cout << "2x" << outHeader.indexNodePages << "*(" << outHeader.pageSize << "x1024)=" << (2L*outHeader.indexNodePages*outHeader.pageSize*1024L) << std::endl;
				std::cout << "check file size" << std::endl;
			}
			delete [] inPtr;
			delete [] outPtr;
			delete [] pagePtr;

			close(inIndexFile);
			close(inDataFile);
			close(outIndexFile);

		}
	};

	template<typename inKey, typename outKey>
	class Convert
	{
	public:
		typedef BinT<inKey> inBin;
		typedef BinT<outKey> outBin;
		void convertIndexNode (	  std::string const & in_header 	// should be *.yaml format
					, std::string const & out_header	// should be *.yaml format
					, std::string const & in_file   	// read only input file
					, std::string const & out_file  	// write output file
					) const 
		{
			ASSERT(in_header != out_header); // dont overwrite input, write to separete file please
			ASSERT(in_file != out_file); // dont overwrite input, write to separate file please
			Header const inHeader(parse(in_header+".yaml"));
			Header const outHeader(parse(out_header+".yaml"));

			ASSERT(inHeader.keySize == sizeof(inKey));
			ASSERT(outHeader.keySize == sizeof(outKey));

			int flags = O_RDWR | O_CREAT | O_NOATIME;
			int inIndexFile = 0;
			inIndexFile = open(in_file.c_str(), flags, S_IRUSR | S_IWUSR);
			if (inIndexFile < 0) {
				throw std::runtime_error("Cannot open the index file: " + in_file);
			}
			struct stat buf;
			if (fstat(inIndexFile,&buf) != 0) {
				throw std::runtime_error("Cannot check the size of the file. Error returned by fstat(): " + in_file);
			}
			off_t file_size = buf.st_size;
			std::cout << "input file size(" << in_file << "):" << file_size << std::endl;
			std::cout << "2x" << inHeader.indexNodePages << "*(" << inHeader.pageSize << "x1024)=" << (2L*inHeader.indexNodePages*inHeader.pageSize*1024L) << std::endl;
			if(file_size != 0) {
				ASSERT((2L * inHeader.indexNodePages * inHeader.pageSize * 1024L) == file_size);
			}

			int outIndexFile = 0;
			outIndexFile = open(out_file.c_str(), flags, S_IRUSR | S_IWUSR);
			if (outIndexFile < 0) {
				throw std::runtime_error("Cannot open the outdex file: " + out_file);
			}

			uint64_t inIndexPageSize = inHeader.indexNodePages * inHeader.pageSize * 1024L;
			uint64_t inDatSize = 2L * inIndexPageSize;
			uint8_t * inPtr = new uint8_t[inDatSize];
			uint8_t * inPtr2 = &inPtr[inIndexPageSize];
			memset(inPtr, 0, inDatSize);
			pread(inIndexFile, inPtr, inDatSize, 0);

			uint64_t outIndexPageSize = outHeader.indexNodePages * outHeader.pageSize * 1024L;
			uint64_t outDatSize = 2L * outIndexPageSize;
			uint8_t * outPtr = new uint8_t[outDatSize];
			uint8_t * outPtr2 = &outPtr[outIndexPageSize];
			memset(outPtr, 0, outDatSize);

			std::vector<inBin> inEntries;
			std::vector<inBin> inEntries2;

			switch(inHeader.revision) {
				case 0: {
					Version<0, inKey> inVersion;
					inVersion.read(inPtr, inHeader, inEntries);
					inVersion.read(inPtr2, inHeader, inEntries2);
					break;
				}
				case 1: {
					Version<1, inKey> inVersion;
					inVersion.read(inPtr, inHeader, inEntries);
					inVersion.read(inPtr2, inHeader, inEntries2);
					break;
				}
				case 2: {
					Version<2, inKey> inVersion;
					inVersion.read(inPtr, inHeader, inEntries);
					inVersion.read(inPtr2, inHeader, inEntries2);
					break;
				}
				default: {
					throw std::runtime_error("unsupported revision: " + inHeader.revision);
				}
			}

			std::vector<outBin> outEntries;
			std::vector<outBin> outEntries2;

			int ind = 0;
			for(auto e : inEntries) {
				outBin b;
				b.firstKey = e.firstKey;
				b.maxKeyOffset = e.maxKeyOffset;
				b.recordsCount = e.recordsCount;
				b.bytesCount = e.bytesCount;
				b.pageId = e.pageId;
				outEntries.push_back(b);
				ind++;
			}
			ind = 0;
			for(auto e : inEntries2) {
				outBin b;
				b.firstKey = e.firstKey;
				b.maxKeyOffset = e.maxKeyOffset;
				b.recordsCount = e.recordsCount;
				b.bytesCount = e.bytesCount;
				b.pageId = e.pageId;
				outEntries2.push_back(b);
				ind++;
			}

			switch(outHeader.revision) {
				case 0: {
					Version<0, outKey> outVersion;
					outVersion.write(outPtr, outHeader, outEntries);
					outVersion.write(outPtr2, outHeader, outEntries2);
					break;
				}
				case 1: {
					Version<1, outKey> outVersion;
					outVersion.write(outPtr, outHeader, outEntries);
					outVersion.write(outPtr2, outHeader, outEntries2);
					break;
				}
				case 2: {
					Version<2, outKey> outVersion;
					outVersion.write(outPtr, outHeader, outEntries);
					outVersion.write(outPtr2, outHeader, outEntries2);
					break;
				}
				default: {
					throw std::runtime_error("unsupported revision: " + outHeader.revision);
				}
			}

			std::cout << "entries:" << outEntries.size() << std::endl;
			uint64_t max_pageId = 0;
			for(auto e : outEntries) {
				max_pageId = (max_pageId > e.pageId) ? max_pageId : e.pageId;
			}
			std::cout << "entries max pageId:" << max_pageId << std::endl;

			std::cout << "entries2:" << outEntries2.size() << std::endl;
			uint64_t max_pageId2 = 0;
			for(auto e : outEntries2) {
				max_pageId2 = (max_pageId2 > e.pageId) ? max_pageId2 : e.pageId;
			}
			std::cout << "entries2 max pageId:" << max_pageId2 << std::endl;

			pwrite(outIndexFile, outPtr, outDatSize, 0);
			delete [] inPtr;
			delete [] outPtr;

			close(inIndexFile);
			close(outIndexFile);

			std::cout << "output file : " << out_file << std::endl;
			std::cout << "2x" << outHeader.indexNodePages << "*(" << outHeader.pageSize << "x1024)=" << (2L*outHeader.indexNodePages*outHeader.pageSize*1024L) << std::endl;
			std::cout << "check file size" << std::endl;
		}

	};

}

#endif
