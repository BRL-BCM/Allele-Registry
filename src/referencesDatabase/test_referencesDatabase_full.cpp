#include "referencesDatabase.hpp"
#include <boost/test/minimal.hpp>
#include <memory>


ReferencesDatabase * db = nullptr;

enum ReferenceType {
	  mainGenome
	, geneRegion
	, transcript
	, protein
};

void checkReference(std::string const & refName, ReferenceType refType)
{
	ReferenceId id = db->getReferenceId(refName);
	std::vector<std::string> names = db->getNames(id);
	BOOST_CHECK( std::find(names.begin(),names.end(),refName) != names.end() );
	BOOST_CHECK( db->isProteinReference(id) == (refType == ReferenceType::protein) );
	BOOST_CHECK( db->isSplicedRefSeq(id) == (refType == ReferenceType::transcript) );
}

void checkSequence(std::string const & refName, unsigned startPosition, std::string sequence)
{
	ReferenceId id = db->getReferenceId(refName);
	std::string const outSequence = db->getSequence( id, RegionCoordinates(startPosition,startPosition+sequence.size()) );
	BOOST_CHECK( outSequence == sequence );
}

void checkCoordinates( std::string const & refName, unsigned unsplLeft, unsigned unsplRight
					 , unsigned splLeftPos , char splLeftSign , unsigned splLeftOffset
					 , unsigned splRightPos, char splRightSign, unsigned splRightOffset )
{
	ReferenceId const id = db->getReferenceId(refName);
	SplicedCoordinate const splLeft  = SplicedCoordinate(splLeftPos , splLeftSign , splLeftOffset );
	SplicedCoordinate const splRight = SplicedCoordinate(splRightPos, splRightSign, splRightOffset);
	SplicedRegionCoordinates const calcSpl = db->convertToSplicedRegion(id, RegionCoordinates(unsplLeft, unsplRight));
	RegionCoordinates const calcUnspl = db->convertToUnsplicedRegion(id, SplicedRegionCoordinates(splLeft,splRight));
	BOOST_CHECK( calcUnspl == RegionCoordinates(unsplLeft, unsplRight) );
	BOOST_CHECK( calcSpl == SplicedRegionCoordinates(splLeft,splRight) );
}

int test_main(int argc, char ** argv)
{
	std::unique_ptr<ReferencesDatabase> scopePtr(new ReferencesDatabase("../dbReferences"));
	db = scopePtr.get();


	checkSequence("ENSP00000411603.1",  0, "MDRCKHVGRLRLAQD");
	checkSequence("ENSP00000478244.1",  0, "XWK");

	return 0;
}
