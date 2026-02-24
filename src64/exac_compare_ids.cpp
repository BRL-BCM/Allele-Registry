#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/trim.hpp>



int main(int argc, char ** argv)
{
	if (argc != 3) {
		std::cerr << "Two parameters:  text_file_with_IDs  text_file_with_json_output" << std::endl;
		return 1;
	}

	std::string const filenameIds  = argv[1];
	std::string const filenameJson = argv[2];

	std::ifstream fileIds (filenameIds );
	std::ifstream fileJson(filenameJson);

	unsigned countAll = 0;
	unsigned countNotFound = 0;
	unsigned countError = 0;
	unsigned countMismatch = 0;

	while (true)
	{
		std::string lineId = "";
		std::string lineJson = "";
		std::getline(fileIds, lineId);
		std::getline(fileJson, lineJson);

		if (lineJson.substr(0,1) == "]") break;

		boost::trim(lineId);
		boost::trim(lineJson);
		if (lineId == "" || lineJson == "") throw std::runtime_error("Error when reading files: files do not match or IO error");

		++countAll;

		auto pos = lineJson.find("\"ExAC\":[");
		if (pos == std::string::npos) {
			if ( lineJson.find("\"errorType\":") == std::string::npos ) {
				++countNotFound;
			} else {
				++countError;
			}
			continue;
		}

		std::vector<std::string> ids;
		auto pos2 = lineJson.find("]",pos);
		while (true) {
			std::string const start = "\"id\":\"";
			pos = lineJson.find(start, pos);
			if (pos >= pos2) break;
			pos += start.size();
			auto posEnd = lineJson.find("\"", pos);
			ids.push_back( lineJson.substr(pos,posEnd-pos) );
			pos = posEnd;
		}

		if (ids.empty()) {
			++countNotFound;
			continue;
		}

		if ( std::find(ids.begin(),ids.end(),lineId) == ids.end() ) {
			++countMismatch;
//			std::cout << lineId << "\t" << ids.front() << std::endl;
			continue;
		}
	}

	std::cout << "all:\t" << countAll << std::endl;
	std::cout << "not found:\t" << countNotFound << std::endl;
	std::cout << "error:\t" << countError << std::endl;
	std::cout << "mismatch:\t" << countMismatch << std::endl;

	return 0;
}
