#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <boost/algorithm/string/trim.hpp>
#include <cstring>

unsigned const bufferSize = 256*1024*1024; // 256MB
std::vector<std::pair<char*,char*>> buffers;  // (buffer,ptrToFreeSpace)
void buffer_init()
{
	buffers.resize(1);
	buffers[0].first = new char[bufferSize];
	buffers[0].second = buffers[0].first;
}
char * buffer_add(char const * data, unsigned size)
{
	if (buffers.back().first + bufferSize < buffers.back().second + size + 1) {
		char * buf = new char[bufferSize];
		buffers.push_back(std::make_pair(buf,buf));
	}
	char * ptr = buffers.back().second;
	strncpy(ptr, data, size);
	ptr[size] = '\0';
	buffers.back().second += (size+1);
	return ptr;
}


int main(int argc, char ** argv)
{
	if (argc != 3) {
		std::cerr << "Two parameters:  input_file  output_file" << std::endl;
		return 1;
	}

	std::vector<std::pair<uint64_t,char*>> data;

	std::string const filenameIds = argv[1];
	std::string const filenameOut = argv[2];

	std::ifstream fileIds(filenameIds );
	std::ofstream fileOut(filenameOut);

	unsigned countAll = 0;
	unsigned countNotFound = 0;

	while (true)
	{
		std::string lineId = "";
		std::getline(fileIds, lineId);
		if (lineId == "") break;
		boost::trim(lineId);

		++countAll;

		if (lineId.substr(0,3) != "chr" || lineId.size() < 10) {
			++countNotFound;
			fileOut << lineId << "\n";
			continue;
		}

		unsigned chr = 0;
		unsigned i = 4;
		if (lineId[3] > '0' && lineId[3] <= '9') {
			chr = lineId[3] - '0';
			if (lineId[4] >= '0' && lineId[4] <= '9') {
				++i;
				chr *= 10;
				chr += (lineId[4] - '0');
				if (chr > 22) chr = 0;
			}
		} else if (lineId[3] == 'X') {
			chr = 23;
		} else if (lineId[3] == 'Y') {
			chr = 24;
		} else if (lineId[3] == 'M' && lineId[4] == 'T') {
			++i;
			chr = 25;
		}
		if (chr == 0 || lineId[i] != ':' || lineId[i+1] != 'g' || lineId[i+2] != '.' || lineId[i+3] < '1' || lineId[i+3] > '9') {
			++countNotFound;
			fileOut << lineId << "\n";
			continue;
		}
		i += 3;

		unsigned pos = 0;
		while ( i < lineId.size() && lineId[i] >= '0' && lineId[i] <= '9' ) {
			pos *= 10;
			pos += (lineId[i] - '0');
		}

		if (i >= lineId.size()) {
			++countNotFound;
			fileOut << lineId << "\n";
			continue;
		}

		uint64_t xxx = chr * static_cast<uint64_t>(1024) * 1024 * 1024 + pos;
		char * buf = buffer_add( lineId.data()+i, lineId.size()-i );
		data.push_back( std::make_pair(xxx,buf) );
	}

	std::cout << "all:\t" << countAll << std::endl;
	std::cout << "not found:\t" << countNotFound << std::endl;

	fileIds.close();

	std::sort(data.begin(),data.end());

	for (auto & p: data) {
		unsigned chr = p.first / (1024*1024*1024);
		unsigned pos = p.first % (1024*1024*1024);
		fileOut << "chr" << chr << ":g." << pos << p.second << "\n";
	}

	return 0;
}
