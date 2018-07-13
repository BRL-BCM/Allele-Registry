#include "canonicalization.hpp"
#include <boost/lexical_cast.hpp>
#include <boost/test/minimal.hpp>
#include <memory>

ReferencesDatabase * db = nullptr;


void checkHgvs
	( std::string const & hgvs
	, std::string const & refName
	, unsigned left, unsigned right, std::string const & orgAllele
	, variantCategory category
	, unsigned lengthChange, std::string const & insertedSequence = "")
{
	std::cout << hgvs << " ... " << std::flush;
	HgvsVariant hgvsVar(true);
	decodeHgvs ( db, hgvs, hgvsVar );
	NormalizedGenomicVariant const var = canonicalize( db, hgvsVar.genomic );
	BOOST_CHECK( var.refId == db->getReferenceId(refName) );
	BOOST_CHECK( var.modifications.size() == 1 );
	NormalizedSequenceModification const & sm = var.modifications.front();
	BOOST_CHECK( sm.region.left()  == left  );
	BOOST_CHECK( sm.region.right() == right );
	BOOST_CHECK( sm.category == category );
	BOOST_CHECK( sm.lengthChange == lengthChange );
	BOOST_CHECK( sm.insertedSequence == insertedSequence );
	BOOST_CHECK( sm.originalSequence == orgAllele );
	std::cout << "OK" << std::endl;
}

void checkHgvs
	( std::string const & hgvs
	, unsigned left, unsigned right, std::string const & orgAllele
	, variantCategory category, std::string const & insertedSequence )
{
	checkHgvs(hgvs, hgvs.substr(0,hgvs.find_first_of(":")), left, right, orgAllele, category, 0, insertedSequence);
}

void checkHgvs
	( std::string const & hgvs
	, unsigned left, unsigned right, std::string const & orgAllele
	, variantCategory category
	, unsigned lengthChange, std::string const & insertedSequence = "")
{
	checkHgvs(hgvs, hgvs.substr(0,hgvs.find_first_of(":")), left, right, orgAllele, category, lengthChange, insertedSequence);
}

//void checkHgvs
//	( std::string const & hgvs
//	, std::string const & refName
//	, unsigned left, unsigned right, std::string const & orgAllele
//	, variantCategory category
//	, std::string const & insertedSequence = "")
//{
//	checkHgvs(hgvs, refName, left, right, orgAllele, category, 0, insertedSequence);
//}

int test_main(int argc, char ** argv)
{
	db = new ReferencesDatabase("./test_data_1");
	std::unique_ptr<ReferencesDatabase> scopePtr(db);

	{// ----- r3
		std::string const seq = "ATGCCGACCTCATTTGACCCTGACATTCGGCCTGTTCGGATTTGTTACATGTTGGTTGTG"
								"CATGGTCGAGCTATACGTCAGTTACGTCGCTTGTTGAAAGTTATTTACCATCGTGATCAT"
								"TATTATTACATCCACGTGGATAAAAGATCGGACTACCTGT";
		checkHgvs("r3:g.1A>T"      ,  0,  1, "A"  , nonShiftable     , "T"  );
		checkHgvs("r3:g.1_2insGCC" ,  1,  1, ""   , nonShiftable     , "GCC");
		checkHgvs("r3:g.12_13insTT", 12, 15, "TT" , duplication      , 2    );
		checkHgvs("r3:g.13_14del"  , 12, 15, "TT" , shiftableDeletion, 2    );
	}

	{// ----- t2, CDS: 10-40
		// ----- t2 -> r3-: 120-130 , 72-102 , 52-60
		// exon 120-130 |  0|  0| TGTAATAATA                      (tattattaca)
		// intron       | 10| 10| ATGATCACGATGGTAAAT              (atttaccatcgtgatcat)
		// exon 72-102  | 28| 10| AACTTTCAACAAGCGACGTAACTGACGTAT  (atacgtcagttacgtcgcttgttgaaagtt)
		// intron       | 58| 40| AGCTCGACCATG                    (catggtcgagct)
		// exon 52-60   | 70| 40| CACAACCA                        (tggttgtg)
		//              | 78| 48|

		checkHgvs("t2:c.-5T>G"  ,  5,  6, "T", nonShiftable, "G");
		checkHgvs("t2:c.-1+6C>G", 15, 16, "C", nonShiftable, "G");
		checkHgvs("t2:c.1-2A="  , 26, 27, "A", nonShiftable, "A");
		checkHgvs("t2:c.30+2G>A", 59, 60, "G", nonShiftable, "A");
		checkHgvs("t2:c.*1-1G>A", 69, 70, "G", nonShiftable, "A");
	}

	{// ------ p1
		std::string const seq = "MLPGRLCWVPLLLALGVGSGSGGGGDSRQRRLLAAKVNKHKPWIETSYHGVITENNDTVI"
								"LDPPLVALDKDAPVPFAGEICAFKIHGQELPFEAVVLNKTSGEGRLRAKSPIDCELQKEY"
								"TFIIQAYDCGAGPHETAWKKSHKAVVHIQVKDVNEFAPTFKEPAYKAVVTEGKIYDSILQ"
								"VEAIDEDCSPQYSQICNYEIVTTDVPFAIDRNGNIRNTEKLSYDKQHQYEILVTAYDCGQ";

	}

	// ========================= switch to real database
// we need to solve problem with shared memory to switch to other references database
//
//	db = new ReferencesDatabase("./referencesDatabase");
//	scopePtr.reset(db);
//
//	{
//
//	}

	return 0;
}
