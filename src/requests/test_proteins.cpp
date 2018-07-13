#include "proteins.hpp"
#include "canonicalization.hpp"
#include <boost/lexical_cast.hpp>
#include <boost/test/minimal.hpp>
#include <memory>

ReferencesDatabase * db = nullptr;

void checkProtein
		( std::string const & transcriptHgvs
		, std::string const & proteinHgvs
		, std::string const & proteinHgvsWellDefined
		)
{
	std::cout << "Test " << transcriptHgvs << std::endl;
	HgvsVariant decodedHgvs(true);
	decodeHgvs ( db, transcriptHgvs, decodedHgvs );
	BOOST_CHECK( decodedHgvs.isGenomic() );
	std::cout << "canonicalize..." << std::endl;
	NormalizedGenomicVariant transVar = canonicalize(db, decodedHgvs.genomic);
	std::string outProteinHgvs = "xxx";
	std::string outProteinHgvsWellDefined = "yyy";
	std::cout << "calculate protein..." << std::endl;
	calculateProteinVariation( db, transVar, outProteinHgvs, outProteinHgvsWellDefined );
	std::cout << proteinHgvs << " == " << outProteinHgvs << std::endl;
	std::cout << proteinHgvsWellDefined << " == " << outProteinHgvsWellDefined << std::endl;
	BOOST_CHECK( proteinHgvs == outProteinHgvs );
	BOOST_CHECK( proteinHgvsWellDefined == outProteinHgvsWellDefined );
}


int test_main(int argc, char ** argv)
{
	db = new ReferencesDatabase("./test_data_1");
	std::unique_ptr<ReferencesDatabase> scopePtr(db);

	// ----- t2 -> r3-: 120-130 , 72-102 , 52-60
	// exon 120-130 |  0|  0| TGTAATAATA                      (tattattaca)
	// intron       | 10| 10| ATGATCACGATGGTAAAT              (atttaccatcgtgatcat)
	// exon 72-102  | 28| 10| AACTTTCAACAAGCGACGTAACTGACGTAT  (atacgtcagttacgtcgcttgttgaaagtt)
	// intron       | 58| 40| AGCTCGACCATG                    (catggtcgagct)
	// exon 52-60   | 70| 40| CACAACCA                        (tggttgtg)
	//              | 78| 48|
	// -----
	// CDS: 10-40
	// AACTTTCAACAAGCGACGTAACTGACGTAT
	//   N  F  Q  Q  A  T  *  L  T  Y

	ReferenceId p2(db->getReferenceId("p2"));
	BOOST_CHECK( db->getSequence(p2,RegionCoordinates(0,db->getMetadata(p2).length)) == "NFQQAT*LTY" );

	checkProtein("t2:c.6T>C", "p2:p.Phe2="  , "");
	checkProtein("t2:c.7C>A", "p2:p.Gln3Lys", "p2:p.Gln3Lys");

	// AACTTTCAACAAGCGCGTAACTGACGTAT
	//   N  F  Q  Q  A  R  N  *  R
	checkProtein("t2:c.16delA", "p2:p.Thr6ArgfsTer3", "p2:p.Thr6_Tyr10delinsArgAsn");

	// AACTCAAAAGCGACGTAACTGACGTAT
	//   N  S  K  A  T  *  L  T  Y
	checkProtein("t2:c.[4del;6del;10del]", "p2:p.Phe2_Gln4delinsSerLys", "p2:p.Phe2_Gln4delinsSerLys");
	// AACTCAAAAGCGACGTAACTGACGTAAT
	//   N  S  K  A  T  *  L  T  *
	checkProtein("t2:c.[4del;6del;10del;28_29insA]", "p2:p.[Phe2_Gln4delinsSerLys;Tyr10Ter]", "p2:p.[Phe2_Gln4delinsSerLys;Tyr10del]");


	return 0;
}

