#include "referencesDatabase/alignment.hpp"
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <iostream>
#include <sstream>
#include <fstream>
#include <map>
#include <list>


void readFastaFile
	( std::string const & filename
	, std::map<std::string,ReferenceId> & refIds
	, std::vector<std::string> & sequences
	)
{
	std::cout << "Parsing fasta file: " << filename << " ... " << std::flush;
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
				std::string const name = line.substr(1);
				if (refIds.count(name)) throw std::runtime_error("Name already in use: " + name);
				refIds[name] = ReferenceId(sequences.size());
				sequences.push_back("");
			} else {
				if (sequences.empty()) throw std::runtime_error("First sequence without name!");
				for (auto & c: line) if (c >= 'a' && c <= 'z') c -= ('a'-'A');
				sequences.back() += line;
			}
		} catch (std::exception const & e) {
			throw std::runtime_error("Error when reading from the following file: " + filename + ", line: "
									+ boost::lexical_cast<std::string>(lineNo) + ", message: " + e.what());
		}
	}
	std::cout << "Done!" << std::endl;
}


void readMapFile
	( std::string const & filename
	, std::map<std::string,ReferenceId> const & refIds
	, std::vector<std::string> const & sequences
	, std::map<std::string,std::vector<Alignment>> & maps
	)
{
	std::cout << "Parsing fasta file: " << filename << " ... " << std::flush;
	std::ifstream file;
	file.open(filename);
	for (unsigned lineNo = 1; file.good(); ++lineNo) {
		std::string line;
		try {
			std::getline(file,line);
			boost::trim(line);
			if (file.eof() && line == "") break;
			std::stringstream ss(line);
			std::string trgName, srcName, srcStrand, cigar;
			unsigned srcPos, trgPos;
			ss >> trgName >> trgPos >> srcName >> srcStrand >> srcPos >> cigar;
			if (ss.fail() && ! ss.eof()) throw std::runtime_error("Cannot parse line");
			if (srcStrand != "+" && srcStrand != "-") throw std::runtime_error("Incorrect strand: " + srcStrand);
			ReferenceId const srcRefId = (refIds.count(srcName)) ? (refIds.at(srcName)) : (ReferenceId::null);
			ReferenceId const trgRefId = (refIds.count(trgName)) ? (refIds.at(trgName)) : (ReferenceId::null);
			if (srcRefId == ReferenceId::null) {
				std::cerr << "WARNING: Unknown sequence " << srcName << std::endl;
				continue;
			}
			if (trgRefId == ReferenceId::null) {
				std::cerr << "WARNING: Unknown sequence " << trgName << std::endl;
				continue;
			}
			Alignment const a = Alignment::createFromFullSequences( cigar, srcRefId,(srcStrand == "-"), sequences[srcRefId.value]
			                                                      , srcPos, sequences[trgRefId.value], trgPos );
			maps[trgName].push_back(a);
		} catch (std::exception const & e) {
			throw std::runtime_error("Error when reading from the following file: " + filename + ", line: "
									+ boost::lexical_cast<std::string>(lineNo) + ", message: " + e.what());
		}
	}
	std::cout << "Done!" << std::endl;
}

// map file:  target  pos  ref  strand  pos  cigar

int main(int argc, char** argv)
{
	if (argc < 3) {
		std::cout << "Parameters: in_source_fasta_file  list_of_map_files" << std::endl;
		std::cout << "For each *.map file there must be corresponding *.fasta. For each *.map file corresponding *.align file is created." << std::endl;
		return 1;
	}

	std::string const pathFastaSrc = argv[1];
	std::vector<std::string> pathsFiles(argv + 2, argv + argc);

	for (std::string & s: pathsFiles) {
		if (s.substr(s.size()-4) != ".map") {
			std::cerr << "The following file has incorrect name: " + s + " (it should be *.map file)." << std::endl;
			return 2;
		}
		s = s.substr(0,s.size()-4);
	}

	try {

		std::vector<std::string> sequences;     // sequences by id
		std::map<std::string,ReferenceId> refIds;  // name -> id
		std::vector<std::string> names;         // id -> name

		readFastaFile(pathFastaSrc, refIds, sequences);

		unsigned const numberOfMainReference = sequences.size();

		for (std::string const & basename: pathsFiles) {

			std::string const pathFastaTrg = basename + ".fasta";
			std::string const pathMap      = basename + ".map";
			std::string const pathAlign    = basename + ".align";

			std::map<std::string,std::vector<Alignment>> mappings;  // target name -> mappings

			readFastaFile(pathFastaTrg, refIds, sequences);
			readMapFile(pathMap, refIds, sequences, mappings);

			names.resize(refIds.size());
			for (auto i: refIds) names.at(i.second.value) = i.first;

			std::ofstream fileOut(pathAlign);

			for (auto const & kv: mappings) {
				std::string const trgName = kv.first;
				std::vector<Alignment> alignments = kv.second;

				auto sort_by_target_region = [](Alignment const & a1,Alignment const & a2){ return a1.targetRegion() < a2.targetRegion(); };
				std::sort( alignments.begin(), alignments.end(), sort_by_target_region );

				// ----- calculate vector with boundaries and list of regions
				std::vector<unsigned> boundaries;
				std::list<std::pair<RegionCoordinates,ReferenceId>> regionsList;
				for (Alignment const & a: alignments) {
					RegionCoordinates const r = a.targetRegion();
					boundaries.push_back(r.left());
					boundaries.push_back(r.right());
					regionsList.push_back( std::make_pair(r,a.sourceRefId) );
				}
				std::sort(boundaries.begin(), boundaries.end());
				boundaries.erase( std::unique(boundaries.begin(), boundaries.end()), boundaries.end() );

				// ----- process list of regions - calculate vector of "small" regions
				std::vector<std::pair<RegionCoordinates,ReferenceId>> regions;
				// cut region one by one
				for (unsigned b: boundaries) {
					// cut regions ending in b
					for ( auto itRegionList = regionsList.begin();  itRegionList != regionsList.end(); ) {
						auto nextIterator = itRegionList;
						++nextIterator;
						// analyze element
						if (itRegionList->first.left() >= b) break;
						regions.push_back(*itRegionList);
						if (itRegionList->first.right() <= b) {
							regionsList.erase(itRegionList);
						} else {
							regions.back().first.setRight(b);
							itRegionList->first.setLeft(b);
						}
						// go to next element
						itRegionList = nextIterator;
					}
				}
				std::sort( regions.begin(), regions.end() );

				// ----- calculate final regions
				std::vector<std::pair<RegionCoordinates,ReferenceId>> finalRegions;
				for ( auto itR = regions.begin(); itR != regions.end(); ) {
					RegionCoordinates const r = itR->first;
					std::vector<ReferenceId> ids;
					for ( ; itR != regions.end() && itR->first == r; ++itR ) ids.push_back(itR->second);
					ReferenceId nc_id, nw_id, nt_id;
					unsigned nc = 0, nw = 0, nt = 0;
					for (ReferenceId const & id: ids) {
						std::string const name = names.at(id.value);
						if      (name.substr(0,3) == "NC_") { ++nc; nc_id = id; }
						else if (name.substr(0,3) == "NT_") { ++nt; nt_id = id; }
						else if (name.substr(0,3) == "NW_") { ++nw; nw_id = id; }
						else throw std::runtime_error("Unknown source reference name: " + name);
					}
					if (nc >= 2) continue;
					if (nc == 0 && nt >= 2) continue;
					if (nc == 0 && nt == 0 && nw >= 2) continue;
					ReferenceId id;
					if (nc) id = nc_id;
					else if (nt) id = nt_id;
					else if (nw) id = nw_id;
					else throw std::logic_error("Something is wrong :-D");
					// add region to final regions
					if (finalRegions.empty() || finalRegions.back().second != id || finalRegions.back().first.right() != r.left()) {
						finalRegions.push_back(std::make_pair(r,id));
					} else {
						finalRegions.back().first.setRight(r.right());
					}
				}

				// ----- generate output records for final regions
				fileOut << ">" <<  trgName << "\n";
				unsigned const trgId = refIds[trgName].value;
				auto itA = alignments.begin();
				unsigned lastPos = 0;
				for (auto fr: finalRegions) {
					RegionCoordinates const & r = fr.first;
					ReferenceId const & id = fr.second;
					while (itA != alignments.end() && itA->targetRegion().right() <= r.left()) ++itA;
					for ( auto itA2 = itA; itA2 != alignments.end() && itA2->targetRegion().left() < r.right(); ++itA2 ) {
						if (itA2->sourceRefId != id || itA2->targetRegion().right() <= r.left()) continue;
						Alignment align = itA2->targetSubalign(r.left(),r.length(),true);
						fileOut << sequences[trgId].substr(lastPos, align.targetLeftPosition - lastPos);
						fileOut << "\t" << names.at(align.sourceRefId.value) << "\t";
						fileOut << (align.sourceStrandNegativ ? '-' : '+') << "\t" << align.sourceLeftPosition;
						fileOut << "\t" << align.cigarString(true) << "\n";
						lastPos = align.targetRegion().right();
					}
				}
				if (sequences[trgId].size() > lastPos) {
					fileOut << sequences[trgId].substr(lastPos);
					fileOut << "\t\t+\t0\t\n";
				}

				if (fileOut.fail()) throw std::runtime_error("Cannot write to file " + pathAlign);
			}

			// ========== cleaning
			for (unsigned i = numberOfMainReference; i < names.size(); ++i) refIds.erase(names[i]);
			names.resize(numberOfMainReference);
			sequences.resize(numberOfMainReference);
		}

	} catch (std::exception const & e) {
		std::cerr << "ERROR: " << e.what() << std::endl;
		return -1;
	}

	return 0;
}
