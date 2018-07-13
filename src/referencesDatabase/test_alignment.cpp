#include "alignment.hpp"
#include <boost/test/minimal.hpp>
#include <stdexcept>
#include <boost/lexical_cast.hpp>
#include <iostream>



void test_alignments(std::string const & src, std::string const & trg, std::vector<std::string> const & cigars)
{
	for (auto & cigar: cigars) {
		std::cout << "Test " << cigar << " " << src << " " << trg << std::endl;
		Alignment a;
		a.set(cigar, ReferenceId::null, false, src, 0, trg, 0);
		std::string const trg2 = a.processSourceSubSequence(src);
		BOOST_CHECK( trg == trg2 );
		Alignment a2 = a.reverse( StringPointer(src.data(),src.size()) , ReferenceId::null);
		std::string const src2 = a2.processSourceSubSequence(trg);
		BOOST_CHECK( src == src2 );
		unsigned const srcL = src.size();
		unsigned const trgL = trg.size();
		unsigned const a1src = a.sourceLength();
		unsigned const a1trg = a.targetLength();
		unsigned const a2src = a2.sourceLength();
		unsigned const a2trg = a2.targetLength();
		BOOST_CHECK( srcL == a1src );
		BOOST_CHECK( trgL == a1trg );
		BOOST_CHECK( srcL == a2trg );
		BOOST_CHECK( trgL == a2src );
		// check target_subalign
		for ( unsigned i = 0; i < trg.size(); ++i ) {
			for ( unsigned j = 0; j < trg.size(); ++j ) {
				Alignment sa = a.targetSubalign(a.targetLeftPosition+i,j,true);
				std::string st = sa.processSourceSubSequence(src.substr(sa.sourceLeftPosition,sa.sourceLength()));
				BOOST_CHECK(st == trg.substr(i,j));
			}
		}
	}
}

int test_main(int argc, char ** argv)
{
	std::string s1 = "ACGGCATCTGAA";
	std::string s2 = "ACGAAGCGAA";

	std::vector<std::string> c12;
	c12.push_back("3M6D4I3M");
	c12.push_back("3M2D1I3M1D3M");
	c12.push_back("3M2D1M1D2I1M1D3M");

	test_alignments(s1, s2, c12);

	// -------
	Alignment a;
	a.set( "78=", ReferenceId(2), true, 52, 0, std::vector<std::string>() );
	Alignment a2 = a.targetSubalign(0,78);
	BOOST_CHECK( a.sourceRegion() == a2.sourceRegion() );
	BOOST_CHECK( a.targetRegion() == a2.targetRegion() );
	BOOST_CHECK( a.sourceRefId == a2.sourceRefId );
	BOOST_CHECK( a.sourceStrandNegativ == a2.sourceStrandNegativ );
	BOOST_CHECK( a.cigarString() == a2.cigarString() );

	s1 = "ACGTGATCGAACGGTGCA";
	convertToReverseComplementary(s1);
	s2 = "ACTTGACGCACGCTGCA";
	a.set("2=1X3=1D2=1X3=1X4=", ReferenceId(3), true, s1, 0, s2, 0);
	StringPointer ss1(s1.data(),s1.size());
	a2 = a.reverse(ss1, ReferenceId(4));
	BOOST_CHECK( a2.cigarString(false) == "4=1X3=1X2=1I3=1X2=" );
	BOOST_CHECK(a.processSourceSubSequence(s1) == s2);
	BOOST_CHECK(a2.processSourceSubSequence(s2) == s1);

	return 0;
}
