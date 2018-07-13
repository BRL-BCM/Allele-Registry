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
	std::unique_ptr<ReferencesDatabase> scopePtr(new ReferencesDatabase("./test_data_1"));
	db = scopePtr.get();

	// ===================== main_genome.fasta
	checkReference("r1", ReferenceType::mainGenome);
	checkReference("r2", ReferenceType::mainGenome);
	checkReference("r3", ReferenceType::mainGenome);
	checkReference("r4", ReferenceType::mainGenome);

	checkSequence("r1",  11, "TCGAACACTAAGACGGTTTCTCCGGAAATGGAAAGCACTGGTTTACGCC");
	checkSequence("r2",   0, "GAAAATAAATTTGAAGAAATTCATTTTGCAACAGAGAAAGTACCAGAGATAATAGTCAAGTACCAACCAAAGTGTGACATCACAATTAAAGA");
	checkSequence("r3",   0, "ATGCCGACCTCATTTGACCCTGACATTCGGCCTGTTCGGATTTGTTACATGTTGGTTGTGCATGGTCGAGCTATACGTCAGTTACGTCGCTT"
						     "GTTGAAAGTTATTTACCATCGTGATCATTATTATTACATCCACGTGGATAAAAGATCGGACTACCTGT");
	checkSequence("r4", 342, "AATGGAGGAAGTGATTGGGTTGCATTGAACCGACGATTGTGTGACTTTGCTGTTAATGGGAACGATCAATTGCTCACACAACTTAAGCAC"
							 "TGGTATGAGTATACATTGTTACCTGCAGAATCATTTTTCCATACTTTGTCCCCTGACC");
// TODO - remove? (not supported for non-transcript)
//	checkCoordinates("r1",  10,  50, 10,'-',0,  50,'+',0);
//	checkCoordinates("r2",   0,   1,  0,'-',0,   1,'+',0);
//	checkCoordinates("r3",   0, 160,  0,'-',0, 160,'+',0);

	// ===================== trans_all.align
	checkReference("t1", ReferenceType::transcript);
	checkReference("t2", ReferenceType::transcript);
	checkReference("t3", ReferenceType::transcript);

	// ----- t1 -> r1+: 11-50 , 62-89 , 100-120
	// exon 11-50   |  0|  0| TCGAACACTAAGACGGTTTCTCCGGAAATGGAAAGCACT
	// intron       | 39| 39| GGTTTACGCCGT
	// exon 62-89   | 51| 39| ATCGTTTATACTCTTAATTCAGGCATT
	// intron       | 78| 66| TTTTACGTTTC
	// exon 100-120 | 89| 66| AATCAAGTCCAAACCTCATG
	//              |109| 86|
	checkSequence("t1",   0, "TCGAACACTAAGACGGTTTCTCCGGAAATGGAAAGCACTGGTTTACGCCGTATCGTTTATACTCTTAATTCAGGCATTTTTTACGTTTC"
							 "AATCAAGTCCAAACCTCATG");
	checkSequence("t1",  40, "GTTTACGCCGTATCGTTTATACTCTTAAT");
	checkSequence("t1",  39, "GGTTTACGCCGTATCGTTTATACTCTTAATTCAGGCATTTTTTACGTTTC");
	checkSequence("t1",  90, "ATCAAGTCCAAACCTCAT");
	checkCoordinates("t1",  0, 109,  0,'-',0, 86,'+',0);
	checkCoordinates("t1", 10,  80, 10,'-',0, 66,'+',2);
	checkCoordinates("t1", 39,  51, 39,'+',0, 39,'-',0);
	checkCoordinates("t1", 51,  89, 39,'-',0, 66,'-',0);
	checkCoordinates("t1", 40,  50, 39,'+',1, 39,'-',1);
	checkCoordinates("t1", 60, 100, 48,'-',0, 77,'+',0);

	// ----- t2 -> r3-: 120-130 , 72-102 , 52-60
	// exon 120-130 |  0|  0| TGTAATAATA                      (tattattaca)
	// intron       | 10| 10| ATGATCACGATGGTAAAT              (atttaccatcgtgatcat)
	// exon 72-102  | 28| 10| AACTTTCAACAAGCGACGTAACTGACGTAT  (atacgtcagttacgtcgcttgttgaaagtt)
	// intron       | 58| 40| AGCTCGACCATG                    (catggtcgagct)
	// exon 52-60   | 70| 40| CACAACCA                        (tggttgtg)
	//              | 78| 48|
	checkSequence("t2",  0, "TGTAATAATAATGATCACGATGGTAAATAACTTTCAACAAGCGACGTAACTGACGTATAGCTCGACCATGCACAACCA");
	checkSequence("t2",  5, "TAATAATGATCACGATGGTAAATAACTTTCAACAAGCGACGTAACTGACGTATAGCTCGACCATGCACAACCA");
	checkSequence("t2", 10, "ATGATCACGATGGTAAATAACTTTCAACAAGCGACGTAACTGACGTATAGCTCGACCATGCACAACCA");
	checkSequence("t2", 20, "TGGTAAATAACTTTCAACAAGCGACGTAACTGACGTATAGCTCGACCATGCACAACCA");
	checkSequence("t2", 28, "AACTTTCAACAAGCGACGTAACTGACGTATAGCTCGACCATGCACAACCA");
	checkSequence("t2", 50, "TGACGTATAGCTCGACCATGCACAACCA");
	checkSequence("t2", 58, "AGCTCGACCATGCACAACCA");
	checkSequence("t2", 70, "CACAACCA");
	checkSequence("t2", 77, "A");
	checkSequence("t2",  0, "TGTAATAATAATGATCACGATGGTAAATAACTTTCAACAAGCG");
	checkSequence("t2",  5, "TAATAATGATCACGATGGTAAATAA");
	checkSequence("t2", 10, "ATGATCACGATGGTAAAT");
	checkSequence("t2", 20, "TGGTAAAT");
	checkSequence("t2", 28, "AACTTTCAACAAGCGACGTAACTGACGTAT");
	checkSequence("t2", 50, "TGACGTATAGCTCGACCA");
	checkSequence("t2", 58, "AGCTCGACCATG");
	checkSequence("t2", 69, "GC");
	checkCoordinates("t2",  0, 78,  0,'-',0, 48,'+',0);
	checkCoordinates("t2",  5, 60,  5,'-',0, 40,'+',2);
	checkCoordinates("t2", 10, 28, 10,'+',0, 10,'-',0);
	checkCoordinates("t2", 30, 60, 12,'-',0, 40,'+',2);
	checkCoordinates("t2", 20, 68, 10,'-',8, 40,'-',2);

	// ----- t3 -> r2+: 0-20(3I6=2X2=4D3=1X2=,TTA,CC,G), 200-230(6=1X8=5I10=1D3=1X,T,CTAAG,C)
	// exon 0-20    |  0|  0| (/TTA)GAAAAT(AA/CC)AT(TTGA/)AGA(A/G)AT
	// intron       | 19| 19| TCATTTTGCAACAGAGAAAGTACCAGAGATAATAGTCAAGTACCAACCAAAGTGTGACATCACAATTAAAGATTCTATATCAGCGTTGTCTCGAGCAACA
	//                        ACTGATCGATGTAAACAACAAATTGCGGACGCTGCTTGCAAAATGCAAGATGGTACATTGTTTCCAAAATCTATGCCAAG
	// exon 200-230 |199| 19| AACGTG(C/T)AAACATGA(/CTAAG)GTCAAAATTT(A/)CAT(T/C)
	//              |233| 53|
	std::string const transcriptSeq = "TTAGAAAATCCATAGAGAT"
							   "TCATTTTGCAACAGAGAAAGTACCAGAGATAATAGTCAAGTACCAACCAAAGTGTGACATCACAATTAAAGATTCTATATCAGCGTTGTCTCGAGCAACA"
							   "ACTGATCGATGTAAACAACAAATTGCGGACGCTGCTTGCAAAATGCAAGATGGTACATTGTTTCCAAAATCTATGCCAAG"
							   "AACGTGTAAACATGACTAAGGTCAAAATTTCATC";

	for (unsigned ib = 0; ib < transcriptSeq.size(); ++ib) {
		for (unsigned ie = ib+1; ie < transcriptSeq.size(); ++ie) checkSequence("t3", ib, transcriptSeq.substr(ib,ie-ib));
	}
	checkCoordinates("t3",   0, 233,  0,'-', 0, 53,'+',0);
	checkCoordinates("t3", 100, 200, 19,'+',81, 20,'+',0);
	checkCoordinates("t3",  19, 199, 19,'+', 0, 19,'-',0);

	// ----- t4 - TODO strand- & alignments


	// TODO - getAlignments(From/To)MainGenome

	// ===================== proteins_all.fasta
	checkReference("p1", ReferenceType::protein);
	checkReference("p2", ReferenceType::protein);

	checkSequence("p1", 60, "LDPPLVALDKDAPVPFAGEICAFKIHGQELPFEAVVLNKTSGEGRLRAKSPIDCELQKEY"
							"TFIIQAYDCGAGPHETAWKKSHKAVVHIQVKDVNEFAPTFKEPAYKAVVTEGKIYDSILQ");
	checkSequence("p2", 0, "LVMEGDDIGNINRALQKVSYINSRQFPTAGVRRLKVSSKVQCFGKLCGASSGIIDLLPSPSAATNWTAGLLVDS");

	return 0;
}
