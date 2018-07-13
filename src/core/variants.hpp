#ifndef VARIANTS_HPP_
#define VARIANTS_HPP_

#include "globals.hpp"
#include "../commonTools/stringTools.hpp"
#include "identifiers.hpp"
#include <cstdint>

enum variantCategory
{
	  nonShiftable       = 0
	, shiftableInsertion = 1
	, duplication        = 2
	, shiftableDeletion  = 3
};

// ================================= plain sequence variant

struct PlainSequenceModification
{
	RegionCoordinates region;
	std::string originalSequence = "";  // may be empty if not known
	std::string newSequence = "";
	PlainSequenceModification() = default;
	PlainSequenceModification(unsigned left, unsigned right, std::string const & pOrgSeq, std::string const & pNewSeq)
	: region(left,right), originalSequence(pOrgSeq), newSequence(pNewSeq)  {}
	std::string toString() const;
	inline bool operator<(PlainSequenceModification const & s) const
	{
		if (region != s.region) return (region < s.region);
		if (newSequence != s.newSequence) return (newSequence < s.newSequence);
		return (originalSequence < s.originalSequence);
	}
};
RELATIONAL_OPERATORS(PlainSequenceModification);
inline std::ostream & operator<<(std::ostream & s, PlainSequenceModification const & p)
{
	s << p.toString();
	return s;
}


struct PlainVariant
{
	ReferenceId refId;
	std::vector<PlainSequenceModification> modifications;
	inline bool operator<(PlainVariant const & pv) const
	{
		if (refId != pv.refId) return (refId < pv.refId);
		return (modifications < pv.modifications);
	}
};
inline std::ostream & operator<<(std::ostream & s, PlainVariant const & p)
{
	s << "(" << p.refId.value << "," << p.modifications << ")";
	return s;
}


struct PlainSequenceModificationOnSplicedReference
{
	SplicedRegionCoordinates region;
	std::string originalSequence = "";  // may be empty if not known
	std::string newSequence = "";
};

struct PlainVariantOnSplicedReference
{
	ReferenceId refId;
	std::vector<PlainSequenceModificationOnSplicedReference> modifications;
};

// ================================= normalized sequence variant
// if insertedSequence.size < newRegionLength then new sequence must be calculated
// left  aligned = (originalSequenceCycle*n)[0..(newRegionLength - insertedSequence.size)-1] + insertedSequence
// right aligned = insertedSequence + (originalSequenceCycle*n)[-(newRegionLength - insertedSequence.size)..-1]
struct NormalizedSequenceModification
{
	RegionCoordinates region;
	variantCategory category;           // from definition
	unsigned lengthChange = 0;          // length of inserted or deleted region (from definition, set for duplication & shiftable deletion, 0 for others)
	std::string insertedSequence = "";  // inserted sequence (from definition, set for non-shiftable & shiftable insertion, empty for others)
	std::string originalSequence = "";  // original sequence of left aligned variant:
	// -> non-shiftable       - sequence from region (replaced sequence)
	// -> shiftable insertion - sequence from region
	// -> duplication         - sequence from region, truncated to lengthChange
	// -> shiftable deletion  - sequence from region, truncated to lengthChange
	NormalizedSequenceModification() {}
//	NormalizedSequenceModification(RegionCoordinates const & region, std::string const & insertedSequence)
//		: region(region), newRegionLength(insertedSequence.size()), insertedSequence(insertedSequence) {}
	inline PlainSequenceModification leftAligned() const
	{
		PlainSequenceModification sm;
		switch (category) {
			case variantCategory::nonShiftable:
				sm.region = region;
				sm.originalSequence = originalSequence;
				sm.newSequence = insertedSequence;
				break;
			case variantCategory::shiftableInsertion:
				sm.region.setLeftAndLength(region.left(), 0);
				sm.originalSequence = "";
				sm.newSequence = originalSequence + insertedSequence;
				break;
			case variantCategory::duplication:
				sm.region.setLeftAndLength(region.left(), 0);
				sm.originalSequence = "";
				sm.newSequence = originalSequence;
				while ( sm.newSequence.size() < lengthChange ) sm.newSequence += originalSequence;
				sm.newSequence = sm.newSequence.substr(0,lengthChange);
				break;
			case variantCategory::shiftableDeletion:
				sm.region.setLeftAndLength(region.left(), lengthChange);
				sm.originalSequence = originalSequence;
				sm.newSequence = "";
				break;
		}
		return sm;
	}
	inline PlainSequenceModification rightAligned() const
	{
		PlainSequenceModification sm;
		switch (category) {
			case variantCategory::nonShiftable:
				sm.region = region;
				sm.originalSequence = originalSequence;
				sm.newSequence = insertedSequence;
				break;
			case variantCategory::shiftableInsertion:
				sm.region.setRightAndLength(region.right(), 0);
				sm.originalSequence = "";
				sm.newSequence = insertedSequence + originalSequence;
				break;
			case variantCategory::duplication:
				sm.region.setRightAndLength(region.right(), 0);
				sm.originalSequence = "";
				sm.newSequence = originalSequence;
				while ( sm.newSequence.size() < lengthChange ) sm.newSequence += originalSequence;
				sm.newSequence = sm.newSequence.substr(sm.newSequence.size() - lengthChange);
				break;
			case variantCategory::shiftableDeletion:
				sm.region.setRightAndLength(region.right(), lengthChange);
				sm.originalSequence = rotateLeft( originalSequence, region.length() - lengthChange );
				sm.newSequence = "";
				break;
		}
		return sm;
	}
	inline bool operator<(NormalizedSequenceModification const & b) const
	{
		if (region != b.region) return (region < b.region);
		if (category != b.category) return (category < b.category);
		if (lengthChange != b.lengthChange) return (lengthChange < b.lengthChange);
		if (insertedSequence.size() != b.insertedSequence.size()) return (insertedSequence.size() < b.insertedSequence.size());
		return (insertedSequence < b.insertedSequence);
	}
	std::string toString() const;
};
RELATIONAL_OPERATORS(NormalizedSequenceModification);
inline std::ostream & operator<<(std::ostream & s, NormalizedSequenceModification const & v) { s << v.toString(); return s; }

struct NormalizedGenomicVariant
{
	ReferenceId refId;
	std::vector<NormalizedSequenceModification> modifications;
	inline PlainVariant leftAligned() const
	{
		PlainVariant sv;
		sv.refId = refId;
		for (auto const & e: modifications) sv.modifications.push_back(e.leftAligned());
		return sv;
	}
	inline PlainVariant rightAligned() const
	{
		PlainVariant sv;
		sv.refId = refId;
		for (auto const & e: modifications) sv.modifications.push_back(e.rightAligned());
		return sv;
	}
	inline bool operator<(NormalizedGenomicVariant const & v) const
	{
		if (refId != v.refId) return (refId < v.refId);
		return (modifications < v.modifications);
	}
	std::string toString() const;
};
RELATIONAL_OPERATORS(NormalizedGenomicVariant);

struct NormalizedProteinVariant
{
	ReferenceId refId;
	std::vector<NormalizedSequenceModification> modifications;
	inline PlainVariant leftAligned() const
	{
		PlainVariant sv;
		sv.refId = refId;
		for (auto const & e: modifications) sv.modifications.push_back(e.leftAligned());
		return sv;
	}
	inline PlainVariant rightAligned() const
	{
		PlainVariant sv;
		sv.refId = refId;
		for (auto const & e: modifications) sv.modifications.push_back(e.rightAligned());
		return sv;
	}
	inline bool operator<(NormalizedProteinVariant const & v) const
	{
		if (refId != v.refId) return (refId < v.refId);
		return (modifications < v.modifications);
	}
	std::string toString() const;
};
RELATIONAL_OPERATORS(NormalizedProteinVariant);

struct VariantDetailsGenomeBuild
{
	PlainVariant definition;
	ReferenceGenome build;
	Chromosome chromosome;
	std::string hgvsDefs;
};

struct VariantDetailsGeneRegion
{
	PlainVariant definition;
	uint32_t geneId;
	std::string hgvsDefs;
};

struct VariantDetailsTranscript
{
	PlainVariantOnSplicedReference definition;
	uint32_t geneId;
	std::string hgvsDefs;
	std::string proteinHgvsDef = "";
	std::string proteinHgvsCanonical = "";
};

struct VariantDetailsProtein
{
	PlainVariant definition;
	uint32_t geneId;
	std::string hgvsDefs;
	std::vector<std::string> hgvsMatchingTranscriptVariants;
};

#endif /* VARIANTS_HPP_ */
