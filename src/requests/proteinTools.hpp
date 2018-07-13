#ifndef REQUESTS_PROTEINTOOLS_HPP_
#define REQUESTS_PROTEINTOOLS_HPP_

#include "../core/exceptions.hpp"

enum AminoAcid {
	  aaAlanine        = 0
	, aaCysteine
	, aaAsparticAcid
	, aaGlutamicAcid
	, aaPhenylalanine
	, aaGlycine
	, aaHistidine
	, aaIsoleucine
	, aaLysine
	, aaLeucine
	, aaMethionine
	, aaAsparagine
	, aaProline
	, aaGlutamine
	, aaArginine
	, aaSerine
	, aaThreonine
	, aaValine
	, aaTryptophan
	, aaTyrosine
	, aaStopCodon
	, aaAsparagineOrAsparticAcid
	, aaGlutamineOrGlutamicAcid
	, aaUnknown
};


inline std::string aminoAcid2threeLettersCode(AminoAcid aa)
{
	switch (aa) {
		case aaAlanine      : return "Ala";
		case aaAsparagineOrAsparticAcid   : return "Asx";
		case aaCysteine     : return "Cys";
		case aaAsparticAcid : return "Asp";
		case aaGlutamicAcid : return "Glu";
		case aaPhenylalanine: return "Phe";
		case aaGlycine      : return "Gly";
		case aaHistidine    : return "His";
		case aaIsoleucine   : return "Ile";
		case aaLysine       : return "Lys";
		case aaLeucine      : return "Leu";
		case aaMethionine 	: return "Met";
		case aaAsparagine 	: return "Asn";
		case aaProline 	    : return "Pro";
		case aaGlutamine 	: return "Gln";
		case aaArginine 	: return "Arg";
		case aaSerine       : return "Ser";
		case aaThreonine    : return "Thr";
		case aaValine       : return "Val";
		case aaTryptophan   : return "Trp";
		case aaTyrosine     : return "Tyr";
		case aaGlutamineOrGlutamicAcid    : return "Glx";
		case aaStopCodon    : return "Ter";
		case aaUnknown      : return "X";
	}
	throw ExceptionHgvsParsingError("Unknown AminoAcid: " + boost::lexical_cast<std::string>(static_cast<int>(aa)));
}

inline std::string aminoAcid2oneLettersCode(AminoAcid aa)
{
	switch (aa) {
		case aaAlanine      : return "A";
		case aaAsparagineOrAsparticAcid   : return "B";
		case aaCysteine     : return "C";
		case aaAsparticAcid : return "D";
		case aaGlutamicAcid : return "E";
		case aaPhenylalanine: return "F";
		case aaGlycine      : return "G";
		case aaHistidine    : return "H";
		case aaIsoleucine   : return "I";
		case aaLysine       : return "K";
		case aaLeucine      : return "L";
		case aaMethionine 	: return "M";
		case aaAsparagine 	: return "N";
		case aaProline 	    : return "P";
		case aaGlutamine 	: return "Q";
		case aaArginine 	: return "R";
		case aaSerine       : return "S";
		case aaThreonine    : return "T";
		case aaValine       : return "V";
		case aaTryptophan   : return "W";
		case aaTyrosine     : return "Y";
		case aaGlutamineOrGlutamicAcid    : return "Z";
		case aaStopCodon    : return "*";
		case aaUnknown      : return "X";
	}
	throw ExceptionHgvsParsingError("Unknown AminoAcid: " + boost::lexical_cast<std::string>(static_cast<int>(aa)));
}

inline AminoAcid threeLettersCode2aminoAcid(std::string const & aa)
{
	if (aa == "Ala") return	aaAlanine;
	if (aa == "Asx") return	aaAsparagineOrAsparticAcid;
	if (aa == "Cys") return	aaCysteine;
	if (aa == "Asp") return	aaAsparticAcid;
	if (aa == "Glu") return	aaGlutamicAcid;
	if (aa == "Phe") return	aaPhenylalanine;
	if (aa == "Gly") return	aaGlycine;
	if (aa == "His") return	aaHistidine;
	if (aa == "Ile") return	aaIsoleucine;
	if (aa == "Lys") return	aaLysine;
	if (aa == "Leu") return	aaLeucine;
	if (aa == "Met") return	aaMethionine;
	if (aa == "Asn") return	aaAsparagine;
	if (aa == "Pro") return	aaProline;
	if (aa == "Gln") return	aaGlutamine;
	if (aa == "Arg") return	aaArginine;
	if (aa == "Ser") return	aaSerine;
	if (aa == "Thr") return	aaThreonine;
	if (aa == "Val") return	aaValine;
	if (aa == "Trp") return	aaTryptophan;
	if (aa == "X"  ) return	aaUnknown;
	if (aa == "Tyr") return	aaTyrosine;
	if (aa == "Glx") return	aaGlutamineOrGlutamicAcid;
	if (aa == "*" || aa == "Ter" ) return	aaStopCodon;
	throw ExceptionHgvsParsingError("Unknown AminoAcid: " + aa);
}

inline AminoAcid oneLettersCode2aminoAcid(std::string const & aa)
{
		if (aa == "A") return	aaAlanine;
		if (aa == "B") return	aaAsparagineOrAsparticAcid;
		if (aa == "C") return	aaCysteine;
		if (aa == "D") return	aaAsparticAcid;
		if (aa == "E") return	aaGlutamicAcid;
		if (aa == "F") return	aaPhenylalanine;
		if (aa == "G") return	aaGlycine;
		if (aa == "H") return	aaHistidine;
		if (aa == "I") return	aaIsoleucine;
		if (aa == "K") return	aaLysine;
		if (aa == "L") return	aaLeucine;
		if (aa == "M") return	aaMethionine;
		if (aa == "N") return	aaAsparagine;
		if (aa == "P") return	aaProline;
		if (aa == "Q") return	aaGlutamine;
		if (aa == "R") return	aaArginine;
		if (aa == "S") return	aaSerine;
		if (aa == "T") return	aaThreonine;
		if (aa == "V") return	aaValine;
		if (aa == "W") return	aaTryptophan;
		if (aa == "X") return	aaUnknown;
		if (aa == "Y") return	aaTyrosine;
		if (aa == "Z") return	aaGlutamineOrGlutamicAcid;
		if (aa == "*") return	aaStopCodon;
	throw ExceptionHgvsParsingError("Unknown AminoAcid: " + aa);
}

inline std::string convertOneLetterAminoAcidSequenceToThreeLetters(std::string const & aa)
{
	std::string r = "";
	for (char c: aa) {
		std::string temp = "X";
		temp[0] = c;
		r += aminoAcid2threeLettersCode(oneLettersCode2aminoAcid(temp));
	}
	return r;
}

inline std::string translateToAminoAcid(std::string const & seq)
{
    std::string res = "";
    for ( unsigned i = 0;  i+3 <= seq.size();  i+=3 ) {
        std::string const c = seq.substr(i,3);
        // -----
        if (c == "TTT" || c == "TTC") res += "F";
        if (c == "TTA" || c == "TTG") res += "L";
        if (c == "CTT" || c == "CTC") res += "L";
        if (c == "CTA" || c == "CTG") res += "L";
        if (c == "ATT" || c == "ATC" || c == "ATA") res += "I";
        if (c == "ATG") res += "M";
        if (c == "GTT" || c == "GTC") res += "V";
        if (c == "GTA" || c == "GTG") res += "V";
        // -----
        if (c == "TCT" || c == "TCC") res += "S";
        if (c == "TCA" || c == "TCG") res += "S";
        if (c == "CCT" || c == "CCC") res += "P";
        if (c == "CCA" || c == "CCG") res += "P";
        if (c == "ACT" || c == "ACC") res += "T";
        if (c == "ACA" || c == "ACG") res += "T";
        if (c == "GCT" || c == "GCC") res += "A";
        if (c == "GCA" || c == "GCG") res += "A";
        // -----
        if (c == "TAT" || c == "TAC") res += "Y";
        if (c == "TAA" || c == "TAG") res += "*";
        if (c == "CAT" || c == "CAC") res += "H";
        if (c == "CAA" || c == "CAG") res += "Q";
        if (c == "AAT" || c == "AAC") res += "N";
        if (c == "AAA" || c == "AAG") res += "K";
        if (c == "GAT" || c == "GAC") res += "D";
        if (c == "GAA" || c == "GAG") res += "E";
        // -----
        if (c == "TGT" || c == "TGC") res += "C";
        if (c == "TGA") res += "*";
        if (c == "TGG") res += "W";
        if (c == "CGT" || c == "CGC") res += "R";
        if (c == "CGA" || c == "CGG") res += "R";
        if (c == "AGT" || c == "AGC") res += "S";
        if (c == "AGA" || c == "AGG") res += "R";
        if (c == "GGT" || c == "GGC") res += "G";
        if (c == "GGA" || c == "GGG") res += "G";
    }
    return res;
}

inline std::string applyVariantsToTheSequence
		( std::string const & sequence  // original sequence
		, unsigned const      offset    // offset of the given sequence
		, std::vector<PlainSequenceModification> const & variants // modifications to apply (must be sorted & must belong to the sequence)
		)
{
	ASSERT( ! variants.empty() );
	ASSERT( offset <= variants.front().region.left() );
	ASSERT( offset + sequence.size() >= variants.back().region.right() );
	std::string seq2 = "";
	unsigned lastRight = 0;
	for (auto & psm: variants) {
		seq2 += sequence.substr( lastRight, psm.region.left() - offset - lastRight );
		seq2 += psm.newSequence;
		lastRight = psm.region.right() - offset;
	}
	if ( seq2.empty() || oneLettersCode2aminoAcid(seq2.substr(seq2.size()-1)) != aaStopCodon ) {
		// add the rest of sequence <=> the last character of inserted sequence is not a stop codon
		seq2 += sequence.substr(lastRight);
	} else {
		// remove stop codon
		seq2.pop_back();
	}
	return seq2;
}

inline std::string applyVariantToTheSequence
		( std::string const & sequence  // original sequence
		, unsigned const      offset    // offset of the given sequence
		, PlainSequenceModification const & variant // modifications to apply (must be sorted & must belong to the sequence)
		)
{
	ASSERT( offset <= variant.region.left() );
	ASSERT( offset + sequence.size() >= variant.region.right() );
	std::string seq2 = sequence.substr( 0, variant.region.left() - offset );
	seq2 += variant.newSequence;
	seq2 += sequence.substr(variant.region.right() - offset);
	return seq2;
}


#endif /* REQUESTS_PROTEINTOOLS_HPP_ */
