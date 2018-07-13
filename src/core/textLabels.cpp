#include "textLabels.hpp"
#include <map>
#include <vector>

namespace {
//	std::vector<std::string> label2string(128,"__unknownField__");  // TODO - hardcoded size
	std::map<std::string,label> string2label;
	std::array<std::string,256> textLabels;
	bool textLabelInitialized = false;

#define LABEL_STRING_CONVERSION(dLabel) textLabels[static_cast<std::vector<std::string>::size_type>(label::dLabel)] = #dLabel; string2label[#dLabel] = label::dLabel;

	void initTextLabels() {
		textLabels[static_cast<unsigned>(label::_at_context)] = "@context"; string2label["@context"] = label::_at_context;
		textLabels[static_cast<unsigned>(label::_at_id     )] = "@id"     ; string2label["@id"     ] = label::_at_id     ;
		LABEL_STRING_CONVERSION(__unknownField__)
		LABEL_STRING_CONVERSION(active)
		LABEL_STRING_CONVERSION(actualAllele)
		LABEL_STRING_CONVERSION(allele)
		LABEL_STRING_CONVERSION(alleleId)
		LABEL_STRING_CONVERSION(AllelicEpigenome)
		LABEL_STRING_CONVERSION(aminoAcidAlleles)
		LABEL_STRING_CONVERSION(canonicalAlleles)
		LABEL_STRING_CONVERSION(CDS)
		LABEL_STRING_CONVERSION(chromosome)
		LABEL_STRING_CONVERSION(ClinVarAlleles)
		LABEL_STRING_CONVERSION(ClinVarVariations)
		LABEL_STRING_CONVERSION(coordinates)
		LABEL_STRING_CONVERSION(COSMIC)
		LABEL_STRING_CONVERSION(dbSNP)
		LABEL_STRING_CONVERSION(debug)
		LABEL_STRING_CONVERSION(description)
		LABEL_STRING_CONVERSION(end)
		LABEL_STRING_CONVERSION(endIntronDirection)
		LABEL_STRING_CONVERSION(endIntronOffset)
		LABEL_STRING_CONVERSION(Ensembl)
		LABEL_STRING_CONVERSION(errorType)
		LABEL_STRING_CONVERSION(ExAC)
		LABEL_STRING_CONVERSION(externalRecords)
		LABEL_STRING_CONVERSION(externalSources)
		LABEL_STRING_CONVERSION(GenBank)
		LABEL_STRING_CONVERSION(gene)
		LABEL_STRING_CONVERSION(geneNCBI_id)
		LABEL_STRING_CONVERSION(geneSymbol)
		LABEL_STRING_CONVERSION(genomicAlleles)
		LABEL_STRING_CONVERSION(givenAllele)
		LABEL_STRING_CONVERSION(gnomAD)
		LABEL_STRING_CONVERSION(HGNC)
		LABEL_STRING_CONVERSION(hgvs)
		LABEL_STRING_CONVERSION(hgvsMatchingTranscriptVariant)
		LABEL_STRING_CONVERSION(hgvsWellDefined)
		LABEL_STRING_CONVERSION(id)
		LABEL_STRING_CONVERSION(inputLine)
		LABEL_STRING_CONVERSION(length)
		LABEL_STRING_CONVERSION(LRG)
		LABEL_STRING_CONVERSION(message)
		LABEL_STRING_CONVERSION(MyVariantInfo_hg19)
		LABEL_STRING_CONVERSION(MyVariantInfo_hg38)
		LABEL_STRING_CONVERSION(name)
		LABEL_STRING_CONVERSION(names)
		LABEL_STRING_CONVERSION(NCBI)
		LABEL_STRING_CONVERSION(position)
		LABEL_STRING_CONVERSION(preferredName)
		LABEL_STRING_CONVERSION(proteinAlleles)
		LABEL_STRING_CONVERSION(proteinEffect)
		LABEL_STRING_CONVERSION(proteinHgvs)
		LABEL_STRING_CONVERSION(RCV)
		LABEL_STRING_CONVERSION(referenceAllele)
		LABEL_STRING_CONVERSION(referenceGenome)
		LABEL_STRING_CONVERSION(referenceSequence)
		LABEL_STRING_CONVERSION(region)
		LABEL_STRING_CONVERSION(rs)
		LABEL_STRING_CONVERSION(sequence)
		LABEL_STRING_CONVERSION(sequenceName)
		LABEL_STRING_CONVERSION(splicedSequence)
		LABEL_STRING_CONVERSION(start)
		LABEL_STRING_CONVERSION(startIntronDirection)
		LABEL_STRING_CONVERSION(startIntronOffset)
		LABEL_STRING_CONVERSION(symbol)
		LABEL_STRING_CONVERSION(transcriptAlleles)
		LABEL_STRING_CONVERSION(type)
		LABEL_STRING_CONVERSION(variant)
		LABEL_STRING_CONVERSION(variationId)
	}

}

label toLabel(std::string const & s)
{
	if (! textLabelInitialized) {
		initTextLabels();
		textLabelInitialized = true;
	}
	auto it = string2label.find(s);
	if (it == string2label.end()) return label::__unknownField__;//throw std::runtime_error("Unknown field: " + s);
	return it->second;
}
//std::string toString(label l)
//{
//	return label2string[static_cast<std::vector<std::string>::size_type>(l.value)];
//}

std::array<std::string,256> getTextLabels()
{
	if (! textLabelInitialized) {
		initTextLabels();
		textLabelInitialized = true;
	}
	return textLabels;
}

