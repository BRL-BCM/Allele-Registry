#include "canonicalization.hpp"
#include "../core/exceptions.hpp"
#include "smithWaterman.hpp"
#include "proteinTools.hpp"
#include <algorithm>


inline NormalizedSequenceModification normalize
	( ReferencesDatabase const * refDb
	, ReferenceId const refId
	, PlainSequenceModification const & sm
	)
{
	unsigned const varLength = std::max(sm.region.length(),static_cast<unsigned>(sm.newSequence.length()));
	if (varLength > 10000) throw ExceptionVariationTooLong(varLength,"");
	RegionCoordinates region = sm.region;
	std::string orgAllele = refDb->getSequence(refId, sm.region);
	std::string newAllele = sm.newSequence;
	if (sm.originalSequence != orgAllele && sm.originalSequence != "") {
		throw ExceptionIncorrectReferenceAllele(refDb->getNames(refId).front(), region, sm.originalSequence, orgAllele);
	}

	// -------- we do not canonicalize identity transforms - just copy & exit
	if (orgAllele == newAllele) {
		NormalizedSequenceModification cm;
		cm.category = variantCategory::nonShiftable;
		cm.region = region;
		cm.insertedSequence = newAllele;
		cm.originalSequence = orgAllele;
		return cm;
	}

	// -------- reduce
	// cut right
	while ( orgAllele != "" && newAllele != "" && orgAllele.back() == newAllele.back() ) {
		orgAllele.pop_back();
		newAllele.pop_back();
	}
	region.decRightPosition( region.length() - orgAllele.size() );
	// cut left
	while ( orgAllele != "" && newAllele != "" && orgAllele.front() == newAllele.front() ) {
		orgAllele = orgAllele.substr(1);
		newAllele = newAllele.substr(1);
	}
	region.incLeftPosition( region.length() - orgAllele.size() );

	// -------- this is all for indels - copy reduced form & exit
	if ( orgAllele != "" && newAllele != "" ) {
		NormalizedSequenceModification cm;
		cm.category = variantCategory::nonShiftable;
		cm.region = region;
		cm.insertedSequence = newAllele;
		cm.originalSequence = orgAllele;
		return cm;
	}

	// -------- calculate whole influenced region - find possible shifting for insertion and deletion
	std::string pattern = (orgAllele != "") ? orgAllele : newAllele;
	// extend right
	unsigned const chunkSize = 256;
	unsigned const maxRight = refDb->getSequenceLength(refId);
	std::string tempSeq = "";
	unsigned rightBoundary = region.right();
	for ( ; rightBoundary < maxRight;  ++rightBoundary ) {
		unsigned const patternPos = (rightBoundary - region.right()) % pattern.size();
		unsigned const chunkPos   = (rightBoundary - region.right()) % chunkSize;
		if (chunkPos == 0) tempSeq = refDb->getSequence(refId, RegionCoordinates(rightBoundary, std::min(rightBoundary+chunkSize,maxRight)));
		if (pattern[patternPos] != tempSeq[chunkPos]) break;
	}
	// extend left
	unsigned leftBoundary = region.left();
	for ( ;  leftBoundary > 0;  --leftBoundary ) {
		unsigned const patternPos = (region.left() - leftBoundary) % pattern.size();
		unsigned const chunkPos   = (region.left() - leftBoundary) % chunkSize;
		if (chunkPos == 0) tempSeq = refDb->getSequence(refId, RegionCoordinates(leftBoundary-std::min(leftBoundary,chunkSize),leftBoundary));
		if (pattern[pattern.size()-1-patternPos] != tempSeq[tempSeq.size()-1-chunkPos]) break;
	}

	// -------- calculate canonical form
	NormalizedSequenceModification cm;
	cm.region.set(leftBoundary, rightBoundary);
	unsigned const shiftLeft  = region.left() - leftBoundary;
	unsigned const shiftRight = rightBoundary - region.right();
	if ( shiftLeft == 0 && shiftRight == 0 ) {
		cm.category = variantCategory::nonShiftable;
		cm.insertedSequence = newAllele;
		cm.originalSequence = orgAllele;
	} else if ( newAllele.size() > shiftLeft + shiftRight ) {
		cm.category = variantCategory::shiftableInsertion;
		cm.insertedSequence = newAllele.substr( shiftRight, newAllele.size() - shiftLeft - shiftRight );
		cm.originalSequence = rotateRight(newAllele,shiftLeft).substr( 0, shiftLeft + shiftRight );
	} else if ( newAllele.size() > 0 ) {
		cm.category = variantCategory::duplication;
		cm.lengthChange = newAllele.size();
		cm.originalSequence = rotateRight(newAllele,shiftLeft);
	} else { // orgAllele.size > 0 - deletion
		cm.category = variantCategory::shiftableDeletion;
		cm.lengthChange = orgAllele.size();
		cm.originalSequence = rotateRight(orgAllele,shiftLeft);
	}
	return cm;
}


NormalizedGenomicVariant canonicalizeGenomic
	( ReferencesDatabase const * refDb
	, PlainVariant const & simpleVar
	)
{
	NormalizedGenomicVariant canonicalVar;
	canonicalVar.refId = simpleVar.refId;

	// ========================= For now, we canonicalize each modification separately
	for (PlainSequenceModification const & sm: simpleVar.modifications) {
		canonicalVar.modifications.push_back( normalize(refDb, simpleVar.refId, sm) );
		for (char c: canonicalVar.modifications.back().insertedSequence) {
			if (c != 'A' && c != 'C' && c != 'G' && c != 'T') throw std::runtime_error("Incorrect or unsupported base-pair in given sequence"); // TODO
		}
		if (canonicalVar.modifications.back().category == variantCategory::duplication || canonicalVar.modifications.back().category == variantCategory::shiftableInsertion) {
			for (char c: canonicalVar.modifications.back().originalSequence) {
				if (c != 'A' && c != 'C' && c != 'G' && c != 'T') throw std::runtime_error("Incorrect or unsupported base-pair in reference sequence"); // TODO
			}
		}
	}

	// =================== sort modifications, check if they do not overlap
	std::sort( canonicalVar.modifications.begin(), canonicalVar.modifications.end() );
	for ( unsigned i = 1;  i < canonicalVar.modifications.size();  ++i ) {
		NormalizedSequenceModification & m1 = canonicalVar.modifications[i-1];
		NormalizedSequenceModification & m2 = canonicalVar.modifications[i];
		// special case - when there are multiple operations on the same region
		if (m1.region == m2.region) {
			// merge current element with the previous one if possible
			if (m1.category == shiftableDeletion && m2.category == shiftableDeletion) {
				m1.lengthChange += m2.lengthChange;
				m1.originalSequence += m2.originalSequence;
			} else if (m1.category == duplication && m2.category == duplication) {
				m1.lengthChange += m2.lengthChange;
				m1.originalSequence += m2.originalSequence;
			} else {
				throw ExceptionComplexAlleleWithOverlappingSimpleAlleles();
			}
			// remove current element
			for ( unsigned j = i+1; j < canonicalVar.modifications.size(); ++j )
				canonicalVar.modifications[j-1] = canonicalVar.modifications[j];
			canonicalVar.modifications.pop_back();
			--i;
		} else {
			if (m1.region.right() > m2.region.left() || m1.region.left() == m2.region.left()) throw ExceptionComplexAlleleWithOverlappingSimpleAlleles();
		}
	}

	return canonicalVar;
}


NormalizedProteinVariant canonicalizeProtein
	( ReferencesDatabase const * refDb
	, PlainVariant const & simpleVar2
	)
{
	PlainVariant pv = simpleVar2;
	// ======================== check reference alleles
	for (auto & m: pv.modifications) {
		if (m.region.length() == 0) continue;
		std::string const seq = refDb->getSequence(pv.refId, m.region);
		if ( ! m.originalSequence.empty() ) {
			if (m.originalSequence != seq) {
				throw ExceptionIncorrectReferenceAllele( refDb->getNames(pv.refId).front(), m.region
						, convertOneLetterAminoAcidSequenceToThreeLetters(m.originalSequence)
						, convertOneLetterAminoAcidSequenceToThreeLetters(seq) );
			}
		} else {
			m.originalSequence = seq;
		}
	}
	// ======================== normalize with SW algorithm
	{
		bool normalizationNotNeeded = false; // workaround to speedup protein calculations
		if (pv.modifications.size() == 1) {
			PlainSequenceModification & psm = pv.modifications.front();
			if (psm.newSequence == psm.originalSequence) {
				throw std::runtime_error("There is no change on amino-acid level, only protein modifications can be registered"); //TODO
			} else if (psm.newSequence.find('*') == std::string::npos && psm.originalSequence.find('*') == std::string::npos) {
				// trim left
				while ( ! psm.newSequence.empty() && ! psm.originalSequence.empty()
						&& psm.newSequence.front() == psm.originalSequence.front() )
				{
					psm.newSequence = psm.newSequence.substr(1);
					psm.originalSequence = psm.originalSequence.substr(1);
					psm.region.incLeftPosition(1);
				}
				// trim right
				while ( ! psm.newSequence.empty() && ! psm.originalSequence.empty()
						&& psm.newSequence.back() == psm.originalSequence.back() )
				{
					psm.newSequence.resize(psm.newSequence.size()-1);
					psm.originalSequence.resize(psm.originalSequence.size()-1);
					psm.region.decRightPosition(1);
				}
				if (psm.newSequence.size() < 2 && psm.originalSequence.size() < 2) {
					normalizationNotNeeded = true;
				}
			}
		}
		if (! normalizationNotNeeded) {
			unsigned const offset = pv.modifications.front().region.left();
			unsigned total_length = refDb->getMetadata(pv.refId).length;
		//	total_length = std::min(total_length, pv.modifications.back().region.right() + 20u);  // TODO - workaround for efficiency - max 20 aa are checked
			std::string seq1 = refDb->getSequence(pv.refId, RegionCoordinates(offset,total_length));
			// TODO == workaround - there is some problem with protein length (sometimes termination codon is included)
			total_length = seq1.size() + offset;
			if (pv.modifications.back().region.right() > total_length) pv.modifications.back().region.setRight(total_length);
			// ============================
			std::string seq2 = applyVariantsToTheSequence(seq1, offset, pv.modifications);
			if ( ! tryDescribeAsProteinVariant( seq1, seq2, offset, pv.modifications ) ) {
				throw ExceptionProteinVariantTooLong( std::max(seq1.size(),seq2.size()) );
			}
		} else {
			if (pv.modifications.front().newSequence.size() > 7) {
				throw ExceptionProteinVariantTooLong( pv.modifications.front().newSequence.size() );
			}
		}
	}

	NormalizedProteinVariant canonicalVar;
	canonicalVar.refId = pv.refId;

	// ========================= For now, we canonicalize each modification separately
	for (PlainSequenceModification const & sm: pv.modifications) {
		canonicalVar.modifications.push_back( normalize(refDb, pv.refId, sm) );
	}

	// =================== sort modifications, check if they do not overlap
	std::sort( canonicalVar.modifications.begin(), canonicalVar.modifications.end() );
	for ( unsigned i = 1;  i < canonicalVar.modifications.size();  ++i ) {
		NormalizedSequenceModification & m1 = canonicalVar.modifications[i-1];
		NormalizedSequenceModification & m2 = canonicalVar.modifications[i];
		// special case - when there are multiple operations on the same region
		if (m1.region == m2.region) {
			// merge current element with the previous one if possible
			if (m1.category == shiftableDeletion && m2.category == shiftableDeletion) {
				m1.lengthChange += m2.lengthChange;
				m1.originalSequence += m2.originalSequence;
			} else if (m1.category == duplication && m2.category == duplication) {
				m1.lengthChange += m2.lengthChange;
				m1.originalSequence += m2.originalSequence;
			} else {
				throw ExceptionComplexAlleleWithOverlappingSimpleAlleles();
			}
			// remove current element
			for ( unsigned j = i+1; j < canonicalVar.modifications.size(); ++j )
				canonicalVar.modifications[j-1] = canonicalVar.modifications[j];
			canonicalVar.modifications.pop_back();
			--i;
		} else {
			if (m1.region.right() > m2.region.left() || m1.region.left() == m2.region.left()) throw ExceptionComplexAlleleWithOverlappingSimpleAlleles();
		}
	}

	if (canonicalVar.modifications.empty())
		throw std::runtime_error("There is no change on amino-acid level, only protein modifications can be registered"); //TODO

	return canonicalVar;
}


NormalizedGenomicVariant canonicalize
	( ReferencesDatabase const * refDb
	, HgvsGenomicVariant const & var
	)
{
	PlainVariant simpleVar;
	simpleVar.refId = var.refId;
	for (HgvsGenomicSequenceModification const & hm: var.firstChromosome) {
		PlainSequenceModification sm;
		sm.region = hm.region;
		if (hm.newRepetitionsCount == 0) {
			for (HgvsGenomicSequence const & hs: hm.newSequence) {
				if ( hs.region.isNull() ) {
					sm.newSequence += hs.sequence;
				} else {
					std::string t = refDb->getSequence(hs.refId, hs.region);
					if (hs.inversion) convertToReverseComplementary(t);
					sm.newSequence += t;
				}
			}
			sm.originalSequence = hm.originalSequence;
		} else {
			// repetitions
			if (sm.region.length() == 0) throw ExceptionAssertionFailed("Incorrect HGVS definition - repetition with empty region");
			if (sm.region.length() != hm.originalSequence.size()) throw ExceptionAssertionFailed("Incorrect HGVS definition - repetition region and original sequence mismatch");
			RegionCoordinates regionWithFirstMismatch = sm.region;
			unsigned repetitions = 0;
			unsigned const maxRight = refDb->getSequenceLength(simpleVar.refId);
			while ( regionWithFirstMismatch.right() <= maxRight && refDb->getSequence(simpleVar.refId,regionWithFirstMismatch) == hm.originalSequence ) {
				regionWithFirstMismatch.incLeftPosition ( sm.region.length() );
				regionWithFirstMismatch.incRightPosition( sm.region.length() );
				++repetitions;
			}
			if (hm.newRepetitionsCount < repetitions) {
				// deletion
				sm.region.incRightPosition( (repetitions - hm.newRepetitionsCount - 1) * sm.region.length() );
			} else if (hm.newRepetitionsCount == repetitions) {
				// the same
				sm.region.setRight(regionWithFirstMismatch.left());
				for (unsigned i = 0; i < repetitions; ++i) sm.newSequence += hm.originalSequence;
			} else {
				// insertion
				sm.region.setRight(sm.region.left());
				for (unsigned i = 0; i < (hm.newRepetitionsCount - repetitions); ++i) sm.newSequence += hm.originalSequence;
			}
		}
		simpleVar.modifications.push_back(sm);
	}
	return canonicalizeGenomic(refDb, simpleVar);
}


NormalizedProteinVariant canonicalize
	( ReferencesDatabase const * refDb
	, HgvsProteinVariant const & var
	)
{
	PlainVariant simpleVar;
	simpleVar.refId = var.refId;
	for (HgvsProteinSequenceModification const & hm: var.firstChromosome) {
		PlainSequenceModification sm;
		sm.region = hm.region;
		if (hm.newRepetitionsCount == 0) {
			for (HgvsProteinSequence const & hs: hm.newSequence) {
				if ( hs.region.isNull() ) {
					sm.newSequence += hs.sequence;
				} else {
					sm.newSequence += refDb->getSequence(var.refId, hs.region);
				}
			}
			sm.originalSequence = hm.originalSequence;
		} else {
			// repetitions
			if (sm.region.length() == 0) throw ExceptionAssertionFailed("Incorrect HGVS definition - repetition with empty region");
			if (sm.region.length() != hm.originalSequence.size()) throw ExceptionAssertionFailed("Incorrect HGVS definition - repetition region and original sequence mismatch");
			RegionCoordinates regionWithFirstMismatch = sm.region;
			unsigned repetitions = 0;
			unsigned const maxRight = refDb->getSequenceLength(simpleVar.refId);
			while ( regionWithFirstMismatch.right() <= maxRight && refDb->getSequence(simpleVar.refId,regionWithFirstMismatch) == hm.originalSequence ) {
				regionWithFirstMismatch.incLeftPosition ( sm.region.length() );
				regionWithFirstMismatch.incRightPosition( sm.region.length() );
				++repetitions;
			}
			if (hm.newRepetitionsCount < repetitions) {
				// deletion
				sm.region.incRightPosition( (repetitions - hm.newRepetitionsCount - 1) * sm.region.length() );
			} else if (hm.newRepetitionsCount == repetitions) {
				// the same
				sm.region.setRight(regionWithFirstMismatch.left());
				for (unsigned i = 0; i < repetitions; ++i) sm.newSequence += hm.originalSequence;
			} else {
				// insertion
				sm.region.setRight(sm.region.left());
				for (unsigned i = 0; i < (hm.newRepetitionsCount - repetitions); ++i) sm.newSequence += hm.originalSequence;
			}
		}
		simpleVar.modifications.push_back(sm);
	}
	return canonicalizeProtein(refDb, simpleVar);
}



#ifdef STANDALONE
#include <iostream>

struct testVar
{
	std::string name;
	unsigned position;
	std::string refAllele;
	std::string newAllele;
	unsigned intronOffset;
	bool intronOffsetNegative;
	testVar(std::string pname,unsigned pposition,std::string prefAllele,std::string pnewAllele)
	: name(pname), position(pposition), refAllele(prefAllele), newAllele(pnewAllele), intronOffset(std::numeric_limits<unsigned>::max()), intronOffsetNegative(false) {}
	testVar(std::string pname,unsigned pposition,char pintronOffsetSign,unsigned pintronOffset,std::string prefAllele,std::string pnewAllele)
	: name(pname), position(pposition), refAllele(prefAllele), newAllele(pnewAllele), intronOffset(pintronOffset), intronOffsetNegative(pintronOffsetSign == '-') {}
};

void testCanonicalization
	( ReferenceSequencesDatabase const* seqDb
	, std::vector<testVar> const &      testAlleles
	, std::string const &               outSequenceName
	, unsigned    const &               outPosition
	, std::string const &               outRefAllele
	, std::string const &               outNewAllele
	)
{
	RegionCoordinates outRegion( outPosition, outPosition + outRefAllele.size() );
	for (testVar const & t: testAlleles) {
		try {
			GeneralRegionCoordinates inRegion;
			inRegion.leftPosition = t.position;
			if (t.intronOffset < std::numeric_limits<unsigned>::max()) { // is intronic
				inRegion.rightPosition = t.position;
				inRegion.leftExternalOffset = t.intronOffset;
				inRegion.leftExternalOffsetNegative = t.intronOffsetNegative;
				inRegion.rightExternalOffset = t.intronOffset;
				inRegion.rightExternalOffsetNegative = t.intronOffsetNegative;
				if (t.intronOffsetNegative) {
					inRegion.rightExternalOffset -= t.refAllele.size();
				} else {
					inRegion.rightExternalOffset += t.refAllele.size();
				}
			} else {
				inRegion.rightPosition = t.position + t.refAllele.size();
			}
			std::string       inRefAllele = "";
			std::string       cnnSequenceName;
			RegionCoordinates cnnRegion;
			std::string       cnnRefAllele;
			std::string       cnnNewAllele;
			canonicalize(seqDb, t.name, inRegion, inRefAllele, t.newAllele, cnnSequenceName, cnnRegion, cnnRefAllele, cnnNewAllele);
			if (inRefAllele     != t.refAllele    ) throw std::runtime_error("Source reference allele does not match: " + inRefAllele + " != " + t.refAllele);
		//	if (cnnSequenceName != outSequenceName) throw std::runtime_error("Sequence name does not match: " + cnnSequenceName + " != " + outSequenceName);
			if (cnnRegion       != outRegion      ) throw std::runtime_error("Region does not match: " + cnnRegion.toString() + " != " + outRegion.toString());
			if (cnnRefAllele    != outRefAllele   ) throw std::runtime_error("Target reference allele does not match: " + cnnRefAllele + " != " + outRefAllele);
			if (cnnNewAllele    != outNewAllele   ) throw std::runtime_error("New allele does not match: " + cnnNewAllele + " != " + outNewAllele);
		} catch (...) {
			std::cerr << "Error for " << t.name << " " << t.position << " " << t.refAllele << " " << t.newAllele << std::endl;
			throw;
		}
	}
}

void test1( ReferenceSequencesDatabase const* seqDb )
{
	std::vector<testVar> testAlleles;
	testAlleles.push_back( testVar("NM_000546.5"   , 530, "G", "A") );
	testAlleles.push_back( testVar("NM_001126112.2", 527, "G", "A") );
	testAlleles.push_back( testVar("NM_001126113.2", 530, "G", "A") );
	testAlleles.push_back( testVar("NM_001126114.2", 530, "G", "A") );
	testAlleles.push_back( testVar("NM_001126118.1", 647, "G", "A") );
	testAlleles.push_back( testVar("NC_000017.11", 7676039, "C", "T") );
	testCanonicalization( seqDb, testAlleles, "NC_000017.11", 7676039, "C", "T" );
}

void test2( ReferenceSequencesDatabase const* seqDb )
{
	std::vector<testVar> testAlleles;
	testAlleles.push_back( testVar("NM_007294.3", 366, '+',    0, "G", "T") );
	testAlleles.push_back( testVar("NM_007297.3", 274, '+', 8291, "G", "T") );
	testCanonicalization( seqDb, testAlleles, "NC_000017.11", 43115724, "C", "A" );
}

void test3( ReferenceSequencesDatabase const* seqDb )
{
	std::vector<testVar> testAlleles;
	testAlleles.push_back( testVar("NM_000546.5", 1302, '+', 12, "A", "G") );
	testAlleles.push_back( testVar("NM_001126112.2", 1299, '+', 12, "A", "G") );
	testCanonicalization( seqDb, testAlleles, "NC_000017.11", 7670595, "T", "C" );
}

void test4( ReferenceSequencesDatabase const* seqDb )
{
	std::vector<testVar> testAlleles;
	testAlleles.push_back( testVar("NM_000038.5", 814, "AG", "") );
	testAlleles.push_back( testVar("NM_001127510.2", 922, "AG", "") );
	testAlleles.push_back( testVar("NM_001127511.2", 888, "AG", "") );
	testCanonicalization( seqDb, testAlleles, "NC_000005.10", 112801276, "AG", "" );
}

void test5( ReferenceSequencesDatabase const* seqDb )
{
	std::vector<testVar> testAlleles;
	testAlleles.push_back( testVar("NM_000314.6", 1108, "C", "T" ) );
	testCanonicalization( seqDb, testAlleles, "NC_000010.11", 87864546, "C", "T" );
}

// this is a deletion defined on the exons' boundary - the smallest range is chosen
// test procedure do not work for transcript allele crossing exons boundary
// this example is really incorrect, see bug #233
//void test6( ReferenceSequencesDatabase const* seqDb )
//{
//	std::vector<testVar> testAlleles;
//	testAlleles.push_back( testVar("NM_015214.2", 1625, "GCTTTATGTACAGACCGAGATCTTCAGGAAATAGGAATTCCTTTAGGACCAAGAAAGAAGATATTAAACTATTTCAGCACCAGAAAAAACTCAATGGGTATT"
//	"AAGAGACCAGCCCCGCAGCCTGCTTCAGGGGCAAACATCCCCAAAGAATCTGAGTTCTGCAGTAGCAGTAATACTAGAAATGGTGACTATCTGGATGTTGGCATTGGGCAGGTGTCTGTGAAATACCCCCGGCTCATCTATAAACCAGAGATAT"
//	"TCTTTGCCTTTGGATCTCCCATTGGAATGTTCCTTACTGTCCGAGGACTAAAAAGAATTGATCCCAACTACAGATTTCCAACGTGCAAAGGTTTCTTCAATATTTATCACCCTTTTGATCCTGTGGCCTATAGGATTGAACCAATGGTGGTCCC"
//	"AGGAGTGGAATTTGAGCCAATGCTGATCCCACATCATAAAGGCAGGAAGCGGATGCACTTAGAACTGAGAGAGGGCTTGACCAGGATGAGTATGGACCTTAAGAACAACTTGCTAGGTTCGCTGCGGATGGCCTGGAAGTCTTTTACCAGAGCT"
//	"CCATACCCTGCCTTACAAGCTTCAGAAACACCAGAAGAAACTGAAGCAGAACCTGAATCAACTTCAGAGAAGCCTAGTG", "" ) );
//	// left alignment shifts it by 1
//	testCanonicalization( seqDb, testAlleles, "NC_000008.11", 38249706, "GGCTTTATGTACAGACCGAGATCTTCAGGAAATAGGAATTCCTTTAGGACCAAGAAAGAAGATATTAAACTATTTCAGCACCAGAAA"
//	"AAACTCAATGGTATGTGCCTAATACAGCTTGTTGGACTAAACAATTCTTTGATGTCCATAGTGTGTCCTTGTGCTTTGTGGGATGTTTACTCTGGGGAGCCAAGCAGTTTTTTGGTAATTGATCTTTGGCTCTTTTTTTTTGAGACAGGGTCTC"
//	"GCTCTGTCACTCAGGCTGGAGTGCAGTGGTGTGATCCCAGCTCATTGCAACCTCTGCCTCCCGGGTTCAAGCGATTCTCCTGCCTCAGCCTCTCGAGTAGCTGGGACTACAGGCGCCCACCACCATGCCTGGCTAATTTTTGTATTTTTAATAG"
//	"AGACGGTGTTTCACCATATTGGCCAGGCTGGTCTCAAACTCCTGACCTTGTGATCTGCCCGCCTTGGCCTCCCAAAGTGCTGGGATTACAGGTGTGAGCCACCCGCCTGGCCGATCTTTGGCTCTCTTTCTAGAAATTAAACACATTTTCTTTT"
//	"TTACATTCTTTTTTTTTTCTTTTCCTGGAAATGAAATCTCGCTGTGTTGCCCGGGGTGGAGTATAGTGGGTGGTCTCGGCTCACTGCAACCTCCACCTTCCAGGTTCAAGCAATTCTCCTGCTTCATCCTCTTGAGTAGCTGGGATTCCAGGTG"
//	"TGCACCACCATGCCCAGCTAACTTTTTGTATTTTTAGTAGAGACGGGGTTTCACTATGTTGGCCAGGCTGGTCTTGAACTCCTGACCTCAGATGACCCATGTGCCTCAGCCTCCCAAAGTGCTGCACCGCTCCTGGCCCTTCTTTTTTTACATC"
//	"CTTAAAAGTATTATTAGCTTCAGAAGTCTTCTCTCCTCTGGAATTCTCCTACCATTTATTTATTTATTTATTTTTTGCTTTTTTCTTTTCATGAGAGTTTTACCAATATCTACTTCACATCACATACCATAAAATTCACCCTTTTAAAGTTTAC"
//	"AATTTAATGGTTTTTAGTATATTCATAGAGTTATGTAACCATTACCACTATCCAATTTCAGCACATTTCCATTGTCCCTTAGGGAAACCCCATACCCATTTGCTGTCACTCCCCATTGCTCCCTGCCCCCAGCCTCTGTCAACCACTAATCTAC"
//	"TTTCTGTTTCTACAGATTTGCTTATTTTGGACATTCCATATAAATGGAATGTCCTTTTGAGCCATGTGTCTGACTTCTTTCACTTAGCATAATATTCTGAAGGTTTACCTGTGTTAACAGTACTTCCACATTCAGTTTTTCTGGTCACTGAATT"
//	"TCTAACTCCATATCTTATTAGTTAGTTTTTTTCTTTTTGAGTCGGAGTGTTGCTCTGTCGCCCAGGATGGAGTGCAGTTGCACGATCTTGGCTCAATTGCAACCTCCGCCTCCTGGATTCAAGTGATTCTCCTGCTTCAGCTTCCTGAGTAGCT"
//	"GGGATTACAGACACGCGACACCACACCCAGCTAATTTTTGTATTTTTAGTATGGGGTTTCACCATGTTGGCCAGGCTGGTCTTGAGCTCCTGACCTCAAGTGATCCGCCCACCTCAGCCTCCCAAAGTGCTGGGATTACAGGCGTGAGCCACCA"
//	"TGCCTGGCCAGTAGTTAGTTCTTTTCCAACTTTTTAGCAGGTGGCTCATTTCTTTTTAAAGACTCAGTTCTTGGCCCAGCCCATCGACATCCTACTTCCCTTGATTACAGAGGCACAGCACTTCATAGAACTCTAGCAATCCTCAGCATTATCT"
//	"TCCCACAGTGCCCTGCTATTGTTTTCTCTTATGATTGTTGCCTAAAGTTTCAACGCTACTGAAATGGCTATGTACAACTTTCATCATTTTAGAGCATCTAAGAATGTCAATTAACAATTTAAGGCAATAATTTAATCTTGTTTTCTGAGGAAAT"
//	"CTTCCTAGAATTTTTGGCTGCCATTTCAGAACTTTCCCAAGGGTCCTTTCAAACCAGTAGAAAACTATATAATCCTTTGGCTATACTTTTACTATATAGTAGCTCCCCATACTTTTTTTTTAAATAGAGACTATGTTGCCCACGTTGGACTTGA"
//	"ACTCTTGGGCTTAAACAATCCTACCTACCTACCTACCTCAGGGTCCTGAGTAGCTGGGACTACAGGTGCATGCCGTCATGCCCAGCTTCTCCACATAACTATTTTTTATTCTTTAGGGTATTAAGAGACCAGCCCCGCAGCCTGCTTCAGGGGC"
//	"AAACATCCCCAAAGAATCTGAGTTCTGCAGTAGCAGTAATACTAGAAATGGTGACTATCTGGATGTTGGCATTGGGCAGGTAACTAATTCTCTTTGATCATTTTACAGCCTGCTATTTGTATGGTAGAAACAGATGTAACTTTTGTTATTAATG"
//	"AGAGATGCTTTTATTTTCCTCCTTTGAGGTGTCTGTGAAATACCCCCGGCTCATCTATAAACCAGAGATATTCTTTGCCTTTGGATCTCCCATTGGAATGTTCCTTACTGTCCGAGGACTAAAAAGAATTGATCCCAACTACAGATTTCCAACG"
//	"TGCAAAGGTTTCTTCAATATTTATCACCCTGTAAGCATTGTACAGCTATTGTGGTTTTACCTAAATATCAGGGCTTATTGTTAAAGAACATAGGATTTTCTGTTGTGTTGATTTTTTTTTCCTATTCCATTGTCTAGCAATGAGACAACTTATA"
//	"ATAGTATTGATTTGGAAGTGATGGAGCAGAACTGTACATTTTTTTAAATGAACTTATGATTATTGTTTAAGATACATAGTTCTGGTTGTACATGGTGACTAATGCCTGTCATCCCAGCACTTTGGGAGGTCAAGGCAGGAGGATTGCTTGAGCC"
//	"TAGGAGTTGAAGGCTACAGTGAGCCATGATTGCACCACTGCACTCCAGCCTGGGTGACAGAGCGAGACCTCATCTCAAAAAAAAAAAAAAAAAAGTTATATAGTTTCCAGACTGTGCTTAGCAGAGGTAAATGTAGCTCTTGTTTTTCCTATGT"
//	"AGTTTGATCCTGTGGCCTATAGGATTGAACCAATGGTGGTCCCAGGAGTGGAATTTGAGCCAATGCTGATCCCACATCATAAAGGCAGGAAGCGGATGCACTTAGGTAAGTCCGAGCATAGAACTTGATAATTCTAGACCTTTTGGCCAGTGCA"
//	"AAGGGCATCATTCACTCTGTTCAGCTATGTTGGACCCTAAGCTCTTTGTAAATCATTTAATGTTCTTTATTTATATGTTTCAGAACTGAGAGAGGGCTTGACCAGGATGAGTATGGACCTTAAGAACAACTTGCTAGGTTCGCTGCGGATGGCC"
//	"TGGAAGTCTTTTACCAGAGCTCCATACCCTGCCTTACAAGCTTCAGAAACACCAGAAGAAACTGAAGCAGAACCTGAATCAACTTCAGAGAAGCCTAGT", "" );
//}

int main()
{
	try {
		ReferenceSequencesDatabase db;
		std::cout << "Test 1 ... " << std::flush;
		test1(&db);
		std::cout << "OK" << std::endl;
		std::cout << "Test 2 ... " << std::flush;
		test2(&db);
		std::cout << "OK" << std::endl;
		std::cout << "Test 3 ... " << std::flush;
		test3(&db);
		std::cout << "OK" << std::endl;
		std::cout << "Test 4 ... " << std::flush;
		test4(&db);
		std::cout << "OK" << std::endl;
		std::cout << "Test 5 ... " << std::flush;
		test5(&db);
		std::cout << "OK" << std::endl;
//		std::cout << "Test 6 ... " << std::flush;
//		test6(&db);
//		std::cout << "OK" << std::endl;
	} catch (std::exception const & e) {
		std::cerr << "Exception: " << e.what() << std::endl;
		return 1;
	}
	return 0;
}

#endif

