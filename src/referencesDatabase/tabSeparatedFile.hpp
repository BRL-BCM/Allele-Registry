#ifndef TABSEPARATEDFILE_HPP_
#define TABSEPARATEDFILE_HPP_


#include <boost/lexical_cast.hpp>
#include <string>
#include <vector>
#include <fstream>
#include <stdexcept>

class TabSeparatedFile {
private:
	std::ifstream fFile;
	std::vector<unsigned> fColumns;
	std::string const fFilename;
	unsigned fLineNo;
	inline bool tryReadLine(std::vector<std::string> & output)
	{
		std::string line = "";
		std::getline(fFile, line);
		if (fFile.eof() && line == "") return false;
		if (fFile.fail()) throw std::runtime_error("Error occurred when reading line "
							+ boost::lexical_cast<std::string>(fLineNo) + " from file " + fFilename + ".");
		output.clear();
		unsigned pos1 = 0;
		do {
			std::string::size_type pos2 = line.find('\t',pos1);
			if (pos2 == std::string::npos) pos2 = line.size();
			output.push_back( line.substr(pos1,pos2-pos1) );
			pos1 = pos2 + 1;
		} while (pos1 <= line.size());
		return true;
	}
public:
	TabSeparatedFile(std::string const & filename, std::vector<std::string> const & columns)
	: fFilename(filename), fLineNo(1)
	{
		fFile.open(filename);
		if (! fFile.good()) throw std::runtime_error("Cannot open file " + filename + ".");
		std::vector<std::string> headers;
		if ( ! tryReadLine(headers) ) throw std::runtime_error("The file " + filename + " is empty.");
		for (auto const & col: columns) {
			auto it = std::find(headers.begin(), headers.end(), col);
			if (it == headers.end()) {
				throw std::runtime_error("Column " + col + " does not exist in the file " + filename + ".");
			}
			fColumns.push_back( it - headers.begin() );
		}
	}
	inline bool readRecord(std::vector<std::string> & values)
	{
		++fLineNo;
		std::vector<std::string> rawData;
		if (! tryReadLine(rawData)) return false;
		values.clear();
		for (unsigned colId: fColumns) {
			if (colId >= rawData.size()) {
				throw std::runtime_error( "Too small number of values in line "
						+ boost::lexical_cast<std::string>(fLineNo) + " from file " + fFilename
						+ ". Number of parsed values: " + boost::lexical_cast<std::string>(rawData.size()) );
			}
			values.push_back(rawData[colId]);
		}
		return true;
	}
};


#endif /* TABSEPARATEDFILE_HPP_ */
