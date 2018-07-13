#ifndef HGVS_HPP_
#define HGVS_HPP_

#include "../referencesDatabase/referencesDatabase.hpp"
#include "../core/variants.hpp"

// ==================================== parsed HGVS expressions

//// ------ assertion to check (if there is a given sequence at given region)
//
//struct HgvsAssertion
//{
//	ReferenceId refId;
//	RegionCoordinates region;
//	std::string sequence;
//};

// ------ transcripts, genes and assembly

struct HgvsGenomicSequence
{
	std::string sequence;  // if empty then it must be calculated from the reference below
	ReferenceId refId;
	RegionCoordinates region;
	bool inversion = false;
};

struct HgvsGenomicSequenceModification  // TODO it should have two union types: plain & repetitions
{
	RegionCoordinates region;
	std::string originalSequence;  // may be empty if not known
	// these two below cannot be set at the same time
	std::vector<HgvsGenomicSequence> newSequence;
	unsigned newRepetitionsCount = 0;
};

struct HgvsGenomicVariant
{
	ReferenceId refId;
	ReferenceId contextRefId;
	std::vector<HgvsGenomicSequenceModification> firstChromosome;
	std::vector<HgvsGenomicSequenceModification> secondChromosome;
	//std::vector<HgvsAssertion> sequencesToCheck;
};

// ----- proteins

struct HgvsProteinSequence
{
	std::string sequence;  // if empty then it must be calculated from the region
	RegionCoordinates region;
};

struct HgvsProteinSequenceModification
{
	RegionCoordinates region;
	std::string originalSequence;  // may be empty if not known
	// these two below cannot be set at the same time
	std::vector<HgvsProteinSequence> newSequence;
	unsigned newRepetitionsCount = 0;
};

struct HgvsProteinVariant
{
	ReferenceId refId;
	std::vector<HgvsProteinSequenceModification> firstChromosome;
	std::vector<HgvsProteinSequenceModification> secondChromosome;
	//std::vector<HgvsAssertion> sequencesToCheck;
};

struct HgvsVariant
{
private:
	bool fIsGenomic; // false - protein
public:
	union {
		HgvsGenomicVariant genomic;
		HgvsProteinVariant protein;
	};
	HgvsVariant(bool isGenomic) : fIsGenomic(isGenomic)
	{
		if (fIsGenomic) {
			new (&(this->genomic)) HgvsGenomicVariant;
		} else {
			new (&(this->protein)) HgvsProteinVariant;
		}
	}
	HgvsVariant(HgvsVariant const & h) : fIsGenomic(h.fIsGenomic)
	{
		if (fIsGenomic) {
			new (&(this->genomic)) HgvsGenomicVariant(h.genomic);
		} else {
			new (&(this->protein)) HgvsProteinVariant(h.protein);
		}
	}
	HgvsVariant(HgvsVariant && h) : fIsGenomic(h.fIsGenomic)
	{
		if (fIsGenomic) {
			new (&(this->genomic)) HgvsGenomicVariant(h.genomic);
		} else {
			new (&(this->protein)) HgvsProteinVariant(h.protein);
		}
	}
	~HgvsVariant()
	{
		if (fIsGenomic) {
			this->genomic.HgvsGenomicVariant::~HgvsGenomicVariant();
		} else {
			this->protein.HgvsProteinVariant::~HgvsProteinVariant();
		}
	}
	HgvsVariant& operator=(HgvsVariant const & h)
	{
		if (fIsGenomic) {
			this->genomic.HgvsGenomicVariant::~HgvsGenomicVariant();
		} else {
			this->protein.HgvsProteinVariant::~HgvsProteinVariant();
		}
		fIsGenomic = h.fIsGenomic;
		if (fIsGenomic) {
			new (&(this->genomic)) HgvsGenomicVariant(h.genomic);
		} else {
			new (&(this->protein)) HgvsProteinVariant(h.protein);
		}
		return (*this);
	}
	HgvsVariant& operator=(HgvsVariant && h)
	{
		if (fIsGenomic) {
			this->genomic.HgvsGenomicVariant::~HgvsGenomicVariant();
		} else {
			this->protein.HgvsProteinVariant::~HgvsProteinVariant();
		}
		fIsGenomic = h.fIsGenomic;
		if (fIsGenomic) {
			new (&(this->genomic)) HgvsGenomicVariant(h.genomic);
		} else {
			new (&(this->protein)) HgvsProteinVariant(h.protein);
		}
		return (*this);
	}
	inline bool isGenomic() const { return fIsGenomic; }
};


void decodeHgvs ( ReferencesDatabase const* seqDb
				, std::string const &       hgvs  // input hgvs
				, HgvsVariant & decodedHgvs // out
				);

// Produce HGVS definition of modification, these definitions include neither reference name nor colon, e.g.: g.1231A>C, not NC001.2:g.1231A>C
std::string toHgvsModifications( ReferencesDatabase const* seqDb, NormalizedGenomicVariant const & );
std::string toHgvsModifications( ReferencesDatabase const* seqDb, NormalizedProteinVariant const & );


#endif /* HGVS_HPP_ */
