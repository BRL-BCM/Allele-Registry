#include "commands.hpp"
#include <iostream>
#include <fstream>
#include <memory>
#include <string>
#include <vector>
#include <stdexcept>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/split.hpp>


inline std::vector<std::string> split_with_single_space(std::string const & s, unsigned const minimal_words_number)
{
	std::vector<std::string> v;
	boost::split(v, s, boost::is_any_of(" \t"));
	if (v.size() < minimal_words_number) {
		throw std::runtime_error("At least " + boost::lexical_cast<std::string>(minimal_words_number)
				+ " word(s) was/were expected, found " + boost::lexical_cast<std::string>(v.size()) + ".");
	}
	return v;
}

void readFromFile( std::string const & filename, std::vector<std::string> & names, std::vector<std::string> & target )
{
	std::ifstream file;
	file.open(filename);
	for (unsigned lineNo = 1; file.good(); ++lineNo) {
		std::string line;
		try {
			std::getline(file,line);
			boost::trim(line);
			if (line == "" || line[0] == '#') continue;
			if (line[0] == '>') {
				// new record
				std::string const name = split_with_single_space(line.substr(1),1)[0];
				names.push_back(name);
				target.push_back("");
			} else {
				if (target.empty()) throw std::runtime_error("First sequence without name!");
				convertToUpperCase(line);
				target.back() += line;
			}
		} catch (std::exception const & e) {
			throw std::runtime_error("Error when reading from the following file: " + filename + ", line: " + boost::lexical_cast<std::string>(lineNo) + ", message: " + e.what());
		}
	}
}



int main(int argc, char ** argv)
{
	if (argc < 3) {
		std::cout << "Parameters: path_to_ref_db  fasta_files_to_test" << std::endl;
		return 1;
	}

	ReferencesDatabase * refDb = new ReferencesDatabase(argv[1]);
	std::unique_ptr<ReferencesDatabase> bindToScope(refDb);

	std::vector<std::string> fastaFiles;
	for (int i = 2; i < argc; ++i) fastaFiles.push_back(argv[i]);

	for ( std::string const & filename: fastaFiles ) {
		std::cout << "Test " << filename << " ... " << std::endl;
		std::vector<std::string> names;
		std::vector<std::string> target;
		readFromFile(filename, names, target);
		for (unsigned i = 0; i < names.size(); ++i) {
			std::cout << " -> " << names[i] << std::endl;
			std::string seq;
			commandGetSequence(refDb, names[i], seq);
			if (seq != target[i]) {
				std::cout << "Error for " << names[i] << std::endl;
				std::cout << "Lengths (org/calculated): " << target[i].size() << " " << seq.size() << std::endl;
				std::string orgSeq = "";
				std::string calcSeq = "";
				for (unsigned j = 0; j < seq.size() && j < target[i].size(); ++j) {
					if (seq[j] != target[i][j]) {
						orgSeq += target[i].substr(j,1);
						calcSeq += seq.substr(j,1);
					} else if (orgSeq != "") {
						std::cout << "Diff at pos " << (j-orgSeq.size()) << ": \n" << orgSeq << "\n" << calcSeq << std::endl;
						orgSeq = calcSeq = "";
					}
				}
				if (orgSeq != "") {
					std::cout << "Diff at pos " << (std::min(seq.size(),target[i].size())-orgSeq.size()) << ": \n" << orgSeq << "\n" << calcSeq << std::endl;
				}
			}
		}
	}

	return 0;
}
