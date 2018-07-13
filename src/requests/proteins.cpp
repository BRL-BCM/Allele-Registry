#include "../core/variants.hpp"
#include "../referencesDatabase/referencesDatabase.hpp"
#include "canonicalization.hpp"
#include "proteinTools.hpp"

inline unsigned abs(std::string::size_type a, std::string::size_type b)
{
	if (a > b) return (a-b);
	return (b-a);
}

// from hgvs
std::string toHgvsProteinPosition( char aa, unsigned position );
std::string toHgvsModification( ReferencesDatabase const* refDb, ReferenceId const refId, PlainSequenceModification const & p );

// returns true <=> stop codon was reached on modified sequence
// cluster - sequence of variants INSIDE protein with total frameshift == 0 (frameshift inside cluster is != 0)
inline bool convertToProteinVariantsBeforeTer
	( ReferencesDatabase const * refDb
	, ReferenceId const refId
	, std::vector<PlainSequenceModification> const & cluster           // variants on transcript - spliced
	, std::vector<std::string>                     & outPartialHgvses  // output is added here
	, std::vector<PlainSequenceModification>       & outVarsInProtein  // output is added here
	)
{
	RegionCoordinates region( cluster.front().region.left(), cluster.back().region.right() );
	unsigned const CDSstart = refDb->getCDS(refId).left();
	region.decLeftPosition( (region.left()-CDSstart ) % 3 );
	region.incRightPosition( (3-((region.right()-CDSstart)%3)) % 3 );
	std::string seq = refDb->getSplicedSequence(refId, region);
	std::string seq2 = applyVariantsToTheSequence(seq, region.left(), cluster);
	ReferenceId const proteinId(refDb->getMetadata(refId).proteinId);

	// convert to proteins
	seq  = translateToAminoAcid(seq );
	seq2 = translateToAminoAcid(seq2);

	unsigned aaOffset = (region.left() - CDSstart) / 3;  // offset in aa of seq/seq2
	unsigned minSize = std::min(seq.size(), seq2.size());

	// find the first difference
	unsigned aaFirstDiff;
	for ( aaFirstDiff = 0;  aaFirstDiff < minSize;  ++aaFirstDiff ) {
		if (seq[aaFirstDiff] != seq2[aaFirstDiff]) break;
	}
	if (aaFirstDiff == minSize && seq.size() == seq2.size()) {
		// no changes
		outPartialHgvses.push_back(toHgvsProteinPosition( seq.front(), aaOffset+1 ) + "=");
		return false;
	}

	// check if there is a Ter codon in seq2
	bool terSpotted = false;
	unsigned terOffset = 0;
	for ( unsigned i = aaFirstDiff; i < seq2.size(); ++i )
		if (seq2[i] == '*') {
			terSpotted = true;
			terOffset = i - aaFirstDiff;
			break;
		}

	if (terSpotted) {
		std::string hgvs = toHgvsProteinPosition( seq.front(), aaOffset + aaFirstDiff + 1 );
		hgvs += convertOneLetterAminoAcidSequenceToThreeLetters( seq2.substr(aaFirstDiff,1) );
		if (terOffset > 0) {
			hgvs += "fsTer" + boost::lexical_cast<std::string>(terOffset+1);
		}
		outPartialHgvses.push_back( hgvs );
		// build protein change
		unsigned const proteinLength = refDb->getMetadata(proteinId).length;
		PlainSequenceModification psm;
		psm.region.set( aaOffset + aaFirstDiff, proteinLength );
		psm.newSequence = seq2.substr(aaFirstDiff, terOffset);
		psm.originalSequence = refDb->getSequence( proteinId, psm.region );
		outVarsInProtein.push_back(psm);
		return true;
	}

	// cut the matching tail
	while ( aaFirstDiff < minSize && seq.back() == seq2.back() ) {
		seq.pop_back();
		seq2.pop_back();
		--minSize;
	}

	PlainSequenceModification psm;
	psm.region.set( aaOffset + aaFirstDiff, aaOffset + seq.size() );
	psm.newSequence = seq2.substr(aaFirstDiff);
	psm.originalSequence = seq.substr(aaFirstDiff);
	outVarsInProtein.push_back(psm);
	outPartialHgvses.push_back( toHgvsModification(refDb, proteinId, psm) );

	return false;
}

// Ter codon is in original sequence
inline void convertToProteinVariantWithTer
	( ReferencesDatabase const * refDb
	, ReferenceId const refId
	, std::vector<PlainSequenceModification> const & cluster           // variants on transcript - spliced
	, std::vector<std::string>                     & outPartialHgvses  // output is added here
	, std::vector<PlainSequenceModification>       & outVarsInProtein  // output is added here
	)
{
	RegionCoordinates region( cluster.front().region.left(), cluster.back().region.right() );
	unsigned const CDSstart  = refDb->getCDS(refId).left();
	unsigned const CDSlength = refDb->getCDS(refId).length();
	region.decLeftPosition( (region.left()-CDSstart ) % 3 );
	region.setRight( std::min(refDb->getMetadata(refId).splicedLength, region.right() + 3*30) );   // TODO - workaround for efficiency - max 30 aa are checked

	std::string seq = refDb->getSplicedSequence(refId, region);
	std::string seq2 = applyVariantsToTheSequence(seq, region.left(), cluster);

	seq = seq.substr( 0, CDSlength - (region.left()-CDSstart) ); // cut stuff behind CDS

	// convert to aa
	seq  = translateToAminoAcid(seq );
	seq2 = translateToAminoAcid(seq2);

	unsigned aaOffset = (region.left() - CDSstart) / 3;
	unsigned minSize = std::min(seq.size(), seq2.size());
	// find the first difference
	unsigned aaFirstDiff;
	for ( aaFirstDiff = 0;  aaFirstDiff < minSize;  ++aaFirstDiff ) {
		if (seq[aaFirstDiff] != seq2[aaFirstDiff]) break;
	}
	if ( aaFirstDiff == minSize ) {
		// no changes - the sequence with the termination codon was the same
		outPartialHgvses.push_back(toHgvsProteinPosition( seq.front(), aaOffset + 1 ) + "=");
		return;
	}

	// check if there is a Ter codon in seq2
	bool terSpotted = false;
	unsigned terOffset = 0;
	for ( unsigned i = aaFirstDiff; i < seq2.size(); ++i )
		if (seq2[i] == '*') {
			terSpotted = true;
			terOffset = i - aaFirstDiff;
			break;
		}

	// calculate HGVS modification
	std::string hgvs = toHgvsProteinPosition( seq[aaFirstDiff], aaOffset + aaFirstDiff + 1 );
	hgvs += convertOneLetterAminoAcidSequenceToThreeLetters(seq2.substr(aaFirstDiff,1));
	if (seq2[aaFirstDiff] != '*') {
		hgvs += ( seq[aaFirstDiff] == '*' ) ? "extTer" : "fsTer";
		if (terSpotted) {
			hgvs += boost::lexical_cast<std::string>(terOffset + 1);
		} else {
			hgvs += "?";
		}
	}
	outPartialHgvses.push_back(hgvs);

	// calculate variant def
	if ( (! seq.empty()) && seq.back() == '*' ) seq.pop_back(); // remove Ter codon if exists
	PlainSequenceModification psm;
	psm.region.set( aaOffset + aaFirstDiff, aaOffset + seq.size() );
	psm.newSequence = seq2.substr(aaFirstDiff);
	if (terSpotted) psm.newSequence.resize(terOffset);
	psm.originalSequence = seq;
	outVarsInProtein.push_back(psm);
}


// true if can be represented as proteinVar, false otherwise
bool calculateProteinVariation
    ( ReferencesDatabase const * refDb
    , NormalizedGenomicVariant const & transcriptVar
    , std::string & outGeneralHgvs
    , std::string & outCanonicalHgvs
    )
{
	outGeneralHgvs = outCanonicalHgvs = "";
	ReferenceMetadata transMetadata = refDb->getMetadata(transcriptVar.refId);
	if (transMetadata.proteinId == 0 || transMetadata.frameOffset > 0) return false;

	try {
		// ====== translate variant to spliced coordinates, filter out intron variants
		std::vector<NormalizedSequenceModification> varsInSplicedRegions; // in spliced coordinates
		{
			std::vector<std::pair<RegionCoordinates,SplicedRegionCoordinates>> exons = refDb->getExons(transcriptVar.refId);
			auto itExon = exons.begin();
			for ( auto & var: transcriptVar.modifications) {
				while ( itExon != exons.end() && itExon->first.right() <= var.region.left()) ++itExon;
				if (itExon == exons.end() || var.region.right() <= itExon->first.left()) continue;
				if (var.region.left() < itExon->first.left() || var.region.right() > itExon->first.right()) {
					// variant region crosses exon-intron boundary
					return false;
				}
				NormalizedSequenceModification sm = var;
				sm.region = refDb->convertToSplicedRegion(transcriptVar.refId, sm.region).toRegion();
				varsInSplicedRegions.push_back(sm);
			}
		}

		// ====== remove variants before CDS
		std::vector<PlainSequenceModification> varsInCDS; // in spliced coordinates
		for (auto const & var: varsInSplicedRegions) {
			PlainSequenceModification ra = var.rightAligned();
			if (ra.region.left() >= transMetadata.CDS.left() + 3) {
				// after START CODON
				varsInCDS.push_back(ra);
			} else if (var.leftAligned().region.right() <= transMetadata.CDS.left()) {
				// before START CODON - TODO: what if it creates new start codon ?
				continue;
			} else {
				// it disturbs START CODON - TODO
				return false;
			}
		}

		// ====== check if there are any changes inside CDS
		std::string const proteinName = refDb->getNames(ReferenceId(transMetadata.proteinId)).front();
		if ( varsInCDS.empty() || varsInCDS.front().region.left() >= transMetadata.CDS.right()+3 ) {
			// no changes
			outGeneralHgvs = proteinName + ":p.=";
			return false;
		}

		// ====== convert to protein variations
		// results
		std::vector<std::string> partialHgvses;
		std::vector<PlainSequenceModification> varsInProtein;
		// temporary container to hold local clusters (cluster: minimal sequence of variants with total frameshift = 0)
		std::vector<PlainSequenceModification> currentVarsCluster;
		// total frameshift for the cluster
		unsigned currentFrameshift = 0;
		// ---- calculactions
		for ( auto itV = varsInCDS.begin();  itV != varsInCDS.end();  ++itV ) {
			// if we passed stop codon ...
			if ( itV->region.left() >= transMetadata.CDS.right()+3 ) {
				// ... and we are inside a cluster, it must be extended by adding the rest of variants.
				// Otherwise, we are done and the rest of variants can be ignored
				if ( ! currentVarsCluster.empty()) {
					currentVarsCluster.insert( currentVarsCluster.end(), itV, varsInCDS.end() );
				}
				break;
			}
			// if the stop codon was touched ...
			if ( itV->region.right() > transMetadata.CDS.right() ) {
				// ...we have to extend the cluster by adding the rest of variants.
				currentVarsCluster.insert( currentVarsCluster.end(), itV, varsInCDS.end() );
				break;
			}
			// --- so here, we are sure that the stop codon is not reached yet
			// update cluster & frameshift
			currentVarsCluster.push_back(*itV);
			currentFrameshift = ( currentFrameshift + abs(itV->newSequence.size(),itV->originalSequence.size()) ) % 3;
			// if there is no frameshift, the cluster can be processed, otherwise we go to the next variant
			if (currentFrameshift == 0) {
				bool const stopCodon = convertToProteinVariantsBeforeTer( refDb, transcriptVar.refId, currentVarsCluster, partialHgvses, varsInProtein );
				currentVarsCluster.clear();
				// if stop codon was reached, we are done
				if (stopCodon) break;
			}
		}
		if ( ! currentVarsCluster.empty() ) convertToProteinVariantWithTer( refDb, transcriptVar.refId, currentVarsCluster, partialHgvses, varsInProtein );

		// ====== calculate hgvs
		outGeneralHgvs = proteinName + ":p.";
		if (partialHgvses.size() == 1) {
			outGeneralHgvs += partialHgvses.front();
		} else {
			ASSERT(partialHgvses.size() > 1);
			outGeneralHgvs += "[" + partialHgvses.front();
			for (unsigned i = 1; i < partialHgvses.size(); ++i) outGeneralHgvs += ";" + partialHgvses[i];
			outGeneralHgvs += "]";
		}

		// ====== calculate canonicalized variant
		if (varsInProtein.empty()) return false;
		PlainVariant protVar;
		protVar.refId.value = transMetadata.proteinId;
		protVar.modifications = varsInProtein;
for (auto & m: protVar.modifications) m.originalSequence = ""; // TODO - it is incorrect sometimes and cause validation errors
		NormalizedProteinVariant proteinVar = canonicalizeProtein(refDb, protVar); // it may throw exception if too long
		outCanonicalHgvs = proteinName + ":" + toHgvsModifications( refDb, proteinVar );

	} catch (std::exception const & e) {
		return false; // TODO - log or something
	}
	return true;
}

