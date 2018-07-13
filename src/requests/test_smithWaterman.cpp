#include "smithWaterman.hpp"
#include <boost/test/minimal.hpp>


static std::vector<std::pair<char,unsigned>> parseCigar(std::string const & cigar)
{
	std::vector<std::pair<char,unsigned>> v;
	std::string::const_iterator iC = cigar.begin();
	std::string::const_iterator iCend = cigar.end();
	unsigned count = 0;
	for ( ;  iC != iCend;  ++iC ) {
		if (isdigit(*iC)) {
			count *= 10;
			count += *iC - '0';
			continue;
		}
		if (count == 0) continue;
//		if ( count == 0 ) {
//			throw std::runtime_error("Incorrect CIGAR string: missing number (or 0) after parsing " + std::string(cigar.begin(),iC) + " from " + cigar);
//		}
		v.push_back( std::make_pair(*iC,count) );
		count = 0;
	}
	if (count) throw std::runtime_error("Incorrect CIGAR string: missing symbol at the end of CIGAR string: " + cigar);
	return v;
}

static std::vector<PlainSequenceModification> cigar2variant
		( std::string const & cigar
		, std::string const & srcFullSeq
		, std::string const & trgFullSeq
		)
{
	std::vector<PlainSequenceModification> results;
	auto const vc = parseCigar(cigar);
	unsigned srcLength = 0, trgLength = 0;
	bool inVariation = false;
	for (auto const & c: vc) {
		switch (c.first) {
		case '=':
			if (srcFullSeq.substr(srcLength,c.second) != trgFullSeq.substr(trgLength,c.second)) throw std::logic_error("cigar1");
			srcLength += c.second;
			trgLength += c.second;
			inVariation = false;
			break;
		case 'X':
			if (! inVariation) {
				PlainSequenceModification sm;
				sm.region.setLeftAndLength(srcLength, 0);
				results.push_back(sm);
				inVariation = true;
			}
			results.back().region.incRightPosition(c.second);
			results.back().originalSequence += srcFullSeq.substr(srcLength,c.second);
			results.back().newSequence += trgFullSeq.substr(trgLength,c.second);
			srcLength += c.second;
			trgLength += c.second;
			break;
		case 'I':
			if (! inVariation) {
				PlainSequenceModification sm;
				sm.region.setLeftAndLength(srcLength, 0);
				results.push_back(sm);
				inVariation = true;
			}
			results.back().newSequence += trgFullSeq.substr(trgLength,c.second);
			trgLength += c.second;
			break;
		case 'D':
			if (! inVariation) {
				PlainSequenceModification sm;
				sm.region.setLeftAndLength(srcLength, 0);
				results.push_back(sm);
				inVariation = true;
			}
			results.back().region.incRightPosition(c.second);
			results.back().originalSequence += srcFullSeq.substr(srcLength,c.second);
			srcLength += c.second;
			break;
		default:
			throw std::runtime_error("Incorrect CIGAR string: alignment symbol not supported: " + std::string(1,c.first));
		}
	}
	if (srcLength != srcFullSeq.size()) throw std::logic_error("cigar2");
	if (trgLength != trgFullSeq.size()) throw std::logic_error("cigar3");
	for (auto const & sm: results) {
		if ( sm.originalSequence.empty() || sm.newSequence.empty() ) continue;
		if ( sm.originalSequence.front() == sm.newSequence.front() ) throw std::logic_error("cigar4");
		if ( sm.originalSequence.back()  == sm.newSequence.back()  ) throw std::logic_error("cigar5");
	}
	return results;
}



static void test(std::string const & org, std::string const & mod, std::string const & cigar)
{
	//std::cout << "TEST " << org << " " << mod << std::endl;
	std::vector<PlainSequenceModification> expectedVariant = cigar2variant(cigar, org, mod);
	std::vector<PlainSequenceModification> calculatedVariant;
	bool const success = tryDescribeAsProteinVariant(org, mod, 0, calculatedVariant);
	unsigned expectedCost = 0;
	for (auto const & sm: expectedVariant) expectedCost += std::max( sm.originalSequence.size(), sm.newSequence.size() );
	bool const expectedSuccess = (expectedCost <= 7);
	if (success != expectedSuccess) throw std::runtime_error("success != expectedSuccess");
	if (! success) return;
	if (expectedVariant != calculatedVariant) {
		std::cout << "expected:";
		for (auto const & sm: expectedVariant) std::cout << "\t" << sm.toString();
		std::cout << "\ncalculated:";
		for (auto const & sm: calculatedVariant) std::cout << "\t" << sm.toString();
		std::cout << "\n";
		std::runtime_error("Different Variant!");
	}
}


int test_main(int argc, char ** argv)
{
	for ( unsigned i = 0; i < 1000; ++i) {
		test("ABC1234ABC", "ABB1233BBC", "2=1X3=2X2=");
		test("ABC1234ABC", "ABC3BBC", "3=2D1=2D1I2=");
		test("aBcccBaccaB", "aBBBcBaccccaBB", "2=2X5=2I2=1I");
		test("CTAAAACCACCTA", "CTAAACCACCACCTA", "5=2I8=");
		test("CTAAACCACCATA", "CTAAAAACCACCACCATA", "5=2I6=3I2=");
		test("XXXYYY", "YYYXXX", "6X");
		test("SPPNCTELQIDSCSSSEE", "THLIVLNCKLIVVLAVK", "17X1D" );
	}
	return 0;
}
