#ifndef ALIGNMENT_HPP_
#define ALIGNMENT_HPP_

#include <vector>
#include <string>
#include <limits>
#include <stdexcept>
#include "../core/globals.hpp"
#include "stringPointer.hpp"

// Transformation: source -> target  (applied to source gives target)
// Both source and target are compact and consistent regions
// Contains source_ref_id, source_strand(positive/negative), source_interval and target_interval
struct Alignment
{
private:
	struct Element {
		// first - length of sequence of matching bp
		unsigned matchedBp = 0;
		// second - length of sequence to delete
		unsigned deletedBp = 0;
		// third - sequence to insert
		std::string insertedSeq = "";
		// helpers
		inline unsigned sourceLength() const { return (matchedBp + deletedBp); }
		inline unsigned targetLength() const { return (matchedBp + insertedSeq.size()); }
	};
	std::vector<Element> elements;
public:
	unsigned sourceLeftPosition;
	unsigned targetLeftPosition;
	ReferenceId sourceRefId;
	bool sourceStrandNegativ;
	unsigned sourceLength() const { unsigned x = 0; for (auto const & e: elements) x += e.matchedBp + e.deletedBp; return x; }
	unsigned targetLength() const { unsigned x = 0; for (auto const & e: elements) x += e.matchedBp + e.insertedSeq.size(); return x; }
	// the length of given source must equal sourceLength
	std::string processSourceSubSequence(std::string const & source) const;
	// These 4 function modify alignment in place.
	// They cut alignment from left or right side in target space.
	// Empty regions (in target space) laying on the border are removed.
	// Functions exactTarget cut exactly the given number of bp in target space.
	// Functions exactAlignment cut the smallest number of bp in target space >= given number that preserves alignment.
	// Functions return number of bp cut in source space.
	// Functions throw exception std::out_of_range if parameter length > length of alignment in target space.
	unsigned targetCutLeft_exactTarget(unsigned length);
	unsigned targetCutRight_exactTarget(unsigned length);
	unsigned targetCutLeft_exactAlignment(unsigned & length);
	unsigned targetCutRight_exactAlignment(unsigned & length);
	// pos and length may lay outside the sequence, like for string::substr - it returns alignments for intersection
	// if there is no intersection, the exception is thrown, pos & length must be given according to target coordinates
	// region empty on the target side are removed from borders
	Alignment targetSubalign(unsigned pos, unsigned length = std::numeric_limits<unsigned>::max(), bool exactTargetEdges = false) const;
	// calculate reverse - switch source <-> target
	// parameters: values from state before reverse, sourceSeq always from + strand
	Alignment reverse(StringPointer const & sourceSeq, ReferenceId targetRefId) const;
	// add Alignment at the end - positions must match appropriately
	void append(Alignment const &);
	// set value
	void set(std::string const & cigar, ReferenceId srcRefId, bool srcNegativeStrand, std::string const & srcSubseq, unsigned srcPos, std::string const & trgSubseq, unsigned trgPos);
	void set(std::string const & cigar, ReferenceId srcRefId, bool srcNegativeStrand, unsigned srcPos, unsigned trgPos, std::vector<std::string> const & insertedOrModifiedSequences);
	// returns true <=> target = source (but still may be on different strand)
	bool isPerfectMatch() const;
	bool isIdentical() const;
	// returns regions
	RegionCoordinates targetRegion() const { return RegionCoordinates( targetLeftPosition, targetLeftPosition + targetLength() ); }
	RegionCoordinates sourceRegion() const { return RegionCoordinates( sourceLeftPosition, sourceLeftPosition + sourceLength() ); }
	// returns CIGAR string (withInsertions will add the insertions after the cigar, separated by single spaces)
	std::string cigarString(bool withInsertions = false) const;
	// create alignment from cigar, positions and sequences
	static Alignment createFromFullSequences(std::string const & cigar, ReferenceId srcRefId, bool srcNegativeStrand, std::string const & srcFullSeq, unsigned srcPos, std::string const & trgFullSeq, unsigned trgPos);
	// create identity alignment (matches only)
	static Alignment createIdentityAlignment(ReferenceId srcRefId, bool srcNegativeStrand, unsigned srcPos, unsigned trgPos, unsigned length);
	// to string
	std::string toString() const;
};



// Transformation: source -> target
// Target is compact and consistent, source may be not
struct GeneralSeqAlignment
{
	struct Element {
		// unaligned sequence
		std::string unalignedSeq;
		// alignment
		Alignment alignment;
		// quick fix for large references
		mutable unsigned targetOffset = std::numeric_limits<unsigned>::max();
		// method
		std::string toString() const { return "{" + unalignedSeq + "," + alignment.toString() + "}"; }
	};
	std::vector<Element> elements;
	// create identity alignments
	static GeneralSeqAlignment createIdentityAlignment(ReferenceId refId, RegionCoordinates const & region);
	// returns true <=> target = source (but still may be on different strand)
	bool isPerfectMatch() const
	{
		if (elements.size() != 1 || elements[0].unalignedSeq != "") return false;
		return elements[0].alignment.isPerfectMatch();
	}
	// convert to simple alignments if possible (GeneralAlignment must be compact)
	Alignment toAlignment() const
	{
		if (elements.size() != 1 || elements[0].unalignedSeq != "") throw std::runtime_error("GeneralSeqAlignment::toAlignment() - alignment is not compact!!!");
		return elements[0].alignment;
	}
	// toString
	std::string toString() const;
};



#endif /* ALIGNMENT_HPP_ */
