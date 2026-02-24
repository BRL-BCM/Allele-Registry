#include "proteinVariantsToGenomic.hpp"
#include "proteinTools.hpp"
#include <boost/algorithm/string.hpp>

void test1()
{
	std::string rnaSeq = "ATGGCGACCGGAGCGAACGCCACGCCGTTGGGTAAGCTGGGCCCCCCCGGGCTGCCCCCG";
	std::string aaSeq  = translateToAminoAcid(rnaSeq);

	std::vector<PlainSequenceModification> rnaVariants;
	rnaVariants.push_back( PlainSequenceModification(38, 38, "", "T") );
	rnaVariants.push_back( PlainSequenceModification(50, 51, "G", "") );
	rnaVariants.push_back( PlainSequenceModification(13, 14, "C", "G") );
	rnaVariants.push_back( PlainSequenceModification(40, 41, "G", "A") );
	std::sort(rnaVariants.begin(), rnaVariants.end());

	std::vector<PlainSequenceModification> aaVariants;
	aaVariants.push_back( PlainSequenceModification(16, 17, "G", "R") );
	aaVariants.push_back( PlainSequenceModification(4, 5, "A", "G") );
	aaVariants.push_back( PlainSequenceModification(16, 19, "GLP", "RH") );
	std::sort(aaVariants.begin(), aaVariants.end());

	std::vector<PlainVariant> aaFullVariants;
	for (auto av: aaVariants) {
		PlainVariant pv;
		pv.refId.value = 123;
		pv.modifications.resize(1, av);
		aaFullVariants.push_back(pv);
	}

	std::vector< std::vector<PlainVariant> > out = searchForMatchingTranscriptVariants( ReferenceId(1234), 0, rnaSeq, rnaVariants, aaSeq, aaFullVariants );
	ASSERT(out.size() == aaFullVariants.size());
	for (unsigned i = 0; i < out.size(); ++i) {
		std::cout << aaVariants[i].toString() << " ==>";
		for (auto & s: out[i]) std::cout << " " << s;
		std::cout << std::endl;
	}
}

PlainSequenceModification parsePainSequenceModification(std::string const & s)
{
	std::vector<std::string> vs;
	boost::split(vs, s, boost::is_any_of(","));
	if (vs.size() != 4) throw std::runtime_error("Incorrect variant: " + s);
	PlainSequenceModification sm;
	sm.region.set( boost::lexical_cast<unsigned>(vs[0]), boost::lexical_cast<unsigned>(vs[1]) );
	sm.originalSequence = vs[2];
	sm.newSequence = vs[3];
	return sm;
}

int main(int argc, char ** argv)
{
	//test1();

	// read data from input
	std::string rnaSeq;
	std::getline(std::cin, rnaSeq);

	std::string line;
	std::getline(std::cin, line);
	std::vector<std::string> vs;
	boost::split(vs, line, boost::is_any_of("\t"));
	if (line.size() < 2) return 1;
	std::vector<PlainSequenceModification> rnaVariants;
	unsigned rnaStartCodon = boost::lexical_cast<unsigned>(vs[0]);
	for (unsigned i = 1; i < vs.size(); ++i) rnaVariants.push_back( parsePainSequenceModification(vs[i]) );
	std::map<PlainSequenceModification, std::vector<std::vector<PlainSequenceModification>>> aaVar2rnaVars;
	while (true) {
		line.clear();
		std::getline(std::cin, line);
		boost::trim(line);
		std::vector<std::string> vs;
		boost::split(vs, line, boost::is_any_of("\t"));
		if (vs.empty() || vs.front().empty()) break; // end of input
		PlainSequenceModification aaVar = parsePainSequenceModification(vs[0]);
		aaVar2rnaVars[aaVar].size();
		if (vs.size() > 1) {
			aaVar2rnaVars[aaVar].resize( aaVar2rnaVars[aaVar].size()+1 );
			for (unsigned i = 1; i < vs.size(); ++i) {
				aaVar2rnaVars[aaVar].back().push_back( parsePainSequenceModification(vs[i]) );
			}
		}
	}
	// aa sequence
	std::string aaSeq = translateToAminoAcid(rnaSeq.substr(rnaStartCodon));
	if (aaSeq.find_first_of('*') != std::string::npos) aaSeq.resize(aaSeq.find_first_of('*')+1);

	// aa variants
	std::vector<PlainVariant> aaFullVariants;
	for (auto & kv: aaVar2rnaVars) {
		PlainVariant pv;
		pv.refId.value = 123;
		pv.modifications.resize(1, kv.first);
		aaFullVariants.push_back(pv);
		std::sort(kv.second.begin(),kv.second.end());
	}

	std::cout << "Start calculations..." << std::endl;

	// run calculations
	std::vector< std::vector<PlainVariant> > out = searchForMatchingTranscriptVariants( ReferenceId(1234), rnaStartCodon, rnaSeq, rnaVariants, aaSeq, aaFullVariants );

	// check response
	for (unsigned i = 0; i < aaFullVariants.size(); ++i) {
		std::vector<std::vector<PlainSequenceModification>> mods;
		for (auto pv: out[i]) {
			mods.push_back(pv.modifications);
		}
		std::sort(mods.begin(),mods.end());
		PlainSequenceModification aaVar = aaFullVariants[i].modifications.front();
		bool match = (mods.size() == aaVar2rnaVars[aaVar].size());
		if (match) {
			for (unsigned j = 0; j < mods.size(); ++j) {
				if (mods[j] != aaVar2rnaVars[aaVar][j]) {
					match = false;
					break;
				}
			}
		}
		if (! match) {
			std::cout << "Error for the following:\n";
			std::cout << "Expected: " << aaVar << " => " << aaVar2rnaVars[aaVar] << std::endl;
			std::cout << "Found   : " << aaVar << " => " << mods << std::endl;
		}
	}

	return 0;
}

