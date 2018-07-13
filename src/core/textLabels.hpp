#ifndef CORE_TEXTLABELS_HPP_
#define CORE_TEXTLABELS_HPP_

#include <string>
#include <array>

struct label {
	enum : uint8_t
	{
		  __unknownField__ = 1  // zero not allowed !
		, _at_context
		, _at_id
		, AllelicEpigenome
		, CDS
		, COSMIC
		, ClinVarAlleles
		, ClinVarVariations
		, ExAC
		, Ensembl
		, GenBank
		, HGNC
		, LRG
		, MyVariantInfo_hg19
		, MyVariantInfo_hg38
		, NCBI
		, RCV
		, active
		, actualAllele
		, allele
		, alleleId
		, aminoAcidAlleles
		, canonicalAlleles
		, chromosome
		, coordinates
		, dbSNP
		, debug
		, description
		, end
		, endIntronDirection
		, endIntronOffset
		, errorType
		, externalRecords
		, externalSources
		, gene
		, geneNCBI_id
		, geneSymbol
		, genomicAlleles
		, givenAllele
		, gnomAD
		, hgvs
		, hgvsMatchingTranscriptVariant
		, hgvsWellDefined
		, id
		, inputLine
		, length
		, message
		, name
		, names
		, position
		, preferredName
		, proteinAlleles
		, proteinEffect
		, proteinHgvs
		, referenceAllele
		, referenceGenome
		, referenceSequence
		, region
		, rs
		, sequence
		, sequenceName
		, splicedSequence
		, start
		, startIntronDirection
		, startIntronOffset
		, symbol
		, transcriptAlleles
		, type
		, variant
		, variationId
	};
	uint8_t value;
	inline label(uint8_t v = label::__unknownField__) : value(v) {}
	inline bool operator< (label const l) const { return (this->value < l.value); }
	inline bool operator!=(label const l) const { return (this->value != l.value); }
	inline operator uint64_t() const { return static_cast<uint64_t>(value); }
};

label toLabel(std::string const & s);

//std::string toString(label l);

//void initTextLabels();
//extern std::array<std::string,256> textLabels;

std::array<std::string,256> getTextLabels();


#endif /* CORE_TEXTLABELS_HPP_ */
