#include "../core/variants.hpp"
#include "proteinTools.hpp"
#include <set>
#include <map>
#include <random>

// produces random protein variants and calculates for them with corresponding combinations of transcript variants
// format of single variant: start_position,stop_position,reference_sequence,new_sequence
// format of the output:
// first line: ACGT sequence - transcript
// second: position_of_start_codon <TAB> list_of_transcript_variants_separated_by_TAB
// other lines: protein_variant <TAB> list_of_transcript_variants_separated_by_TAB


std::default_random_engine generator;

std::string generateSequence(unsigned length, bool noStopCodons = false)
{
	std::uniform_int_distribution<int> distribution(0,3);
	std::string s = "";
	while (s.size() < length) {
		switch (distribution(generator)) {
		case 0: s += "A"; break;
		case 1: s += "C"; break;
		case 2: s += "G"; break;
		case 3: s += "T"; break;
		}
		if (noStopCodons && s.size() % 3 == 0) {
			std::string lastCodon = s.substr(s.size()-3);
			if (lastCodon == "TAA" || lastCodon == "TAG" || lastCodon == "TGA") s.resize(s.size()-3);
		}
	}
	return s;
}

unsigned generateNumber(unsigned min, unsigned max)
{
	std::uniform_int_distribution<unsigned> distribution(min,max);
	return distribution(generator);
}

void print(std::ostream & out, PlainSequenceModification const & sm)
{
	out << sm.region.left() << "," << sm.region.right() << "," << sm.originalSequence << "," << sm.newSequence;
}

int main(int argc, char ** argv)
{

	unsigned seed = 1;
	if (argc == 2) seed = atoi(argv[1]);
	generator.seed(seed);

	unsigned const proteinLength = 20;
	std::string const rnaSeq = "ACTGGTGCTAATG" + generateSequence(3*(proteinLength-2),true) + "TAGATAGAATAGGACT";
	unsigned const startCodon = 10;
	std::string const aaSeq = translateToAminoAcid(rnaSeq.substr(startCodon,3*proteinLength));

	unsigned const numberOfIndels = 25;
	std::vector<PlainSequenceModification> rnaIndels;

	for ( unsigned i = 0; i < numberOfIndels; ++i ) {
		unsigned left = generateNumber(startCodon+3,startCodon+3*proteinLength);
		unsigned right = generateNumber(left, left+5);
		PlainSequenceModification sm;
		sm.newSequence = generateSequence(generateNumber(0,5));
		sm.region.set(left, right);
		sm.originalSequence = rnaSeq.substr(left,right-left);
		// cut from left
		while ( ! sm.newSequence.empty() && ! sm.originalSequence.empty() && sm.newSequence.front() == sm.originalSequence.front() ) {
			sm.newSequence = sm.newSequence.substr(1);
			sm.originalSequence = sm.originalSequence.substr(1);
			sm.region.incLeftPosition(1);
		}
		// cut from right
		while ( ! sm.newSequence.empty() && ! sm.originalSequence.empty() && sm.newSequence.back() == sm.originalSequence.back() ) {
			sm.newSequence.resize(sm.newSequence.size()-1);
			sm.originalSequence.resize(sm.originalSequence.size()-1);
			sm.region.decRightPosition(1);
		}
		// check if not empty
		if (sm.newSequence.empty() && sm.originalSequence.empty()) continue;
		// rotate right
		if ( sm.newSequence.empty() ) {
			while ( sm.region.right() < rnaSeq.size() && sm.originalSequence.front() == rnaSeq[sm.region.right()] ) {
				sm.originalSequence = sm.originalSequence.substr(1) + sm.originalSequence.substr(0,1);
				sm.region.incRightPosition(1);
				sm.region.incLeftPosition(1);
			}
		}
		if ( sm.originalSequence.empty() ) {
			while ( sm.region.right() < rnaSeq.size() && sm.newSequence.front() == rnaSeq[sm.region.right()] ) {
				sm.newSequence = sm.newSequence.substr(1) + sm.newSequence.substr(0,1);
				sm.region.incRightPosition(1);
				sm.region.incLeftPosition(1);
			}
		}
		// save result
		rnaIndels.push_back(sm);
	}

	std::sort(rnaIndels.begin(), rnaIndels.end());
	rnaIndels.erase( std::unique(rnaIndels.begin(), rnaIndels.end()), rnaIndels.end() );

	std::map<PlainSequenceModification,std::vector<unsigned>> aaVariantsToCombinations;

	unsigned pot2 = 1;
	for (unsigned i = 0; i < rnaIndels.size(); ++i) pot2 <<= 1;
	for ( unsigned comb = 1;  comb < pot2;  ++comb ) {
		std::vector<PlainSequenceModification> variants;
		unsigned i = 0;
		for ( unsigned c = 1; c < pot2; c <<= 1 ) {
			if ( c & comb ) variants.push_back(rnaIndels[i]);
			++i;
		}
		for ( i = 1; i < variants.size(); ++i ) if (variants[i-1].region.right() >= variants[i].region.left()) { variants.clear(); break; }
		if (variants.empty()) continue;
		std::string s = applyVariantsToTheSequence(rnaSeq, 0, variants);
		std::string aa = translateToAminoAcid(s.substr(startCodon));
		std::string::size_type ter = aa.find_first_of('*');
		if (ter != std::string::npos) aa.resize(ter+1);
		// compare with original AA
		unsigned minSize = std::min(aa.size(),aaSeq.size());
		unsigned left = 0;
		for ( ;  left < aa.size() && left < aaSeq.size() && aa[left] == aaSeq[left];  ++left );
		unsigned right = 0;
		for ( ;  right + left < minSize && aa[aa.size()-right-1] == aaSeq[aaSeq.size()-right-1];  ++right );
		PlainSequenceModification sm;
		sm.region.set(left, aaSeq.size() - right);
		sm.originalSequence = aaSeq.substr(left,sm.region.length());
		sm.newSequence = aa.substr(left, aa.size()-left-right);
		if (sm.originalSequence.size() < 7 && sm.newSequence.size() < 7) {
			aaVariantsToCombinations[sm].push_back(comb);
		}
	}

	std::ostream & out = std::cout;

	out << rnaSeq << "\n" << startCodon;
	for (auto x: rnaIndels) {
		out << "\t";
		print(out, x);
	}
	out << "\n";
	for (auto kv: aaVariantsToCombinations)
	{
		for (unsigned i = 0; i < kv.second.size(); ++i) {
			bool isOk = true;
			// check if there is no better one
			for (unsigned j = 0; j < kv.second.size(); ++j) {
				if (i != j) {
					if ( (kv.second[i] & kv.second[j]) == kv.second[j] ) {
						isOk = false;
						break;
					}
				}
			}
			if (isOk) {
				print(out, kv.first);
				unsigned j = 0;
				for ( unsigned c = 1; c < pot2; c <<= 1 ) {
					if ( c & kv.second[i] ) {
						out << "\t";
						print(out, rnaIndels[j]);
					}
					++j;
				}
				out << "\n";
			}
		}
	}

	return 0;
}
