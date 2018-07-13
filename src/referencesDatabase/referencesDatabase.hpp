#ifndef GENOMEINDEX_HPP_
#define GENOMEINDEX_HPP_

#include <string>
#include <vector>
#include <map>

#include "../core/globals.hpp"
#include "alignment.hpp"
#include "stringPointer.hpp"

/*
 * Files with main genome & proteins - fasta files
 *
 * Files with other genomes & genomic regions
 * header -> name of ref
 * unalignedSequence + mainRefId + mainRefStrand + mainRefOffset + alignment
 * (alignment: like CIGAR string M-match I-insertion D-deletion X-SNV)
 *
 * Files with genomic regions - the same as above
 *
 * File with transcripts - the same as above
 *
 * File with proteins:
 * one file - name + transcript ID + CDS start
 *
 * File with names:
 * all names representing the same sequence in one row
 *
 */

struct Gene {
	bool active = false;
	unsigned hgncId = 0;
	unsigned refSeqId = 0;
	std::string hgncName;
	std::string hgncSymbol;
	std::string ensemblId;
	std::vector<std::string> otherSymbols;
	std::vector<std::string> obsoleteSymbols;
	std::vector<ReferenceId> assignedReferences;
};

struct ReferenceMetadata
{
	RegionCoordinates CDS; // in spliced coordinates
	unsigned length = 0;
	unsigned splicedLength = 0;
	unsigned geneId = 0;
	unsigned proteinId = 0;
	std::string genomeBuild = "";
	Chromosome chromosome = chrUnknown;
	unsigned frameOffset = 0;
	uint64_t proteinAccessionIdentifier = 0;
};

class ReferencesDatabase
{
private:
	struct Pim;
	Pim * pim;
	unsigned hgncSymbolToGeneId(std::string const & hgncGeneSymbol) const;
public:
	// Creates the object and load data from path
	ReferencesDatabase(std::string const & path);
	// destructor
	~ReferencesDatabase();
	// Get sequence of any refSeq, for spliced refSeq introns are included (parameters are always unspliced coordinates)
	std::string getSequence(ReferenceId refId, RegionCoordinates region) const;
	std::string getSplicedSequence(ReferenceId refId, RegionCoordinates region) const;
	// Get alignment for mapped refSeq
	GeneralSeqAlignment  getAlignmentFromMainGenome(ReferenceId refId, RegionCoordinates region) const;
	// Get alignments
	std::vector<GeneralSeqAlignment> getAlignmentsToMainGenome(ReferenceId refId, RegionCoordinates region) const;
	// convert spliced coordinates <-> unspliced coordinates
	RegionCoordinates convertToUnsplicedRegion(ReferenceId refId, SplicedRegionCoordinates const & region) const;
	SplicedRegionCoordinates convertToSplicedRegion(ReferenceId refId, RegionCoordinates const & region) const;
    // get exons
    std::vector<std::pair<RegionCoordinates,SplicedRegionCoordinates>> getExons(ReferenceId refId) const;
	// Check if given refSeq is spliced refSeq
	bool isSplicedRefSeq(ReferenceId refId) const;
	bool isProteinReference(ReferenceId refId) const;
	// Returns refSeq ID associated with given name or null if name is unknown
	ReferenceId getReferenceId(std::string const & name) const;
	// Returns ID associated with given chromosome
	ReferenceId getReferenceId(ReferenceGenome, Chromosome) const;
	unsigned getCarRsId(ReferenceId refId) const;
	// Returns names associated with this refSeq
	std::vector<std::string> getNames(ReferenceId refId) const;
	std::vector<ReferenceId> getReferencesByName(std::string const & name) const;
	std::vector<ReferenceId> getReferencesByGene(std::string const & hgncGeneSymbol) const;
	std::vector<ReferenceId> getReferencesByGene(unsigned id) const;
	// Returns CDS associated with given refId (it must be transcript)
	RegionCoordinates getCDS(ReferenceId refId) const;
	// Return sequence length (unspliced)
	unsigned getSequenceLength(ReferenceId refId) const;
	// Return metadata
	ReferenceMetadata const & getMetadata(ReferenceId refId) const;
	// Return gene region
	std::map<ReferenceId,std::vector<RegionCoordinates>> getGeneRegions(std::string const & hgncGeneSymbol) const;
	// Return lengths of sequences from the main genome
	std::vector<unsigned> getMainGenomeReferencesLengths() const; // return [refId]->length
	Gene getGeneById(unsigned id) const;
	std::vector<Gene> getGenesByName(std::string const & name) const;
	std::vector<Gene> const & getGenes() const;
	ReferenceId getTranscriptForProtein(ReferenceId proteinId) const;
	ReferenceId getReferenceIdFromProteinAccessionIdentifier(uint64_t proteinAccessionIdentifier) const;
	// Return references
//	std::vector<ReferenceMetadata> const & getReferencesMetadata() const;
};


#endif /* GENOMEINDEX_HPP_ */
