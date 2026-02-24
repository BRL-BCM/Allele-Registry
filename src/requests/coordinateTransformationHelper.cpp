/*
Classes required to support coordinate transformation.
Author: Ronak Patel.
Philosophy/Structure of the class: 
- Classes perform coordinate transformation one at a time. All the wrapper that uses need to do is call the constructor with reference database and textInput.
- There are two key functions that are called in the constructor:
-- getProjectionsToGrch38: From the provided coordinates, this function projects those coordinates to GRCh38. It uses inputCoordinates of type Acse to project. If successful then stores the GRCh38 coordiantes. Otherwise marks the mappable to genome assembly false and returns inputCoordinates back in transformations.
-- getProjectionToOtherAssembly: If the projection to GRCH38 was successful, this function maps to other genome assemblies. 
*/
#include "requests.hpp"
#include "hgvs.hpp"
#include "canonicalization.hpp"
#include "coordinateTransformationHelper.hpp"
// Do not mess with the following constant variable, this is a key variable that is derived based on many tradeoffs.
// If the variant is less than this many base pairs, then no attemps will be made to transform coordinates.
// Increasing this limit may hang entire code, as it requires a hack of memory.
#define CAN_LIMIT 2000000

std::vector<std::string> grch38_refseqs = {"NC_000001.11", "NC_000002.12", "NC_000003.12", "NC_000004.12", "NC_000005.10", "NC_000006.12", "NC_000007.14", "NC_000008.11", "NC_000009.12", "NC_000010.11", "NC_000011.10", "NC_000012.12", "NC_000013.11", "NC_000014.9", "NC_000015.10", "NC_000016.10", "NC_000017.11", "NC_000018.10", "NC_000019.10", "NC_000020.11", "NC_000021.9", "NC_000022.11", "NC_000023.11", "NC_000024.10", "NC_012920.1"}; 
std::vector<std::string> grch37_refseqs = {"NC_000001.10", "NC_000002.11", "NC_000003.11", "NC_000004.11", "NC_000005.9", "NC_000006.11", "NC_000007.13", "NC_000008.10", "NC_000009.11", "NC_000010.10", "NC_000011.9", "NC_000012.11", "NC_000013.10", "NC_000014.8", "NC_000015.9", "NC_000016.9", "NC_000017.10", "NC_000018.9", "NC_000019.9", "NC_000020.10", "NC_000021.8", "NC_000022.10", "NC_000023.10", "NC_000024.9", "NC_001807.4"};
std::vector<std::string> ncbi36_refseqs = {"NC_000001.9", "NC_000002.10", "NC_000003.10", "NC_000004.10", "NC_000005.8", "NC_000006.10", "NC_000007.12", "NC_000008.9", "NC_000009.10", "NC_000010.9", "NC_000011.8", "NC_000012.10", "NC_000013.9", "NC_000014.7", "NC_000015.8", "NC_000016.8", "NC_000017.9", "NC_000018.8", "NC_000019.8", "NC_000020.9", "NC_000021.7", "NC_000022.9", "NC_000023.9", "NC_000024.8"};

Chromosome refseq2chr(std::string referenceSequenceName){
		if(     referenceSequenceName == "NC_000001.11" || referenceSequenceName == "NC_000001.10" || referenceSequenceName == "NC_000001.9") { return (Chromosome::chr1); }
		else if(referenceSequenceName == "NC_000002.12" || referenceSequenceName == "NC_000002.11" || referenceSequenceName == "NC_000002.10") { return (Chromosome::chr2); }
		else if(referenceSequenceName == "NC_000003.12" || referenceSequenceName == "NC_000003.11" || referenceSequenceName == "NC_000003.10") { return (Chromosome::chr3); }
		else if(referenceSequenceName == "NC_000004.12" || referenceSequenceName == "NC_000004.11" || referenceSequenceName == "NC_000004.10") { return (Chromosome::chr4); }
		else if(referenceSequenceName == "NC_000005.10" || referenceSequenceName == "NC_000005.9"  || referenceSequenceName == "NC_000005.8") { return (Chromosome::chr5); }
		else if(referenceSequenceName == "NC_000006.12" || referenceSequenceName == "NC_000006.11" || referenceSequenceName == "NC_000006.10") { return (Chromosome::chr6); }
		else if(referenceSequenceName == "NC_000007.14" || referenceSequenceName == "NC_000007.13" || referenceSequenceName == "NC_000007.12") { return (Chromosome::chr7); }
		else if(referenceSequenceName == "NC_000008.11" || referenceSequenceName == "NC_000008.10" || referenceSequenceName == "NC_000008.9") { return (Chromosome::chr8); }
		else if(referenceSequenceName == "NC_000009.12" || referenceSequenceName == "NC_000009.11" || referenceSequenceName == "NC_000009.10") { return (Chromosome::chr9); }
		else if(referenceSequenceName == "NC_000010.11" || referenceSequenceName == "NC_000010.10" || referenceSequenceName == "NC_000010.9") { return (Chromosome::chr10);}
		else if(referenceSequenceName == "NC_000011.10" || referenceSequenceName == "NC_000011.9"  || referenceSequenceName == "NC_000011.8") { return (Chromosome::chr11);}
		else if(referenceSequenceName == "NC_000012.12" || referenceSequenceName == "NC_000012.11" || referenceSequenceName == "NC_000012.10") { return (Chromosome::chr12);}
		else if(referenceSequenceName == "NC_000013.11" || referenceSequenceName == "NC_000013.10" || referenceSequenceName == "NC_000013.9") { return (Chromosome::chr13);}
		else if(referenceSequenceName == "NC_000014.9"  || referenceSequenceName == "NC_000014.8"  || referenceSequenceName == "NC_000014.7" ) { return (Chromosome::chr14);}
		else if(referenceSequenceName == "NC_000015.10" || referenceSequenceName == "NC_000015.9"  || referenceSequenceName == "NC_000015.8") { return (Chromosome::chr15);}
		else if(referenceSequenceName == "NC_000016.10" || referenceSequenceName == "NC_000016.9"  || referenceSequenceName == "NC_000016.8") { return (Chromosome::chr16);}
		else if(referenceSequenceName == "NC_000017.11" || referenceSequenceName == "NC_000017.10" || referenceSequenceName == "NC_000017.9") { return (Chromosome::chr17);}
		else if(referenceSequenceName == "NC_000018.10" || referenceSequenceName == "NC_000018.9"  || referenceSequenceName == "NC_000018.8") { return (Chromosome::chr18);}
		else if(referenceSequenceName == "NC_000019.10" || referenceSequenceName == "NC_000019.9"  || referenceSequenceName == "NC_000019.8") { return (Chromosome::chr19);}
		else if(referenceSequenceName == "NC_000020.11" || referenceSequenceName == "NC_000020.10" || referenceSequenceName == "NC_000020.9") { return (Chromosome::chr20);}
		else if(referenceSequenceName == "NC_000021.9"  || referenceSequenceName == "NC_000021.8"  || referenceSequenceName == "NC_000021.7" ) { return (Chromosome::chr21);}
		else if(referenceSequenceName == "NC_000022.11" || referenceSequenceName == "NC_000022.10" || referenceSequenceName == "NC_000022.9") { return (Chromosome::chr22);}
		else if(referenceSequenceName == "NC_000023.11" || referenceSequenceName == "NC_000023.10" || referenceSequenceName == "NC_000023.9") { return (Chromosome::chrX); }
		else if(referenceSequenceName == "NC_000024.10" || referenceSequenceName == "NC_000024.9"  || referenceSequenceName == "NC_000024.8") { return (Chromosome::chrY); }
		else if(referenceSequenceName == "NC_012920.1"  || referenceSequenceName == "NC_001807.4" ) { return (Chromosome::chrM); }
		else throw std::runtime_error(("An error occured while converting the reference sequence name to chromosome. Provided reference sequence => "+referenceSequenceName));
	}

ReferenceGenome refseq2refGenome(std::string & referenceSequenceName){
	for(auto & refseq: grch38_refseqs){
		if(refseq == referenceSequenceName){
			return(ReferenceGenome::rgGRCh38);
		}
	}

	for(auto & refseq: grch37_refseqs){
		if(refseq == referenceSequenceName){
			return(ReferenceGenome::rgGRCh37);
		}
	}	

	for(auto & refseq: ncbi36_refseqs){
		if(refseq == referenceSequenceName){
			return(ReferenceGenome::rgNCBI36);
		}
	}
	throw std::runtime_error("Unable to map reference sequence to genome assembly. Likely the reference sequence is not part of GRCh37, GRCh38 or NCBI36 assembly.");
}

genomicCoordinateTransformer::genomicCoordinateTransformer(ReferencesDatabase const * seqDb, textInput const & text):seqDb(seqDb)
{
	// If degenerate range is provided, let user know that it is not supported.
	if(text.cstart == text.cend){
		throw std::runtime_error("The start and end coordinates can not be the same. This is a limitation of current approach where we assume nothing about the type of coordinates (zero or one based) provided through paramers.");
	}

	try {
		RegionCoordinates region(text.cstart, text.cend);
		inputCoordinates = {parseReferenceGenome(text.assembly), parseChromosome(text.chr), region};
	}
	catch(...){
		throw std::runtime_error("Problem with provided input");
	}
	try {
		getProjectionsToGrch38();
		if(mappableToMainAssembly){
			try{
				getProjectionsToOtherAssembly();
			} catch(...){
				throw std::runtime_error("Error occured while projecting coordinates to other assemblies");
			}			
		}
	} catch(...){
		throw std::runtime_error( ( "Error occured while projecting to main genome assembly => "
			+ toString(inputCoordinates.assembly)  + "-"
			+ toString(inputCoordinates.chr)  + "-"
			+ toString(inputCoordinates.region.left()) + "-"
			+ toString(inputCoordinates.region.right())
			 )
		);
	}
};

genomicCoordinateTransformer::~genomicCoordinateTransformer(){
};


bool isGoodAlignment(GeneralSeqAlignment & inAli){

	bool size_check = (inAli.elements.size() == 1);

	bool good_match = true;
	// Alignment must not include major mismatches
	for (uint i = 0; i < inAli.elements.front().alignment.cigarString().size(); i++){
		char tc = inAli.elements.front().alignment.cigarString()[i];
		if(tc == 'D' || tc == 'I' || tc == ' ' || tc == 'N' || tc == 'H' || tc == 'P' || tc == 'S'){
			good_match = false;
			break;
		}
	}

  // In rare scenario if the reference sequence in alignment is not mapped to any identifier, the alignment gives back UINT_MAX
  // To provide atlease input coordinates back rather than breaking up this check is done.
	bool reference_number_check = false;
	if(inAli.elements.front().alignment.sourceRefId.value < UINT_MAX){
		reference_number_check = true;
	}

	if(size_check && good_match && reference_number_check)
		return true;
	else 
		return false;
}

bool isGenomicRefSeq(std::string const & rs){
	if(rs[0] == 'N' && rs[1] == 'C' && rs[2] == '_')
		return true;
	else
		return false;
}

bool isGenomicAli(std::vector<std::string> & v){
	return isGenomicRefSeq(v.front());
}

void genomicCoordinateTransformer::getProjectionsToGrch38(){
	if(inputCoordinates.region.length() < CAN_LIMIT){
		ReferenceId rid = seqDb->getReferenceId(inputCoordinates.assembly, inputCoordinates.chr);
		GeneralSeqAlignment toMainGenome = seqDb->getAlignmentFromMainGenome(rid, inputCoordinates.region);
		if(isGoodAlignment(toMainGenome)){
			std::string refseq = seqDb->getNames(toMainGenome.elements.front().alignment.sourceRefId).front();
			// The genomic sequence can include NT_, to handle those scenario please check if the input sequence contain NC_
			if(isGenomicRefSeq(refseq)){
					grch38Coordinates.assembly = ReferenceGenome::rgGRCh38;
					grch38Coordinates.chr = refseq2chr(refseq);
					RegionCoordinates region(
						toMainGenome.elements.front().alignment.sourceRegion().left(),
						toMainGenome.elements.front().alignment.sourceRegion().right()
						);
					grch38Coordinates.region = region;
					transformation.push_back(grch38Coordinates);
					mappableToMainAssembly = true;
				}
			else {
				transformation.push_back(inputCoordinates);
				mappableToMainAssembly = false;	
			}		
		} else	{
			transformation.push_back(inputCoordinates);
			mappableToMainAssembly = false;
		}
	}
	else {
		transformation.push_back(inputCoordinates);
		mappableToMainAssembly = false ;
	}
}

void genomicCoordinateTransformer::getProjectionsToOtherAssembly(){
	if(mappableToMainAssembly){
		ReferenceId rid = seqDb->getReferenceId(grch38Coordinates.assembly, grch38Coordinates.chr);
		std::vector<GeneralSeqAlignment> alignments = seqDb->getAlignmentsToMainGenome(rid, grch38Coordinates.region);
		for(auto & ali: alignments){
			auto v = seqDb->getNames(ali.elements.front().alignment.sourceRefId);
			if(isGoodAlignment(ali) && isGenomicAli(v)){
				std::string refseq = seqDb->getNames(ali.elements.front().alignment.sourceRefId).front();
				RegionCoordinates region(
					ali.elements.front().alignment.sourceRegion().left(),
					ali.elements.front().alignment.sourceRegion().right()
				);
				transformation.push_back({refseq2refGenome(refseq), refseq2chr(refseq), region});									
			}
		}
	}
}
