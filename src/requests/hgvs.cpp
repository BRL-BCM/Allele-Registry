#include <sstream>
#include <stdexcept>
#include <cstring>
#include <boost/lexical_cast.hpp>
#include "hgvs.hpp"
#include "../core/exceptions.hpp"
#include "../commonTools/parseTools.hpp"
#include "proteinTools.hpp"

// ==================================================================================

enum HgvsChangeType {
	  hctUnknown  = 0
	, hctNoChange = 1
	, hctSNV
	, hctDeletion
	, hctDuplication
	, hctInsertion
	, hctIndel
	, hctInversion
	, hctSeqRepeatitions
};

enum HgvsChromosome {
	  hcUnknown = 0
	, hcFirst   = 1
	, hcSecond  = 2
};

enum HgvsPositionType {
	  hptStandard    = 0
	, hptBeforeStart = 1   // for cDNA, 5' UTR
	, hptAfterEnd    = 2   // for cDNA, 3' UTR
};

enum HgvsExpressionType {
	  hetGenomic   = 0
	, hetCodingDNA = 1
	, hetProtein   = 2
	, hetMitochondrial = 3
	, hetNonCodingRNA  = 4
	, hetRNA = 5
};

struct HgvsVariation {
	HgvsChangeType type       = hctUnknown;
	HgvsPositionType positionFirstType = hptStandard;
	HgvsPositionType positionLastType  = hptStandard;
	unsigned positionFirst    = 0;
	unsigned positionLast     = 0;
	int positionFirstIntron   = 0;
	int positionLastIntron    = 0;
	std::string refSequence   = "";
	std::string newSequence   = "";
	bool frameshift           = false;
	unsigned frameshiftTerOffset = 0;
	unsigned numberOfRepeats  = 0;
	HgvsChromosome chromosome = hcUnknown;
	HgvsVariation(HgvsChangeType p = hctUnknown) : type(p) {}
	bool hasSinglePosition() const { return (positionFirst == positionLast && positionFirstIntron == positionLastIntron && positionFirstType == positionLastType); }
};

struct HgvsExpression {
	std::string refName;
	HgvsExpressionType expType;
	std::vector<HgvsVariation> variations;
};

// =========================================================


bool tryParseHgvsProteinChar(char const *& ptr, AminoAcid & aa)
{
	aa = aaUnknown;
	try {
		if (ptr[0] != '\0' && ptr[1] != '\0' && ptr[2] != '\0') {
			aa = threeLettersCode2aminoAcid(std::string(ptr, ptr+3));
		}
	} catch (...) { }
	if (aa != aaUnknown) {
		ptr += 3;
		return true;
	}
	try {
		if (ptr[0] != '\0') {
			aa = oneLettersCode2aminoAcid(std::string(ptr, ptr+1));
		}
	} catch (...) { }
	if (aa != aaUnknown) {
		++ptr;
		return true;
	}
	return false;
}

std::string toString(HgvsChangeType ct)
{
	switch (ct) {
	case hctDeletion   : return "Deletion";
	case hctDuplication: return "Duplication";
	case hctIndel      : return "Indel";
	case hctInsertion  : return "Insertion";
	case hctInversion  : return "Inversion";
	case hctNoChange   : return "NoChange";
	case hctSNV        : return "SNV";
	case hctSeqRepeatitions: return "SequenceRepeatVariability";
	case hctUnknown    : return "?";
	}
	return "???";
}

std::string toString(HgvsPositionType pt, unsigned pos, int intron)
{
	std::ostringstream s("");
	if (pt == hptAfterEnd) s << "*"; else if (pt == hptBeforeStart) s << "-";
	s << pos;
	if (intron > 0) {
		s << "+" << intron;
	} else if (intron < 0) {
		s << intron;
	}
	return s.str();
}

std::string toString(HgvsChromosome c)
{
	switch (c) {
	case hcFirst: return "Maternal";
	case hcSecond: return "Paternal";
	case hcUnknown: return "?";
	}
	return "???";
}

std::string toString(HgvsExpressionType et)
{
	switch (et) {
	case hetCodingDNA: return "cDNA";
	case hetGenomic  : return "genomic";
	case hetProtein  : return "protein";
	case hetMitochondrial: return "mitochondrial";
	case hetNonCodingRNA : return "ncRNA";
	case hetRNA          : return "RNA";
	}
	return "???";
}

std::ostream & operator<<(std::ostream & str, HgvsVariation const & hgvs)
{
	str << "{\"changeType\":\"" << toString(hgvs.type) << "\", \"position_1\":\"" << toString(hgvs.positionFirstType,hgvs.positionFirst,hgvs.positionFirstIntron) << "\", ";
	if (! hgvs.hasSinglePosition()) str <<  "\"position_2\":\"" << toString(hgvs.positionLastType,hgvs.positionLast,hgvs.positionLastIntron) << "\", ";
	str << "\"ref\":\"" << hgvs.refSequence << "\", \"alt\":\"" << hgvs.newSequence << "\"";
	if (hgvs.frameshift) {
		str << ", \"frameshift\":\"true\"";
	}
	if (hgvs.frameshiftTerOffset) {
		str << ", \"TerOffset\":" << hgvs.frameshiftTerOffset;
	}
	if (hgvs.numberOfRepeats) {
		str << ", \"SeqRepeats\":" << hgvs.numberOfRepeats;
	}
	if (hgvs.chromosome != hcUnknown) {
		str << ", \"chr\":\"" << toString(hgvs.chromosome) << "\"";
	}
	str << "}";
	return str;
}


std::ostream & operator<<(std::ostream & str, HgvsExpression const & hgvs)
{
	str << "{\n\"refName\": \"" << hgvs.refName << "\",\n\"expType\": \"" << toString(hgvs.expType) << "\",\n\"variations\":\n";
	bool first = true;
	for (HgvsVariation const & var: hgvs.variations) {
		if (first) str << "["; else str << ",";
		str << var << "\n";
	}
	str << "]}";
	return str;
}

void parseChar(char const *& ptr, char const v)
{
	if (*ptr != v) {
		std::string expected = "X";
		expected[0] = v;
		throw ExceptionHgvsParsingError("The character '" + expected + "' was expected" + butTheCharacterWasFound(ptr));
	}
	++ptr;
}

void parseUnsignedInt(char const *& ptr, unsigned & number)
{
	if (*ptr < '0' || *ptr > '9') {
		if (*ptr == '?' || *ptr == '(') {
			throw ExceptionHgvsParsingError("Integer was expected" + butTheCharacterWasFound(ptr) + ". HGVS expression must be unambiguous, unknown parameters are not allowed");
		}
		throw ExceptionHgvsParsingError("Integer was expected" + butTheCharacterWasFound(ptr));
	}
	number = 0;
	do {
		number *= 10;
		number += (*ptr - '0');
		++ptr;
	} while (*ptr >= '0' && *ptr <= '9');
}

void parseBp(char const *& ptr, std::string & out)
{
	switch (*ptr) {
		case 'A': out = "A"; ++ptr; return;
		case 'C': out = "C"; ++ptr; return;
		case 'G': out = "G"; ++ptr; return;
		case 'T': out = "T"; ++ptr; return;
		default : throw ExceptionHgvsParsingError("Single nucleotide was expected (A,C,G or T)" + butTheCharacterWasFound(ptr));
	}
}



void parseProtein(char const *& ptr, AminoAcid & aa)
{
	if (*ptr >= 'A' && *ptr <= 'Z' && ptr[1] >= 'a' && ptr[1] <= 'z' && ptr[2] >= 'a' && ptr[2] <= 'z') {
		aa = threeLettersCode2aminoAcid(std::string(ptr, ptr+3));
		ptr += 3;
	} else {
		aa = oneLettersCode2aminoAcid(std::string(ptr, ptr+1));
		++ptr;
	}
}

void parseProteinSequence(char const *& ptr, std::string & out)
{
	out = "";
	AminoAcid aa;
	while (tryParseHgvsProteinChar(ptr,aa)) {
		out += aminoAcid2oneLettersCode(aa);
	}
}

bool tryParseFrameshift(char const *& ptr, HgvsVariation & out)
{
	if (! tryParse(ptr,"fs")) return false;
	out.frameshift = true;
	if (tryParse(ptr,"*") || tryParse(ptr,"Ter")) {
		parseUnsignedInt(ptr, out.frameshiftTerOffset);
	}
	return true;
}

template <HgvsExpressionType tExpType>
void parseHgvsPosition(char const *& ptr, HgvsPositionType & outPosType, unsigned & outPos, int & outPosIntron)
{
	outPosType = hptStandard;
	outPosIntron = 0;
	parseUnsignedInt(ptr,outPos);
}

template <>
void parseHgvsPosition<hetCodingDNA>(char const *& ptr, HgvsPositionType & outPosType, unsigned & outPos, int & outPosIntron)
{
	if (*ptr == '-') {
		outPosType = hptBeforeStart;
		++ptr;
	} else if (*ptr == '*') {
		outPosType = hptAfterEnd;
		++ptr;
	} else {
		outPosType = hptStandard;
	}
	parseUnsignedInt(ptr,outPos);
	if (*ptr == '-' || *ptr == '+') {
		bool const minus = (*ptr == '-');
		++ptr;
		unsigned v;
		parseUnsignedInt(ptr,v);
		outPosIntron = v;
		if (minus) outPosIntron *= -1;
	} else {
		outPosIntron = 0;
	}
}

template <>
void parseHgvsPosition<hetNonCodingRNA>(char const *& ptr, HgvsPositionType & outPosType, unsigned & outPos, int & outPosIntron)
{
	parseHgvsPosition<hetCodingDNA>(ptr,outPosType,outPos,outPosIntron);
}

template <>
void parseHgvsPosition<hetRNA>(char const *& ptr, HgvsPositionType & outPosType, unsigned & outPos, int & outPosIntron)
{
	parseHgvsPosition<hetCodingDNA>(ptr,outPosType,outPos,outPosIntron);
}

AminoAcid parseHgvsPositionProtein(char const *& ptr, HgvsPositionType & outPosType, unsigned & outPos, int & outPosIntron)
{
	outPosType = hptStandard;
	outPosIntron = 0;
	AminoAcid aa;
	parseProtein(ptr, aa);
	parseUnsignedInt(ptr,outPos);
	return aa;
}

template <HgvsExpressionType tExpType>
void parseHgvsRange(char const *& ptr, HgvsVariation & out)
{
	parseHgvsPosition<tExpType>(ptr, out.positionFirstType, out.positionFirst, out.positionFirstIntron);
	if (*ptr == '_') {
		++ptr;
		parseHgvsPosition<tExpType>(ptr, out.positionLastType, out.positionLast, out.positionLastIntron);
	} else {
		out.positionLastType   = out.positionFirstType;
		out.positionLast       = out.positionFirst;
		out.positionLastIntron = out.positionFirstIntron;
	}
}

template <>
void parseHgvsRange<hetProtein>(char const *& ptr, HgvsVariation & out)
{
	AminoAcid aa = parseHgvsPositionProtein(ptr, out.positionFirstType, out.positionFirst, out.positionFirstIntron);
	if (*ptr == '_') {
		++ptr;
		parseHgvsPositionProtein(ptr, out.positionLastType, out.positionLast, out.positionLastIntron);
	} else {
		out.positionLastType   = out.positionFirstType;
		out.positionLast       = out.positionFirst;
		out.positionLastIntron = out.positionFirstIntron;
		out.refSequence = aminoAcid2oneLettersCode(aa);
	}
}


template <HgvsExpressionType tExpType>
void parseHgvsChange(char const *& ptr, HgvsVariation & out)
{
	if (tryParse(ptr,"del")) {
		parseNucleotideSequence(ptr,out.refSequence);
		if (out.refSequence == "" && *ptr >= '0' && *ptr <= '9') {
			unsigned length;
			parseUnsignedInt(ptr, length);
			//TODO - validate length
		}
		if (tryParse(ptr,"ins")) {
			parseNucleotideSequence(ptr,out.newSequence);
			out.type = hctIndel;
		} else {
			out.type = hctDeletion;
		}
	} else if (tryParse(ptr,"dup")) {
		out.type = hctDuplication;
		parseNucleotideSequence(ptr,out.newSequence);
		unsigned length = 0;
		if (out.newSequence == "" && *ptr >= '0' && *ptr <= '9') parseUnsignedInt(ptr, length);
		// TODO - validate length
	} else if (tryParse(ptr,"ins")) {
		parseNucleotideSequence(ptr,out.newSequence);
		out.type = hctInsertion;
	} else if (tryParse(ptr,"inv")) {
		parseNucleotideSequence(ptr,out.refSequence);
		out.type = hctInversion;
//	} else if (tryParse(ptr,"con")) {
//		throw ExceptionHgvsParsingError("Mutations defined as 'con' are not supported yet");  // TODO
	} else {
		parseNucleotideSequence(ptr,out.refSequence);
		if ( out.refSequence.size() == 1 && tryParse(ptr, ">") ) {
			parseBp(ptr,out.newSequence);
			out.type = hctSNV;
		} else if ( out.refSequence.size() == 1 && tryParse(ptr, "=") ) {
			out.newSequence = out.refSequence;
			out.type = hctNoChange;
		} else if (*ptr == '[') {
			parseChar(ptr, '[');
			parseUnsignedInt(ptr, out.numberOfRepeats);
			parseChar(ptr, ']');
			out.type = hctSeqRepeatitions;
		} else {
			throw ExceptionHgvsParsingError("Cannot parse definition of mutation");
		}
	}
}

template <>
void parseHgvsChange<hetProtein>(char const *& ptr, HgvsVariation & out)
{
	if (tryParse(ptr,"del")) {
		parseProteinSequence(ptr, out.refSequence);
		if (tryParse(ptr,"ins")) {
			parseProteinSequence(ptr, out.newSequence);
			out.type = hctIndel;
		} else {
			out.type = hctDeletion;
		}
		tryParseFrameshift(ptr,out);
	} else if (tryParse(ptr,"dup")) {
		out.type = hctDuplication;
		tryParseFrameshift(ptr,out);
	} else if (tryParse(ptr,"ins")) {
		parseProteinSequence(ptr,out.newSequence);
		out.type = hctInsertion;
		tryParseFrameshift(ptr,out);
	} else if (tryParseFrameshift(ptr,out)) {
		out.type = hctNoChange;
	} else if (tryParse(ptr,"=")) {
		out.type = hctNoChange;
		tryParseFrameshift(ptr,out);
	} else {
		AminoAcid aa;
		parseProtein(ptr,aa);
		out.newSequence = aminoAcid2oneLettersCode(aa);
		out.type = hctSNV;
		if (! tryParseFrameshift(ptr,out)) tryParse(ptr,"=");
	}
}

template <HgvsExpressionType tExpType>
HgvsVariation parseHgvsVariation(char const *& ptr)
{
	HgvsVariation r;
	parseHgvsRange<tExpType>(ptr, r);
	parseHgvsChange<tExpType>(ptr, r);
	return r;
}

template <HgvsExpressionType tExpType>
void parseHgvsHaploVariation(char const *& ptr, std::vector<HgvsVariation> & output, HgvsChromosome const chromosome = hcUnknown)
{
	output.push_back(parseHgvsVariation<tExpType>(ptr));
	output.back().chromosome = chromosome;
	while (*ptr == ';') {
		++ptr;
		output.push_back(parseHgvsVariation<tExpType>(ptr));
		output.back().chromosome = chromosome;
	}
}

template <HgvsExpressionType tExpType>
void parseHgvsDiploVariation(char const *& ptr, std::vector<HgvsVariation> & output)
{
	if (*ptr == '[') {
		parseChar(ptr,'[');
		parseHgvsHaploVariation<tExpType>(ptr,output,hcFirst);
		parseChar(ptr,']');
		if (*ptr == ';') {
			++ptr;
			parseChar(ptr,'[');
			parseHgvsHaploVariation<tExpType>(ptr,output,hcSecond);
			parseChar(ptr,']');
		}
	} else {
		output.push_back( parseHgvsVariation<tExpType>(ptr) );
	}
}

void parseHgvs(char const *& ptr, HgvsExpression & expHgvs)
{
	char const * const begin = ptr;
	try {
		for (; *ptr != ':';  ++ptr ) {
			if (*ptr == '\0') throw ExceptionHgvsParsingError("Unexpected end of expression. The character ':' was not found");
		}
		expHgvs.refName = std::string(begin, ptr);
		++ptr;
		switch (*ptr) {
			case 'c': expHgvs.expType = hetCodingDNA    ; break;
			case 'g': expHgvs.expType = hetGenomic      ; break;
			case 'p': expHgvs.expType = hetProtein      ; break;
			case 'm': expHgvs.expType = hetMitochondrial; break;
			case 'n': expHgvs.expType = hetNonCodingRNA ; break;
			case 'r': expHgvs.expType = hetRNA          ; break;
			default : throw ExceptionHgvsParsingError("One of the characters c,g,p,m,n,r was expected," + butTheCharacterWasFound(ptr));
		}
		++ptr;
		parseChar(ptr, '.');
		switch (expHgvs.expType) {
			case hetCodingDNA    : parseHgvsDiploVariation<hetCodingDNA    >(ptr,expHgvs.variations); break;
			case hetGenomic      : parseHgvsDiploVariation<hetGenomic      >(ptr,expHgvs.variations); break;
			case hetProtein      : parseHgvsDiploVariation<hetProtein      >(ptr,expHgvs.variations); break;
			case hetMitochondrial: parseHgvsDiploVariation<hetMitochondrial>(ptr,expHgvs.variations); break;
			case hetNonCodingRNA : parseHgvsDiploVariation<hetNonCodingRNA >(ptr,expHgvs.variations); break;
			case hetRNA          : parseHgvsDiploVariation<hetRNA          >(ptr,expHgvs.variations); break;
			default: throw std::logic_error("Unknown HGVS type in switch statement");
		}
		if (*ptr != '\0') {
			throw ExceptionHgvsParsingError("The HGVS expression was parsed, but the following suffix was not processed: " + std::string(ptr));
		}
	} catch (ExceptionHgvsParsingError & e) {
		e.setContext(begin, ptr-begin);
		throw;
	}
}


void decodeHgvs ( ReferencesDatabase const* seqDb
				, std::string const &       phgvs
				, HgvsVariant & pDecodedHgvs // out
				)
{
	HgvsExpression hgvs;
	char * buf = new char[phgvs.size()+1];
	try {
		strcpy(buf, phgvs.c_str());
		char const * buf2 = buf;
		parseHgvs(buf2, hgvs);
		delete [] buf;
	} catch (...) {
		delete [] buf;
		throw;
	}
	try {
		if (hgvs.variations.empty()) throw std::logic_error("No modifications were found in HGVS expression.");
		pDecodedHgvs = HgvsVariant( (hgvs.expType != hetProtein) );
		if ( pDecodedHgvs.isGenomic() ) {
			HgvsGenomicVariant & decodedHgvs = pDecodedHgvs.genomic;
			decodedHgvs.refId = seqDb->getReferenceId(hgvs.refName);
			for (HgvsVariation & var : hgvs.variations) {
				if (var.chromosome != hgvs.variations.front().chromosome) throw ExceptionHgvsParsingError("Diploid alleles are not supported.");
				HgvsGenomicSequenceModification m;
				switch (hgvs.expType) {
					case hetCodingDNA:
					{
						if ( ! seqDb->isSplicedRefSeq(decodedHgvs.refId) ) throw ExceptionHgvsParsingError("Definition of coding DNA variation for non-spliced reference.");
						RegionCoordinates const & cds = seqDb->getCDS(decodedHgvs.refId);
						unsigned left = 0;
						switch (var.positionFirstType) {
						case hptAfterEnd   :
							left = cds.right() + var.positionFirst - 1;
							break;
						case hptStandard   :
							left = cds.left()  + var.positionFirst - 1;
							break;
						case hptBeforeStart:
							if (cds.left() < var.positionFirst) throw ExceptionIncorrectHgvsPosition("The position is located before the beginning of the transcript");
							left = cds.left() - var.positionFirst;
							break;
						}
						unsigned right = 0;
						switch (var.positionLastType) {
						case hptAfterEnd   :
							right = cds.right() + var.positionLast;
							break;
						case hptStandard   :
							right = cds.left()  + var.positionLast;
							break;
						case hptBeforeStart:
							if (cds.left() + 1 < var.positionLast) throw ExceptionIncorrectHgvsPosition("The position is located before the beginning of the transcript");
							right = cds.left() + 1 - var.positionLast;
							break;
						}
						SplicedCoordinate fullLeft;
						if (var.positionFirstIntron < 0) {
							fullLeft.set( left, '-', -var.positionFirstIntron );
						} else if (var.positionFirstIntron > 0) {
							fullLeft.set( left+1, '+', var.positionFirstIntron-1 );
						} else {
							fullLeft.set( left, '-', 0 );
						}
						SplicedCoordinate fullRight;
						if (var.positionLastIntron < 0) {
							fullRight.set( right-1, '-', -var.positionLastIntron-1 );
						} else if (var.positionLastIntron > 0) {
							fullRight.set( right, '+', var.positionLastIntron );
						} else {
							fullRight.set( right, '+', 0 );
						}
						m.region = seqDb->convertToUnsplicedRegion( decodedHgvs.refId, SplicedRegionCoordinates(fullLeft,fullRight) );
						break;
					}
					case hetNonCodingRNA:
					case hetRNA:
					{
						if ( ! seqDb->isSplicedRefSeq(decodedHgvs.refId) ) throw ExceptionHgvsParsingError("Definition of coding DNA variation for non-spliced reference.");
						if ( var.positionFirstType != hptStandard || var.positionLastType != hptStandard ) throw ExceptionIncorrectHgvsPosition("The position is located outside of the sequence");
						if ( var.positionLast > seqDb->getMetadata(decodedHgvs.refId).length ) throw ExceptionIncorrectHgvsPosition("The position is located outside of the sequence");
						unsigned const left = var.positionFirst - 1;
						unsigned const right = var.positionLast;
						SplicedCoordinate fullLeft;
						if (var.positionFirstIntron < 0) {
							fullLeft.set( left, '-', -var.positionFirstIntron );
						} else if (var.positionFirstIntron > 0) {
							fullLeft.set( left+1, '+', var.positionFirstIntron-1 );
						} else {
							fullLeft.set( left, '-', 0 );
						}
						SplicedCoordinate fullRight;
						if (var.positionLastIntron < 0) {
							fullRight.set( right-1, '-', -var.positionLastIntron-1 );
						} else if (var.positionLastIntron > 0) {
							fullRight.set( right, '+', var.positionLastIntron );
						} else {
							fullRight.set( right, '+', 0 );
						}
						m.region = seqDb->convertToUnsplicedRegion( decodedHgvs.refId, SplicedRegionCoordinates(fullLeft,fullRight) );
						break;
					}
					case hetGenomic:
					case hetMitochondrial:
						if ( seqDb->isProteinReference(decodedHgvs.refId) ) throw ExceptionHgvsParsingError("Definition of genomic variation for protein reference.");
						if ( seqDb->isSplicedRefSeq(decodedHgvs.refId) ) throw ExceptionHgvsParsingError("Definition of genomic variation for spliced reference.");
						if ( var.positionFirstType != hptStandard || var.positionLastType != hptStandard ) throw ExceptionIncorrectHgvsPosition("The position is located outside of the sequence");
						if ( var.positionLast > seqDb->getMetadata(decodedHgvs.refId).length ) throw ExceptionIncorrectHgvsPosition("The position is located outside of the sequence");
						if ( var.positionFirstIntron != 0 || var.positionLastIntron != 0 ) throw ExceptionIncorrectHgvsPosition("An intronic offset was given for unspliced sequence");
						m.region.set( var.positionFirst-1, var.positionLast );
						break;
					case hetProtein:
						ASSERT(false);
				}
				// -----------------------
				m.originalSequence = var.refSequence;
				switch (var.type) {
					case hctInsertion:
					{
						// range correction for insertion (in HGVS 2 bp are given, between whom the insertion occurs)
						if (m.region.length() != 2) throw ExceptionIncorrectHgvsPosition("The position of insertion is not correct");
						m.region.incLeftPosition(1);
						m.region.decRightPosition(1);
						// set inserted sequence
						HgvsGenomicSequence s;
						s.sequence = var.newSequence;
						m.newSequence.push_back(s);
						break;
					}
					case hctDuplication:
					{
						// set inserted sequence
						HgvsGenomicSequence s;
						s.refId = decodedHgvs.refId;
						s.region = m.region;
						m.newSequence.push_back(s);
						m.newSequence.push_back(s);
						break;
					}
					case hctInversion:
					{
						// set inserted sequence
						HgvsGenomicSequence s;
						s.refId = decodedHgvs.refId;
						s.region = m.region;
						s.inversion = true;
						m.newSequence.push_back(s);
						break;
					}
					case hctSeqRepeatitions:
					{
						m.newRepetitionsCount = var.numberOfRepeats;
						break;
					}
					default:
					{
						// set inserted sequence
						HgvsGenomicSequence s;
						s.sequence = var.newSequence;
						m.newSequence.push_back(s);
						break;
					}
				}
				decodedHgvs.firstChromosome.push_back(m);
			}
		} else { // is protein
			HgvsProteinVariant & decodedHgvs = pDecodedHgvs.protein;
			decodedHgvs.refId = seqDb->getReferenceId(hgvs.refName);
			for (HgvsVariation & var : hgvs.variations) {
				if (var.chromosome != hgvs.variations.front().chromosome) throw ExceptionHgvsParsingError("Diploid alleles are not supported.");
				HgvsProteinSequenceModification m;
				if ( ! seqDb->isProteinReference(decodedHgvs.refId) ) throw ExceptionHgvsParsingError("Definition of protein variation for non-protein reference.");
				if ( var.positionFirstType != hptStandard || var.positionLastType != hptStandard ) throw ExceptionIncorrectHgvsPosition("Position outside the protein sequence");
				if ( var.positionLast > seqDb->getMetadata(decodedHgvs.refId).length ) throw ExceptionIncorrectHgvsPosition("Position outside the protein sequence");
				if ( var.positionFirstIntron != 0 || var.positionLastIntron != 0 ) throw ExceptionIncorrectHgvsPosition("Intronic offsets for protein sequences are not allowed");
				if ( var.frameshift ) throw ExceptionHgvsParsingError("Protein alleles with frameshift are not supported"); // TODO
				m.region.set( var.positionFirst-1, var.positionLast );
				if (var.type == hctNoChange) {
					if (var.refSequence.empty()) {
						var.refSequence = var.newSequence = seqDb->getSequence(decodedHgvs.refId,m.region);
					} else {
						var.newSequence = var.refSequence;
					}
				}
				switch (var.type) {
					case hctInsertion:
					{
						// range correction for insertion (in HGVS 2 bp are given, between whom the insertion occurs)
						if (m.region.length() != 2) throw ExceptionIncorrectHgvsPosition("The position of insertion is not correct");
						m.region.incLeftPosition(1);
						m.region.decRightPosition(1);
						// set inserted sequence
						HgvsProteinSequence s;
						s.sequence = var.newSequence;
						m.newSequence.push_back(s);
						break;
					}
					case hctDuplication:
					{
						// set inserted sequence
						HgvsProteinSequence s;
						s.region = m.region;
						m.newSequence.push_back(s);
						m.newSequence.push_back(s);
						m.originalSequence = var.refSequence;
						break;
					}
					case hctSeqRepeatitions:
					{
						m.newRepetitionsCount = var.numberOfRepeats;
						break;
					}
					default:
					{
						// set inserted sequence
						HgvsProteinSequence s;
						s.sequence = var.newSequence;
						m.newSequence.push_back(s);
						m.originalSequence = var.refSequence;
						break;
					}
				}
				decodedHgvs.firstChromosome.push_back(m);
			}
		}
	} catch (ExceptionHgvsParsingError & e) {
		e.setContext(phgvs,phgvs.size());
		throw;
	} catch (ExceptionIncorrectHgvsPosition & e) {
		e.setContext(phgvs);
		throw;
	}
}

static std::string haplotypeAsString(std::vector<std::string> const & haplotype)
{
	std::string s = haplotype.at(0);
	if (haplotype.size() > 1) {
		for (unsigned i = 1; i < haplotype.size(); ++i) s += ";" + haplotype[i];
		s = "[" + s + "]";
	}
	return s;
}

static std::string toHgvsPosition( ReferencesDatabase const* refDb, ReferenceId refId, unsigned unsplicedHgvsPosition )
{
	std::string s;
	if (refDb->isSplicedRefSeq(refId)) {
		SplicedRegionCoordinates region = refDb->convertToSplicedRegion(refId, RegionCoordinates(unsplicedHgvsPosition-1,unsplicedHgvsPosition));
		unsigned splicedPos = region.right().position();
		if (region.right().hasNegativeOffset()) ++splicedPos; // for HFVS position we have to move to bp after intron
		RegionCoordinates cds = refDb->getCDS(refId);
		if (cds.left() > 0 && cds.right() > 0) { // TODO - it should check for NULL-s
			if (splicedPos <= cds.left()) {
				s = "-" + boost::lexical_cast<std::string>(cds.left() - splicedPos + 1);
			} else if (splicedPos <= cds.right()) {
				s = boost::lexical_cast<std::string>(splicedPos - cds.left());
			} else {
				s = "*" + boost::lexical_cast<std::string>(splicedPos - cds.right());
			}
		} else {
			s = boost::lexical_cast<std::string>(splicedPos);
		}
		if (region.isIntronic()) {
			if (region.right().hasNegativeOffset()) {
				s += "-" + boost::lexical_cast<std::string>( region.right().offsetSize()+1 );
			} else {
				s += "+" + boost::lexical_cast<std::string>( region.right().offsetSize() );
			}
		}
	} else {
		s = boost::lexical_cast<std::string>(unsplicedHgvsPosition);
	}
	return s;
}

std::string toHgvsProteinPosition( char aa, unsigned position )
{
	std::string s = convertOneLetterAminoAcidSequenceToThreeLetters(std::string(1,aa));
	s += boost::lexical_cast<std::string>(position);
	return s;
}

std::string toHgvsModifications( ReferencesDatabase const* refDb, NormalizedGenomicVariant const & var )
{
	std::vector<std::string> haplotype;
	for (NormalizedSequenceModification const & m: var.modifications) {
		std::string result = "";
		if (m.category == variantCategory::duplication && m.region.length() >= m.lengthChange) {
			result += toHgvsPosition( refDb, var.refId, m.region.right()-m.lengthChange+1 );
			if (m.lengthChange > 1) result += "_" + toHgvsPosition( refDb, var.refId, m.region.right() );
			result += "dup";
			// TODO - repetitions & inversions
		} else {
			PlainSequenceModification p = m.rightAligned();
			if (p.originalSequence.size() == 1 && p.newSequence.size() == 1) {
				// SNV
				result += toHgvsPosition( refDb, var.refId, p.region.right() );
				result += p.originalSequence;
				if (p.originalSequence == p.newSequence) {
					result += "=";
				} else {
					result += ">" + p.newSequence;
				}
			} else if (p.originalSequence.size() == 0) {
				// insertion
				result += toHgvsPosition( refDb, var.refId, p.region.left() );
				result += "_" + toHgvsPosition( refDb, var.refId, p.region.right()+1 );
				result += "ins";
				result += p.newSequence;
			} else {
				// deletion or indel
				result += toHgvsPosition( refDb, var.refId, p.region.left()+1 );
				if (p.region.length() > 1) result += "_" + toHgvsPosition( refDb, var.refId, p.region.right() );
				result += "del";
				if (p.newSequence.size() > 0) {
					// indel
					result += "ins" + p.newSequence;
				}
			}
		}
		haplotype.push_back(result);
	}
	std::string prefix;
	if (refDb->isSplicedRefSeq(var.refId)) {
		RegionCoordinates cds = refDb->getCDS(var.refId);
		if (cds.left() > 0 && cds.right() > 0) { // TODO - it should check for NULL-s
			prefix = "c."; // coding DNA (spliced)
		} else {
			prefix = "n."; // non-coding DNA (spliced)
		}
	} else {
		bool isMitochondrial = false;
		try { isMitochondrial = (var.refId == refDb->getReferenceId("NC_012920.1")); } catch (...) { }
		if (! isMitochondrial) try { isMitochondrial = (var.refId == refDb->getReferenceId("NC_001807.4")); } catch (...) { }
		if ( isMitochondrial ) {
			prefix = "m."; // mitochondrial
		} else {
			prefix = "g."; // genomic
		}
	}
	return ( prefix + haplotypeAsString(haplotype) );
}

std::string toHgvsModification( ReferencesDatabase const* refDb, ReferenceId const refId, PlainSequenceModification const & p )
{
	std::string result = "";
	if (p.originalSequence.size() == 1 && p.newSequence.size() == 1) {
		// SNV
		result += toHgvsProteinPosition( p.originalSequence[0], p.region.right() );
		if (p.originalSequence == p.newSequence) {
			result += "=";
		} else {
			result += convertOneLetterAminoAcidSequenceToThreeLetters(p.newSequence);
		}
	} else if (p.originalSequence.size() == 0) {
		// insertion
		if (p.region.left() > 0) {
			std::string const s = refDb->getSequence( refId, RegionCoordinates(p.region.left()-1,p.region.right()+1) );
			result += toHgvsProteinPosition( s.front(), p.region.left() );
			result += "_" + toHgvsProteinPosition( s.back(), p.region.right()+1 );
		} else {
			result += "Met1";
		}
		result += "ins";
		result += convertOneLetterAminoAcidSequenceToThreeLetters(p.newSequence);
	} else {
		// deletion or indel
		result += toHgvsProteinPosition( p.originalSequence.front(), p.region.left()+1 );
		if (p.region.length() > 1) result += "_" + toHgvsProteinPosition( p.originalSequence.back(), p.region.right() );
		result += "del";
		if (p.newSequence.size() > 0) {
			// indel
			result += "ins" + convertOneLetterAminoAcidSequenceToThreeLetters(p.newSequence);
		}
	}
	return result;
}

std::string toHgvsModifications( ReferencesDatabase const* refDb, NormalizedProteinVariant const & var )
{
	// protein
	std::vector<std::string> haplotype;
	for (NormalizedSequenceModification const & m: var.modifications) {
		PlainSequenceModification p = m.rightAligned();
		std::string result = "";
		if (m.category == variantCategory::duplication && m.region.length() >= m.lengthChange) {
			result += toHgvsProteinPosition( p.newSequence.front(), m.region.right()-m.lengthChange+1 );
			if (m.lengthChange > 1) result += "_" + toHgvsProteinPosition( p.newSequence.back(), m.region.right() );
			result += "dup";
			// TODO - repetitions & inversions
		} else {
			if ( p.newSequence == "" && (p.region.right() + 1 == refDb->getMetadata(var.refId).length) ) {  // TODO - +1 because of termination codon
				// convert deletion of the rest of protein to SNP with stop codon
				p.originalSequence.resize(1);
				p.newSequence.resize(1, aminoAcid2oneLettersCode(aaStopCodon)[0]);
				p.region.setRight(p.region.left()+1);
			}
			result = toHgvsModification(refDb, var.refId, p);
		}
		haplotype.push_back(result);
	}
	return ( "p." + haplotypeAsString(haplotype) );
}


#ifdef STANDALONE
#include <iostream>
#include <cstring>
#include <cassert>

void testHgvs   ( ReferenceSequencesDatabase const* seqDb
				, std::string const &               phgvs
				, std::string const &               psequenceName
				, unsigned const                    position
				, char const                        intronOffsetSign
				, unsigned                          intronOffset
				, std::string const &               prefAllele
				, std::string const &               pnewAllele
				)
{
	std::cout << phgvs << std::endl;
	std::string              sequenceName;
	std::vector<LocalModification> modifications;
	decodeHgvs(seqDb, phgvs, sequenceName, modifications);
	if (modifications.size() != 1) throw std::logic_error("Incorrect number of modifications found in HGVS expression");
	GeneralRegionCoordinates region    = modifications[0].region;
	std::string              refAllele = modifications[0].refAllele;
	std::string              newAllele = modifications[0].newAllele;
	assert( sequenceName == psequenceName );
	assert( refAllele == prefAllele );
	assert( newAllele == pnewAllele );
	assert( position == region.leftPosition );
	assert( (intronOffsetSign == '-' || intronOffsetSign == '+') == region.leftIsIntronic() );
	assert( region.rightIsIntronic() == region.leftIsIntronic() );
	if (region.isIntronic()) {
		assert( (intronOffsetSign == '-') == region.leftExternalOffsetNegative );
		assert( region.rightExternalOffsetNegative == region.leftExternalOffsetNegative );
		assert( region.leftExternalOffset == intronOffset );
		if ( intronOffsetSign == '-' ) {
			assert( region.leftExternalOffset == region.rightExternalOffset + prefAllele.size() );
		} else {
			assert( region.leftExternalOffset == region.rightExternalOffset - prefAllele.size() );
		}
		assert( position == region.rightPosition );
	} else {
		assert( position+prefAllele.size() == region.rightPosition );
	}
}

void testHgvs   ( ReferenceSequencesDatabase const* seqDb
				, std::string const &               phgvs
				, std::string const &               psequenceName
				, unsigned const                    position
				, std::string const &               prefAllele
				, std::string const &               pnewAllele
				)
{
	testHgvs(seqDb, phgvs, psequenceName, position, ' ', 0, prefAllele, pnewAllele);
}

int main(int argc, char ** argv)
{
	std::cout << "Run unit tests ... " << std::endl;

	ReferenceSequencesDatabase db;

	testHgvs(&db, "NM_000546.5:c.329G>A", "NM_000546.5"   , 530, "G", "A");
	testHgvs(&db, "NM_001126112.2:c.329G>A", "NM_001126112.2", 527, "G", "A");
	testHgvs(&db, "NM_001126113.2:c.329G>A", "NM_001126113.2", 530, "G", "A");
	testHgvs(&db, "NM_001126114.2:c.329G>A", "NM_001126114.2", 530, "G", "A");
	testHgvs(&db, "NM_001126118.1:c.212G>A", "NM_001126118.1", 647, "G", "A");
	testHgvs(&db, "NC_000017.11:g.7676040C>T", "NC_000017.11", 7676039, "C", "T");

	testHgvs(&db, "NM_007294.3:c.134+1G>T", "NM_007294.3", 366, '+', 0, "G", "T");
	testHgvs(&db, "NM_007297.3:c.-8+8292G>T", "NM_007297.3", 274, '+', 8291, "G", "T");
	testHgvs(&db, "NC_000017.11:g.43115725C>A", "NC_000017.11", 43115724, "C", "A");

	testHgvs(&db, "NM_000546.5:c.1100+13A>G", "NM_000546.5", 1302, '+', 12, "A", "G" );

	testHgvs(&db, "NM_000314.6:c.42delG", "NM_000314.6", 1072, "G", "" );
	testHgvs(&db, "NC_000010.11:g.87864510delG", "NC_000010.11", 87864509, "G", "" );

	testHgvs(&db, "NM_000038.5:c.730_731delAG", "NM_000038.5", 814, "AG", "");
	testHgvs(&db, "NM_001127510.2:c.730_731delAG", "NM_001127510.2", 922, "AG", "");
	testHgvs(&db, "NM_001127511.2:c.676_677delAG", "NM_001127511.2", 888, "AG", "");
	testHgvs(&db, "NC_000005.10:g.112801279_112801280delAG", "NC_000005.10", 112801278, "AG", "");

	testHgvs(&db, "NM_000314.6:c.78C>T", "NM_000314.6", 1108, "C", "T" );

	testHgvs(&db, "NC_000017.11:g.15260293_15260294insA", "NC_000017.11", 15260293, "", "A");

	testHgvs(&db, "NM_001281455.1:c.78+353_78+354insT", "NM_001281455.1", 277, '+', 353, "", "T");

	testHgvs(&db, "NM_000059.3:c.516_516+1insC", "NM_000059.3", 743, '+', 0, "", "C");

	testHgvs(&db, "NC_000013.11:g.32326278dupT", "NC_000013.11", 32326277, "", "T");

	// it should fail: NM_000518.4:c.-80del

	std::cout << "OK" << std::endl;

	//code allowing to parse many HGVS from stdin
	if (argc < 2) {
		std::cout << "Parameters: one or more HGVS expressions" << std::endl;
		std::cout << "Special parameters:\n- read data from standard input" << std::endl;
		return 1;
	}

	std::vector<std::string> data;

	for ( int i = 1;  i < argc;  ++i ) {
		if (argv[i][0] == '-') {
			// special parameter
			std::string p = argv[i];
			if (p == "-") {
				while (std::cin.good()) {
					std::string v;
					std::cin >> v;
					data.push_back(v);
				}
			} else {
				std::cerr << "Unknown parameter: " << p << std::endl;
			}
		} else {
			// hgvs expression
			data.push_back(argv[i]);
		}
	}

	std::cout << "[";
	for ( unsigned i = 0;  i < data.size();  ++i ) {
		char buf[256] = {0};
		char const * ptr = buf;
		HgvsExpression hgvs;
		try {
			if (data[i].size() > 255) throw ExceptionHgvsParsingError("Expression is too large: " + data[i]);
			strcpy(buf, data[i].c_str());
			if (i > 0) std::cout << ",";
			parseHgvs(ptr, hgvs);
			std::cout << hgvs << "\n";
		} catch (std::exception const & e) {
			std::cerr << "Error occurred when the following expression was being parsed: " << buf << std::endl;
			std::cerr << "Position: " << (ptr - buf) << std::endl;
			std::cerr << "Message: " << e.what() << std::endl;
			// output
			std::cout << "{ \"Error\": \"" << e.what() << "\" }\n";
		}
	}
	std::cout << "]" << std::endl;

	return 0;
}
#endif

