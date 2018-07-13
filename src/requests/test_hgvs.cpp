#include "hgvs.hpp"
#include <boost/lexical_cast.hpp>
#include <boost/test/minimal.hpp>
#include <memory>

ReferencesDatabase * db = nullptr;

static std::string oneLettersCode2aminoAcid(std::string const & aa)
{
		if (aa == "A") return	"Ala";
		if (aa == "B") return	"Asx";
		if (aa == "C") return	"Cys";
		if (aa == "D") return	"Asp";
		if (aa == "E") return	"Glu";
		if (aa == "F") return	"Phe";
		if (aa == "G") return	"Gly";
		if (aa == "H") return	"His";
		if (aa == "I") return	"Ile";
		if (aa == "K") return	"Lys";
		if (aa == "L") return	"Leu";
		if (aa == "M") return	"Met";
		if (aa == "N") return	"Asn";
		if (aa == "P") return	"Pro";
		if (aa == "Q") return	"Gln";
		if (aa == "R") return	"Arg";
		if (aa == "S") return	"Ser";
		if (aa == "T") return	"Thr";
		if (aa == "V") return	"Val";
		if (aa == "W") return	"Trp";
		if (aa == "Y") return	"Tyr";
		if (aa == "Z") return	"Glx";
		if (aa == "*") return	"Ter";
	throw std::logic_error("Unknown AminoAcid: " + aa);
}

void checkGenomicSnv(std::string const & refName, unsigned hgvsPosition, char pOldAllele, char pNewAllele)
{
	std::string newAllele = "";
	newAllele += pNewAllele;
	std::string oldAllele = "";
	oldAllele += pOldAllele;
	std::string hgvs = "g." + boost::lexical_cast<std::string>(hgvsPosition) + oldAllele;
	if (pOldAllele != pNewAllele) {
		hgvs += ">" + newAllele;
	} else {
		hgvs += "=";
	}
	HgvsVariant decoded(true);
	decodeHgvs(db,  refName + ":" + hgvs, decoded);
	BOOST_CHECK( decoded.isGenomic() == true );
	BOOST_CHECK( decoded.genomic.refId == db->getReferenceId(refName) );
	// TODO - check context ref Id
	BOOST_CHECK( decoded.genomic.firstChromosome.size() == 1 );
	BOOST_CHECK( decoded.genomic.secondChromosome.size() == 0 );
	HgvsGenomicSequenceModification const & mod = decoded.genomic.firstChromosome.at(0);
	BOOST_CHECK( mod.newSequence.size() == 1 );
	BOOST_CHECK( mod.newSequence.at(0).sequence == newAllele );
	BOOST_CHECK( mod.newRepetitionsCount == 0 );
	BOOST_CHECK( mod.region == RegionCoordinates(hgvsPosition-1,hgvsPosition) );
	BOOST_CHECK( mod.originalSequence == oldAllele );
}

void checkProteinSnv(std::string const & refName, unsigned hgvsPosition, char pOldAllele, char pNewAllele)
{
	std::string newAllele = "";
	newAllele += pNewAllele;
	std::string oldAllele = "";
	oldAllele += pOldAllele;
	std::string hgvs = "p." + oneLettersCode2aminoAcid(oldAllele) + boost::lexical_cast<std::string>(hgvsPosition);
	if (oldAllele != newAllele) {
		hgvs += oneLettersCode2aminoAcid(newAllele);
	} else {
		hgvs += "=";
	}
	HgvsVariant decoded(true);
	decodeHgvs(db,  refName + ":" + hgvs, decoded);
	BOOST_CHECK( decoded.isGenomic() == false );
	BOOST_CHECK( decoded.protein.refId == db->getReferenceId(refName) );
	// TODO - check context ref Id
	BOOST_CHECK( decoded.protein.firstChromosome.size() == 1 );
	BOOST_CHECK( decoded.protein.secondChromosome.size() == 0 );
	HgvsProteinSequenceModification const & mod = decoded.protein.firstChromosome.at(0);
	BOOST_CHECK( mod.newSequence.size() == 1 );
	BOOST_CHECK( mod.newSequence.at(0).sequence == newAllele );
	BOOST_CHECK( mod.newRepetitionsCount == 0 );
	BOOST_CHECK( mod.region == RegionCoordinates(hgvsPosition-1,hgvsPosition) );
	BOOST_CHECK( mod.originalSequence == oldAllele || mod.originalSequence == "" );  //TODO - correct that
}

void checkSplicedSnv(std::string const & refName, unsigned hgvsPosition, char pNewAllele, std::string const & correctHgvsMod)
{
	std::string newAllele = "";
	newAllele += pNewAllele;
	HgvsVariant decoded(true);
	decodeHgvs(db, refName + ":" + correctHgvsMod, decoded );
	BOOST_CHECK( decoded.isGenomic() == true );
	BOOST_CHECK( decoded.genomic.refId == db->getReferenceId(refName) );
	// TODO - check context ref Id
	BOOST_CHECK( decoded.genomic.firstChromosome.size() == 1 );
	BOOST_CHECK( decoded.genomic.secondChromosome.size() == 0 );
	HgvsGenomicSequenceModification const & mod = decoded.genomic.firstChromosome.at(0);
	BOOST_CHECK( mod.newSequence.size() == 1 );
	BOOST_CHECK( mod.newSequence.at(0).sequence == newAllele );
	BOOST_CHECK( mod.newRepetitionsCount == 0 );
	BOOST_CHECK( mod.region == RegionCoordinates(hgvsPosition-1,hgvsPosition) );
	//BOOST_CHECK( mod.originalSequence == oldAllele );
}



int test_main(int argc, char ** argv)
{
	db = new ReferencesDatabase("./test_data_1");
	std::unique_ptr<ReferencesDatabase> scopePtr(db);

	{// ----- r3
		std::string const seq = "ATGCCGACCTCATTTGACCCTGACATTCGGCCTGTTCGGATTTGTTACATGTTGGTTGTG"
								"CATGGTCGAGCTATACGTCAGTTACGTCGCTTGTTGAAAGTTATTTACCATCGTGATCAT"
								"TATTATTACATCCACGTGGATAAAAGATCGGACTACCTGT";
		for (unsigned i = 0; i < seq.size(); ++i) {
			std::string const nucleo = "ACGT";
			for (char c: nucleo) checkGenomicSnv("r3", i+1, seq[i], c);
		}
	}

	{// ----- t2, CDS: 10-40
		// ----- t2 -> r3-: 120-130 , 72-102 , 52-60
		// exon 120-130 |  0|  0| TGTAATAATA                      (tattattaca)
		// intron       | 10| 10| ATGATCACGATGGTAAAT              (atttaccatcgtgatcat)
		// exon 72-102  | 28| 10| AACTTTCAACAAGCGACGTAACTGACGTAT  (atacgtcagttacgtcgcttgttgaaagtt)
		// intron       | 58| 40| AGCTCGACCATG                    (catggtcgagct)
		// exon 52-60   | 70| 40| CACAACCA                        (tggttgtg)
		//              | 78| 48|
		checkSplicedSnv("t2",  6, 'G', "c.-5T>G");
		checkSplicedSnv("t2", 16, 'G', "c.-1+6C>G");
		checkSplicedSnv("t2", 27, 'A', "c.1-2A=");
		checkSplicedSnv("t2", 60, 'A', "c.30+2G>A");
		checkSplicedSnv("t2", 70, 'A', "c.*1-1G>A");
	}

	{// ------ p1
		std::string const seq = "MLPGRLCWVPLLLALGVGSGSGGGGDSRQRRLLAAKVNKHKPWIETSYHGVITENNDTVI"
								"LDPPLVALDKDAPVPFAGEICAFKIHGQELPFEAVVLNKTSGEGRLRAKSPIDCELQKEY"
								"TFIIQAYDCGAGPHETAWKKSHKAVVHIQVKDVNEFAPTFKEPAYKAVVTEGKIYDSILQ"
								"VEAIDEDCSPQYSQICNYEIVTTDVPFAIDRNGNIRNTEKLSYDKQHQYEILVTAYDCGQ";
		for (unsigned i = 0; i < seq.size(); ++i) {
			std::string const aa = "ABCDEFGHIKLMNPQRSTVWYZ";
			for (char c: aa) if (seq[i] != 'X') checkProteinSnv("p1", i+1, seq[i], c);
		}
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

