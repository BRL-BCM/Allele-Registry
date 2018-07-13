#include "referencesDatabase.hpp"
#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/lexical_cast.hpp>
#include <iostream>
#include <fstream>


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
	if (argc != 3) {
		std::cout << "Parameters:  path_to_reference_database  fasta_file" << std::endl;
		return 1;
	}

	std::string const pathRefDb = argv[1];
	std::string const pathFasta = argv[2];

	std::vector<std::string> names;
	std::vector<std::string> sequences;

	std::cout << "Load fasta file..." << std::endl;
	readFromFile(pathFasta, names, sequences);

	std::cout << "Load references database..." << std::endl;
	ReferencesDatabase * refDb = new ReferencesDatabase(pathRefDb);

	unsigned countUnknown = 0;
	unsigned mismatches = 0;

	std::cout << "Compare reference sequences..." << std::endl;
	for (unsigned i = 0; i < names.size(); ++i) {
		for (char & c : sequences[i]) if (c == 'U') c = '*';
		ReferenceId refId;
		try {
			//std::cout << "Get refId for: " << names[i] << std::endl;
			refId = refDb->getReferenceId(names[i]);
		} catch (...) {
			++countUnknown;
			continue;
		}
		try {
			//std::cout << "Get sequence length for refID=" << refId.value << std::endl;
			unsigned const length =  refDb->getSequenceLength(refId);
			//std::cout << "Get sequence" << std::endl;
			//std::cout << "Length=" << length << std::endl;
			std::string seq = refDb->getSequence(refId, RegionCoordinates(0,length));
			if (sequences[i].size() < seq.size()) seq.pop_back(); // TODO - temporary - some ENSEMBL proteins have last amino-acid missing
			if (seq != sequences[i]) {
				std::cout << "Mismatch for " << names[i] << "\n" << seq << "\n" << sequences[i] << std::endl;
				++mismatches;
			}
		} catch (...) {
			std::cerr << "Exception for " << names[i] << std::endl;
			throw;
		}
	}

	std::cout << "count of all sequences = " << names.size() << std::endl;
	std::cout << "unknown sequences: " << countUnknown << std::endl;
	std::cout << "mismatches: " << mismatches << std::endl;

	return 0;
}

