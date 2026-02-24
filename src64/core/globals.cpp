#include "globals.hpp"
#include "../commonTools/parseTools.hpp"
#include <boost/lexical_cast.hpp>

ReferenceId const ReferenceId::null;
CanonicalId const CanonicalId::null;
RegionCoordinates const RegionCoordinates::null;


/*	--------------------------------------
	Symbol    Meaning      Nucleic Acid
	--------------------------------------
	A            A           Adenine
	C            C           Cytosine
	G            G           Guanine
	T            T           Thymine
	U            U           Uracil
	M          A or C
	R          A or G        Purine
	W          A or T
	S          C or G
	Y          C or T        Pyrimidine
	K          G or T
	V        A or C or G
	H        A or C or T
	D        A or G or T
	B        C or G or T
	X      G or A or T or C
	N      G or A or T or C
*/
void convertToReverseComplementary(std::string & seq)
{
	for (char & c: seq) {
		switch (c) {
		case 'A': c = 'T'; break;
		case 'C': c = 'G'; break;
		case 'G': c = 'C'; break;
		case 'T': c = 'A'; break;
		case 'M': c = 'K'; break;
		case 'R': c = 'Y'; break;
		case 'W': break;
		case 'S': break;
		case 'Y': c = 'R'; break;
		case 'K': c = 'M'; break;
		case 'V': c = 'B'; break;
		case 'H': c = 'D'; break;
		case 'D': c = 'H'; break;
		case 'B': c = 'V'; break;
		case 'N': break;
		case 'X': break;
		default: throw std::logic_error("Unknown base-pair");
		}
	}
	std::reverse(seq.begin(),seq.end());
}

void convertToUpperCase(std::string & str)
{
	for (auto & c: str) {
		if (c >= 'a' && c <= 'z') c -= ('a' - 'A');
	}
}

// -----------------------------
//
//std::string toString(NucleotideChangeType v)
//{
//	switch (v) {
//		case nctDeletion:             return "SO:0000159^deletion";
//		case nctInsertion:            return "SO:0000667^insertion";
//		case nctInversion:            return "SO:1000036^inversion";
//		case nctTranslocation:        return "SO:0000199^translocation";
//		case nctIndel:                return "SO:1000032^indel";
//		case nctCopyNumberVariation:  return "SO:0001019^copy_number_variation";
//		case nctSubstitution:         return "SO:1000002^substitution";
//		case nctNoSequenceAlteration: return "SO:000000^no_sequence_alteration";
//	}
//	throw std::logic_error("Unknown value of NucleotideChangeType (toString)");
//}
//
//NucleotideChangeType parseNucleotideChangeType(std::string const & v)
//{
//	if (v == "SO:0000159^deletion"             ) return nctDeletion;
//	if (v == "SO:0000667^insertion"            ) return nctInsertion;
//	if (v == "SO:1000036^inversion"            ) return nctInversion;
//	if (v == "SO:0000199^translocation"        ) return nctTranslocation;
//	if (v == "SO:1000032^indel"                ) return nctIndel;
//	if (v == "SO:0001019^copy_number_variation") return nctCopyNumberVariation;
//	if (v == "SO:1000002^substitution"         ) return nctSubstitution;
//	if (v == "SO:000000^no_sequence_alteration") return nctNoSequenceAlteration;
//	throw std::logic_error("Unknown value of NucleotideChangeType (parse)");
//}

// -----------------------------
//
//std::string toString(ReferenceSequenceType v)
//{
//	switch (v) {
//		case rstChromosome    : return "chromosome";
//		case rstTranscript    : return "transcript";
//		case rstGenomicRegion : return "gene";
//		case rstAlternateLocus: return "chromosome";
//		case rstAminoAcid     : return "amino-acid";
//	}
//	throw std::logic_error("Unknown value of ReferenceSequenceType (toString)");
//}
//
//ReferenceSequenceType parseReferenceSequenceType(std::string const & v)
//{
//	if (v == "chromosome") return rstChromosome;
//	if (v == "transcript") return rstTranscript;
//	if (v == "gene"      ) return rstGenomicRegion;
//	if (v == "amino-acid") return rstAminoAcid;
//	throw std::logic_error("Unknown value of ReferenceSequenceType (parse)");
//}

// -----------------------------
//
//std::string toString(CanonicalAlleleType v)
//{
//	switch (v) {
//		case catNucleotide: return "nucleotide";
//		case catAminoAcid : return "amino-acid";
//	}
//	throw std::logic_error("Unknown value of CanonicalAlleleType (toString)");
//}
//
//CanonicalAlleleType parseCanonicalAlleleType(std::string const & v)
//{
//	if (v == "nucleotide") return catNucleotide;
//	if (v == "amino-acid") return catAminoAcid;
//	throw std::logic_error("Unknown value of CanonicalAlleleType (parse)");
//}

// ---------------------
//
std::string toString(ReferenceGenome v)
{
	switch (v) {
		case rgNCBI36: return "NCBI36";
		case rgGRCh37: return "GRCh37";
		case rgGRCh38: return "GRCh38";
		default: throw std::logic_error("Unknown value of ReferenceGenome (toString)");
	}
}

ReferenceGenome parseReferenceGenome(std::string const & v)
{
	if (v == "NCBI36") return rgNCBI36;
	if (v == "GRCh37") return rgGRCh37;
	if (v == "GRCh38") return rgGRCh38;
	throw std::logic_error("Unknown value of ReferenceGenome (parse)");
}

// ---------------------
//
std::string toString(Chromosome v)
{
	switch (v) {
		case chrUnknown: throw std::logic_error("Unknown value of Chromosome (toString)");
		case chrX: return "X";
		case chrY: return "Y";
		case chrM: return "MT";
		default: return boost::lexical_cast<std::string>(static_cast<int>(v));
	}
}

Chromosome parseChromosome(std::string const & v)
{
	if (v == "X") return chrX;
	if (v == "Y") return chrY;
	if (v == "MT") return chrM;
	if (v.size() > 0 && v[0] >= '0' && v[0] <= '9') {
		if (v.size() == 1 || (v.size() == 2 && v[1] >= '0' && v[1] <= '9')) {
			int i = boost::lexical_cast<int>(v);
			if (i > 0 && i < 23) {
				return static_cast<Chromosome>(i);
			}
		}
	}
	throw std::logic_error("Unknown value of Chromosome (parse)");
}

void parse(char const *& ptr, Chromosome & v)
{
	unsigned n;
	if ( tryParse(ptr, n) ) {
		if (n < 1 || n > 22) throw std::runtime_error("Incorrect chromosome number: " + boost::lexical_cast<std::string>(n));
		v = static_cast<Chromosome>(n);
	} else if ( tryParse(ptr,"X") ) {
		v = Chromosome::chrX;
	} else if ( tryParse(ptr,"Y") ) {
		v = Chromosome::chrY;
	} else if ( tryParse(ptr,"MT") ) {
		v = Chromosome::chrM;
	} else {
		throw std::runtime_error("Cannot parse chromosome.");
	}
}

// ---------------------
//
//std::string toString(ReferenceIdentifierType v)
//{
//	switch (v) {
//		case ritNCBI   : return "http://www.ncbi.nlm.nih.gov/nuccore/";
//		case ritLRG    : return "http://www.lrg-sequence.org/";
//		case ritEnsembl: return "http://ensembl.org";
//		case ritGenBank: return "http://www.ncbi.nlm.nih.gov/genbank/";
//	}
//	throw std::logic_error("Unknown value of ReferenceIdentifierType (toString)");
//}
//
//ReferenceIdentifierType parseReferenceIdentifierType(std::string const & v)
//{
//	if (v == "http://www.ncbi.nlm.nih.gov/nuccore/") return ritNCBI;
//	if (v == "http://www.lrg-sequence.org/"        ) return ritLRG;
//	if (v == "http://ensembl.org"                  ) return ritEnsembl;
//	if (v == "http://www.ncbi.nlm.nih.gov/genbank/") return ritGenBank;
//	throw std::logic_error("Unknown value of ReferenceIdentifierType (parse)");
//}

// ---------------------------------------

std::string SplicedCoordinate::toString() const
{
	std::string s = boost::lexical_cast<std::string>(fPosition);
	s += offsetSign();
	s += boost::lexical_cast<std::string>(fOffset);
	return s;
}

std::string RegionCoordinates::toString() const { return ("[" + boost::lexical_cast<std::string>(left()) + "," + boost::lexical_cast<std::string>(right()) + ")"); }

std::string SplicedRegionCoordinates::toString() const { return ("[" + fLeftPosition.toString() + "," + fRightPosition.toString() + ")"); }


//std::string GeneralRegionCoordinates::toString() const
//{
//	std::string s = "[" + boost::lexical_cast<std::string>(leftPosition);
//	if (this->leftIsIntronic()) s += (leftExternalOffsetNegative ? "-" : "+") + boost::lexical_cast<std::string>(leftExternalOffset);
//	s += "," + boost::lexical_cast<std::string>(rightPosition);
//	if (this->rightIsIntronic()) s += (rightExternalOffsetNegative ? "-" : "+") + boost::lexical_cast<std::string>(rightExternalOffset);
//	s += ")";
//	return s;
//}

