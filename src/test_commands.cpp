#include "commands.hpp"
#include <boost/lexical_cast.hpp>
#include <boost/test/minimal.hpp>
#include <memory>


ReferencesDatabase * db = nullptr;


bool operator==(Allele const & a1, Allele const & a2)
{
	if (a1.newSequence != a2.newSequence) return false;
	if (a1.originalSequence != a2.originalSequence) return false;
	if (a1.region != a2.region) return false;
	return true;
}

bool operator==(AllelesSequence const & a1, AllelesSequence const & a2 )
{
	if ( ! (a1.alleles == a2.alleles) ) return false;
	if (a1.hgvsDefinitions != a2.hgvsDefinitions) return false;
	if (a1.refCarId != a2.refCarId) return false;
	if (a1.refId != a2.refId) return false;
	if (a1.refNames != a2.refNames) return false;
	return true;
}

AllelesSequence chooseAlleleFromRef(ReferenceId refId, std::vector<AllelesSequence> const & v)
{
	std::vector<AllelesSequence> v2;
	for (auto const & a: v) {
		if (a.refId == refId) v2.push_back(a);
	}
	BOOST_CHECK( v2.size() == 1 );
	return v2[0];
}

void checkInsertion( std::string const & refName, unsigned position, std::string const & insertion  // input
				   , unsigned outPosition, std::string const & outInsertion  // output - left aligned
				   , std::string const & hgvs  // hgvs - right aligned
				   )
{
	Allele inA;
	inA.region = SplicedRegionCoordinates(RegionCoordinates(position,position));
	inA.newSequence = insertion;
	Allele outA;
	outA.region = SplicedRegionCoordinates(RegionCoordinates(outPosition,outPosition));
	outA.newSequence = outInsertion;
	outA.originalSequence = "";

	std::vector<AllelesSequence> output1;
	commandMapAndCanonicalizeVariance( db, refName, std::vector<Allele>({inA}), output1 );
	AllelesSequence output = chooseAlleleFromRef( db->getReferenceId(refName), output1 );
	BOOST_CHECK( output.alleles.size() == 1);
	BOOST_CHECK( output.alleles[0] == outA );
	BOOST_CHECK( std::find(output.hgvsDefinitions.begin(),output.hgvsDefinitions.end(),hgvs) != output.hgvsDefinitions.end() );

	std::vector<AllelesSequence> output2;
	commandMapAndCanonicalizeHgvs( db, refName + ":" + hgvs, output2 );
	BOOST_CHECK(output1 == output2);
}

void checkDeletion( std::string const & refName, unsigned leftPosition, unsigned rightPosition  // input
				  , unsigned outLeftPosition  // output - left aligned
				  , std::string const & hgvs  // hgvs - right aligned
				  )
{
	Allele inA;
	inA.region = SplicedRegionCoordinates(RegionCoordinates(leftPosition,rightPosition));
	inA.newSequence = "";
	Allele outA;
	outA.region = SplicedRegionCoordinates(RegionCoordinates(outLeftPosition,outLeftPosition+(rightPosition-leftPosition)));
	outA.newSequence = "";
	outA.originalSequence = db->getSequence(db->getReferenceId(refName),outA.region.toRegion());

	std::vector<AllelesSequence> output1;
	commandMapAndCanonicalizeVariance( db, refName, std::vector<Allele>({inA}), output1 );
	AllelesSequence output = chooseAlleleFromRef( db->getReferenceId(refName), output1 );
	BOOST_CHECK( output.alleles.size() == 1);
	BOOST_CHECK( output.alleles[0] == outA );
	BOOST_CHECK( std::find(output.hgvsDefinitions.begin(),output.hgvsDefinitions.end(),hgvs) != output.hgvsDefinitions.end() );

	std::vector<AllelesSequence> output2;
	commandMapAndCanonicalizeHgvs( db, refName + ":" + hgvs, output2 );
	BOOST_CHECK(output1 == output2);
}

int test_main(int argc, char ** argv)
{
	db = new ReferencesDatabase("./test_data_1");
	std::unique_ptr<ReferencesDatabase> scopePtr(db);


	{// ----- r1, region 100-180
		// 100-120 AATCAAGTCCAAACCTCATG
		// 120-140 GAAGAAGAACATTTACGAAG
		// 140-160 ATTAAAAGAACTTCAGATTA
		// 160-180 AAAAACATCAAGAGCTTGCC

		checkInsertion("r1", 124,  "CTC", 124,  "CTC", "g.124_125insCTC");
		checkInsertion("r1", 124,   "AG", 122,   "AG", "g.124_125dupGA");
		checkInsertion("r1", 162, "AATA", 161, "AAAT", "g.164_165insTAAA");
		checkInsertion("r1", 128,  "AGA", 120,  "GAA", "g.127_129dupGAA");
		checkInsertion("r1", 160,    "A", 159,    "A", "g.165dupA");

		checkDeletion("r1", 103, 104, 103, "g.104del");
		checkDeletion("r1", 123, 126, 120, "g.127_129del");
		checkDeletion("r1", 125, 127, 125, "g.127_128del");
		checkDeletion("r1", 144, 145, 143, "g.147del");
	}

	return 0;
}
