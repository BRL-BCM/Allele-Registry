#ifndef GLOBALS_HPP_
#define GLOBALS_HPP_

#include <string>
#include <limits>
#include <stdexcept>
#include <vector>
#include <set>
#include <boost/lexical_cast.hpp>

#include "../mysql/mysqlConnection.hpp"
#include "../debug.hpp"

std::string const alleleRegistryVersion = "__ALLELE_REGISTRY_VERSION__";

// ============================== configuration
struct Configuration {
	std::string alleleRegistryFQDN = ""; // no 'http://' prefix, no '/' at the end
	std::string referencesDatabase_path = "";
	std::string allelesDatabase_path = "";
	unsigned    allelesDatabase_threads = 1;
	unsigned    allelesDatabase_ioTasks = 1;
	unsigned    allelesDatabase_cache_genomic = 128;
	unsigned    allelesDatabase_cache_protein = 128;
	unsigned    allelesDatabase_cache_sequence = 128;
	unsigned    allelesDatabase_cache_idCa = 128;
	unsigned    allelesDatabase_cache_idClinVarAllele = 128;
	unsigned    allelesDatabase_cache_idClinVarRCV = 128;
	unsigned    allelesDatabase_cache_idClinVarVariant = 128;
	unsigned    allelesDatabase_cache_idDbSnp = 128;
	unsigned    allelesDatabase_cache_idPa = 128;
	std::vector<std::string> genboree_allowedHostnames;
	std::string logFile_path = "";
	MySqlConnectionParameters genboree_db;
	MySqlConnectionParameters externalSources_db;
	bool        noAuthentication = false;
	bool        readOnly = false; // read-only mode - no writes to database are possible, PUT request are rejected
	std::set<std::string> superusers;
};

// ============================== general stuff - operators - operator < must be defined
#define RELATIONAL_OPERATORS(tType) \
inline bool operator> (tType const & a, tType const & b) { return   (b < a); } \
inline bool operator<=(tType const & a, tType const & b) { return ! (b < a); } \
inline bool operator>=(tType const & a, tType const & b) { return ! (a < b); } \
inline bool operator==(tType const & a, tType const & b) { return ! (a < b || b < a); } \
inline bool operator!=(tType const & a, tType const & b) { return   (a < b || b < a); }


inline std::string toString(unsigned v)
{
	return boost::lexical_cast<std::string>(v);
}

template<typename tElement>
std::string toString(std::vector<tElement> const & v)
{
	if (v.empty()) return "[]";
	std::string r = "";
	for (auto e: v) r += "," + toString(e);
	r[0] = '[';
	r.push_back(']');
	return r;
}


struct ReferenceId {
	static const ReferenceId null;
	unsigned value = std::numeric_limits<unsigned>::max();
	ReferenceId() {}
	explicit ReferenceId(unsigned id) : value(id) {}
	inline bool isNull() const { return (value == std::numeric_limits<unsigned>::max());}
	inline bool operator<(ReferenceId b) const { return (value < b.value); }
};
RELATIONAL_OPERATORS(ReferenceId);


struct CanonicalId {
	static CanonicalId const null;
	uint32_t value = std::numeric_limits<uint32_t>::max();
	inline CanonicalId() {}
	inline explicit CanonicalId(uint32_t v) : value(v) {}
	inline explicit operator uint32_t() const { return value; }
	inline bool isNull() const { return (value == null.value); }
	inline bool operator<(CanonicalId b) const { return (value < b.value); }
};
RELATIONAL_OPERATORS(CanonicalId);


unsigned const AlleleIdNull = 0;
unsigned const SequenceIdNull = 0;

void convertToReverseComplementary(std::string & seq);
void convertToUpperCase(std::string & str);

// ============================== enum types
//enum NucleotideChangeType {
//	  nctUnknown = 0
//	, nctDeletion
//	, nctInsertion
//	, nctInversion
//	, nctTranslocation
//	, nctIndel
//	, nctCopyNumberVariation
//	, nctSubstitution
//	, nctNoSequenceAlteration
//};
//std::string toString(NucleotideChangeType v);
//NucleotideChangeType parseNucleotideChangeType(std::string const & v);
//// ---------------------
//enum ReferenceSequenceType {
//	  rstChromosome
//	, rstTranscript
//	, rstGenomicRegion
//	, rstAlternateLocus
//	, rstAminoAcid
//};
//std::string toString(ReferenceSequenceType v);
//ReferenceSequenceType parseReferenceSequenceType(std::string const & v);
//// ---------------------
//enum CanonicalAlleleType {
//	  catNucleotide
//	, catAminoAcid
//};
//std::string toString(CanonicalAlleleType v);
//CanonicalAlleleType parseCanonicalAlleleType(std::string const & v);
// ---------------------
enum ReferenceGenome {
	  rgUnknown = 0
	, rgNCBI36  = 1
	, rgGRCh37  = 2
	, rgGRCh38  = 3
};
std::string toString(ReferenceGenome v);
ReferenceGenome parseReferenceGenome(std::string const & v);
// ---------------------
enum Chromosome : uint8_t
{
	  chrUnknown = 0
	, chr1  = 1
	, chr2  = 2
	, chr3  = 3
	, chr4  = 4
	, chr5  = 5
	, chr6  = 6
	, chr7  = 7
	, chr8  = 8
	, chr9  = 9
	, chr10 = 10
	, chr11 = 11
	, chr12 = 12
	, chr13 = 13
	, chr14 = 14
	, chr15 = 15
	, chr16 = 16
	, chr17 = 17
	, chr18 = 18
	, chr19 = 19
	, chr20 = 20
	, chr21 = 21
	, chr22 = 22
	, chrX  = 23
	, chrY  = 24
	, chrM  = 25
};
std::string toString(Chromosome v);
Chromosome parseChromosome(std::string const & v);
void parse(char const *& ptr, Chromosome &);
//// ---------------------
//enum ReferenceIdentifierType {
//	  ritNCBI = 0
//	, ritLRG  = 1
//	, ritEnsembl = 2
//	, ritGenBank = 3
//};
//std::string toString(ReferenceIdentifierType v);
//ReferenceIdentifierType parseReferenceIdentifierType(std::string const & v);
// ============================== structs / classes

// ============================== base types

// Can store  intronic coordinate in spliced regions.
// Intronic coordinate is defined as standard positions, sign (+ or -) and unsigned offset.
struct SplicedCoordinate {
private:
	unsigned fPosition = 0;
	bool     fNegativeOffset = false;
	unsigned fOffset = 0;
public:
	SplicedCoordinate() {}
//	SplicedCoordinate(unsigned position, bool leftBorder) : fPosition(position), fNegativeOffset(leftBorder) {}
	SplicedCoordinate(unsigned position, char offsetSign, unsigned offsetSize)
	: fPosition(position), fNegativeOffset(offsetSign == '-'), fOffset(offsetSize)
	{
		if (offsetSign != '-' && offsetSign != '+') {
			throw std::logic_error("GeneralCoordinate() - the only acceptable offset signs are '-' and '+'");
		}
	}
	inline unsigned position()   const { return fPosition; }
	// setters
//	inline void set(unsigned position, bool leftBorder)
//	{
//		fPosition = position;
//		fNegativeOffset = leftBorder;
//		fOffset = 0;
//	}
	// offset sign must be '+' or '-', in other case an exception is thrown
	inline void set(unsigned position, char offsetSign, unsigned offsetSize)
	{
		if (offsetSign != '-' && offsetSign != '+') {
			throw std::logic_error("GeneralCoordinate::set() - the only acceptable offset signs are '-' and '+'");
		}
		fPosition = position;
		fNegativeOffset = (offsetSign == '-');
		fOffset = offsetSize;
	}
	// all functions below throws exception if the coordinate is not intronic
	inline bool hasNegativeOffset() const
	{
		return fNegativeOffset;
	}
	inline char offsetSign() const
	{
		if (fNegativeOffset) return '-';
		return '+';
	}
	inline unsigned offsetSize() const
	{
		return fOffset;
	}
	std::string toString() const;
// When positions are the same, the order is as follows: intronics with + sign by increasing offset sizes, intronics with - sign by decreasing offset sizes
	bool operator<(SplicedCoordinate const & b) const
	{
		if ( fPosition != b.fPosition ) return (fPosition < b.fPosition);
		if ( fNegativeOffset && b.fNegativeOffset ) return (fOffset > b.fOffset);
		if ( fNegativeOffset ) return false;
		if ( b.fNegativeOffset ) return true;
		return (fOffset < b.fOffset);
	}
};
RELATIONAL_OPERATORS(SplicedCoordinate);

// It stores standard regions coordinates as interval [left,right].
// The condition left <= right must be always met, in other case an exception is thrown.
struct RegionCoordinates
{
private:
	unsigned fLeftPosition = std::numeric_limits<unsigned>::max();
	unsigned fRightPosition = std::numeric_limits<unsigned>::max();
public:
	static const RegionCoordinates null;
	RegionCoordinates() {}
	RegionCoordinates(unsigned left, unsigned right) : fLeftPosition(left), fRightPosition(right)
	{
		if (left>right) throw std::logic_error("RegionCoordinates: left>right in constructor");
	}
	inline bool isNull() const { return (fLeftPosition == std::numeric_limits<unsigned>::max()); }
	// set values
	inline void set(unsigned left, unsigned right)
	{
		if (left>right) throw std::logic_error("RegionCoordinates: left>right in set");
		fLeftPosition = left;
		fRightPosition = right;
	}
	inline void setLeft (unsigned value)
	{
		if (value > fRightPosition) throw std::logic_error("RegionCoordinates: left>right in setLeft" );
		fLeftPosition  = value;
	}
	inline void setRight(unsigned value)
	{
		if (value < fLeftPosition ) throw std::logic_error("RegionCoordinates: left>right in setRight");
		fRightPosition = value;
	}
	inline void setLeftAndLength (unsigned left , unsigned length)
	{
		fLeftPosition  = left;
		fRightPosition = fLeftPosition  + length;
	}
	inline void setRightAndLength(unsigned right, unsigned length)
	{
		if (length > right) throw std::logic_error("RegionCoordinates: length>right in setRightAndLength" );
		fRightPosition = right;
		fLeftPosition  = fRightPosition - length;
	}
	inline void incLeftPosition (unsigned shift)
	{
		if (fLeftPosition + shift > fRightPosition) throw std::logic_error("RegionCoordinates: left>right in incLeftPosition");
		fLeftPosition += shift;
	}
	inline void decLeftPosition (unsigned shift) { fLeftPosition -= shift; }
	inline void incRightPosition(unsigned shift) { fRightPosition += shift; }
	inline void decRightPosition(unsigned shift)
	{
		if (fLeftPosition + shift > fRightPosition) throw std::logic_error("RegionCoordinates: left>right in decRightPosition");
		fRightPosition -= shift;
	}
	// read only stuff
	inline unsigned left() const { return fLeftPosition; }
	inline unsigned right() const { return fRightPosition; }
	inline unsigned length() const { return (fRightPosition-fLeftPosition); }
	std::string toString() const;
	inline bool operator<(RegionCoordinates const & b) const
	{
		if (fLeftPosition==b.fLeftPosition) return (fRightPosition<b.fRightPosition);
		return (fLeftPosition<b.fLeftPosition);
	}
};
RELATIONAL_OPERATORS(RegionCoordinates);

inline std::ostream& operator<<(std::ostream& s, RegionCoordinates const & r)
{
	s << r.toString();
	return s;
}

// there is a special case for empty non-intronic region
struct SplicedRegionCoordinates {
private:
	SplicedCoordinate fLeftPosition;
	SplicedCoordinate fRightPosition;
public:
	SplicedRegionCoordinates() {}
	SplicedRegionCoordinates(SplicedCoordinate const & left, SplicedCoordinate const & right)
	: fLeftPosition(left), fRightPosition(right)
	{
		if ( left > right && (left.position()!=right.position() || left.offsetSize() || right.offsetSize()) ) throw std::logic_error("SplicedRegionCoordinates: left>right in constructor");
	}
	SplicedRegionCoordinates(RegionCoordinates const & region) // TODO - this should be forbidden
	{
		fRightPosition.set(region.right(), '+', 0);
		fLeftPosition .set(region.left (), '-', 0);
	}
	// set values
	inline void set(SplicedCoordinate const & left, SplicedCoordinate const & right)
	{
		if ( left > right && (left.position()!=right.position() || left.offsetSize() || right.offsetSize()) ) throw std::logic_error("SplicedRegionCoordinates: left>right in set");
		fLeftPosition = left;
		fRightPosition = right;
	}
	inline void setLeft(SplicedCoordinate const & value)
	{
		set(value,fRightPosition);
		//if (value > fRightPosition) throw std::logic_error("SplicedRegionCoordinates: left>right in setLeft" );
		//fLeftPosition  = value;
	}
	inline void setRight(SplicedCoordinate const & value)
	{
		set(fLeftPosition,value);
		//if (value < fLeftPosition ) throw std::logic_error("SplicedRegionCoordinates: left>right in setRight");
		//fRightPosition = value;
	}
	// read only stuff
	inline SplicedCoordinate left() const { return fLeftPosition; }
	inline SplicedCoordinate right() const { return fRightPosition; }
	inline bool isIntronic() const { return (fLeftPosition.offsetSize() || (! fLeftPosition.hasNegativeOffset()) || fRightPosition.offsetSize() || fRightPosition.hasNegativeOffset()); }
//	inline bool leftIsIntronic() const { return (fLeftPosition.isIntronic()); }
//	inline bool rightIsIntronic() const { return (fRightPosition.isIntronic()); }
	RegionCoordinates toRegion() const { if (isIntronic()) throw std::logic_error("SplicedRegionCoordinates::toRegion() for intronic spliced region"); return RegionCoordinates(fLeftPosition.position(),fRightPosition.position()); }
	std::string toString() const;
	inline bool operator<(SplicedRegionCoordinates const & b) const
	{
		if (fLeftPosition == b.fLeftPosition) return (fRightPosition < b.fRightPosition);
		return (fLeftPosition < b.fLeftPosition);
	}
};
RELATIONAL_OPERATORS(SplicedRegionCoordinates);


// ================================= data

//struct ExternalRecordExac
//{
//	unsigned short chromosome;
//	unsigned position;
//	std::string reference;
//	std::string alternate;
//};
//
//struct ExternalRecordDbSnp
//{
//	unsigned rs;
//};
//
//struct ExternalRecordClinVarVariation
//{
//	unsigned variationId;
//	std::vector<unsigned> rcvs;
//};
//
//struct ExternalRecordClinVarAllele
//{
//	unsigned alleleId;
//	std::string preferredName;
//};


#endif /* GLOBALS_HPP_ */
