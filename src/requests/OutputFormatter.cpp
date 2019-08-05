#include "OutputFormatter.hpp"
#include "identifiersTools.hpp"
#include "../commonTools/JsonBuilder.hpp"
#include "../commonTools/DocumentSettings.hpp"
#include "../core/textLabels.hpp"
#include "hgvs.hpp"
#include <boost/algorithm/string.hpp>
#include <iomanip>


struct OutputFormatter::Pim
{
	// It contains document structure with boolean values.
	// Fields with 'true' must be included in the output, these ones with 'false' must be omitted.
	jsonBuilder::DocumentStructure<label> docSchema;
	jsonBuilder::DocumentSettings extSrcConf;
	responseFormat format;
	std::string carURI;
	std::string getURI(std::string const & prefix, unsigned id)
	{
		std::string number = boost::lexical_cast<std::string>(id);
		while (number.size() < 6) number = "0" + number;
		return ("http://" + carURI + "/" + prefix + number);
	}
	// =====================================================
	template<class tObject>
	inline void objects(jsonBuilder::NodeOfJsonTree<label> const & node, std::vector<tObject> const & objects);
	inline void object(jsonBuilder::NodeOfJsonTree<label> const & node, VariantDetailsGenomeBuild const & v);
	inline void object(jsonBuilder::NodeOfJsonTree<label> const & node, VariantDetailsGeneRegion const & v);
	inline void object(jsonBuilder::NodeOfJsonTree<label> const & node, VariantDetailsTranscript const & v);
	inline void object(jsonBuilder::NodeOfJsonTree<label> const & node, VariantDetailsProtein const & v);
	template<class tVariantDetails>
	inline void variantCommon(jsonBuilder::NodeOfJsonTree<label> const & node, tVariantDetails const & v);
};


ReferencesDatabase const * OutputFormatter::refDb = nullptr;
std::string OutputFormatter::carURI = "";

OutputFormatter::OutputFormatter(documentType docType) : pim(new Pim)
{
	pim->carURI = carURI;

	if (docType == documentType::activeGenomicVariant) {
		jsonBuilder::DocumentStructure<label> & docActiveGenomicVariant = pim->docSchema;
		docActiveGenomicVariant[label::_at_context].addNode();
		docActiveGenomicVariant[label::_at_id     ].addNode();
		docActiveGenomicVariant[label::type].addNode();
		docActiveGenomicVariant[label::externalRecords][label::dbSNP][label::_at_id].addNode();
		docActiveGenomicVariant[label::externalRecords][label::dbSNP][label::rs].addNode();
		docActiveGenomicVariant[label::externalRecords][label::ClinVarAlleles][label::_at_id].addNode();
		docActiveGenomicVariant[label::externalRecords][label::ClinVarAlleles][label::alleleId].addNode();
		docActiveGenomicVariant[label::externalRecords][label::ClinVarAlleles][label::preferredName].addNode();
		docActiveGenomicVariant[label::externalRecords][label::ClinVarVariations][label::_at_id].addNode();
		docActiveGenomicVariant[label::externalRecords][label::ClinVarVariations][label::variationId].addNode();
		docActiveGenomicVariant[label::externalRecords][label::ClinVarVariations][label::RCV].addNode();
		docActiveGenomicVariant[label::externalRecords][label::MyVariantInfo_hg19][label::_at_id].addNode();
		docActiveGenomicVariant[label::externalRecords][label::MyVariantInfo_hg19][label::id].addNode();
		docActiveGenomicVariant[label::externalRecords][label::MyVariantInfo_hg38][label::_at_id].addNode();
		docActiveGenomicVariant[label::externalRecords][label::MyVariantInfo_hg38][label::id].addNode();
		docActiveGenomicVariant[label::externalRecords][label::ExAC][label::_at_id].addNode();
		docActiveGenomicVariant[label::externalRecords][label::ExAC][label::id].addNode();
		docActiveGenomicVariant[label::externalRecords][label::ExAC][label::variant].addNode();
		docActiveGenomicVariant[label::externalRecords][label::gnomAD][label::_at_id].addNode();
		docActiveGenomicVariant[label::externalRecords][label::gnomAD][label::id].addNode();
		docActiveGenomicVariant[label::externalRecords][label::gnomAD][label::variant].addNode();
		docActiveGenomicVariant[label::externalRecords][label::AllelicEpigenome][label::_at_id].addNode();
		docActiveGenomicVariant[label::externalRecords][label::COSMIC][label::_at_id].addNode();
		docActiveGenomicVariant[label::externalRecords][label::COSMIC][label::id].addNode();
		docActiveGenomicVariant[label::externalRecords][label::COSMIC][label::active].addNode();
		docActiveGenomicVariant[label::externalSources].addNode();
		docActiveGenomicVariant[label::genomicAlleles][label::hgvs].addNode();
		docActiveGenomicVariant[label::genomicAlleles][label::referenceSequence].addNode();
		docActiveGenomicVariant[label::genomicAlleles][label::gene].addNode();
		docActiveGenomicVariant[label::genomicAlleles][label::coordinates][label::start].addNode();
		docActiveGenomicVariant[label::genomicAlleles][label::coordinates][label::end].addNode();
		docActiveGenomicVariant[label::genomicAlleles][label::coordinates][label::referenceAllele].addNode();
		docActiveGenomicVariant[label::genomicAlleles][label::coordinates][label::allele].addNode();
		docActiveGenomicVariant[label::genomicAlleles][label::referenceGenome].addNode();
		docActiveGenomicVariant[label::genomicAlleles][label::chromosome].addNode();
		docActiveGenomicVariant[label::transcriptAlleles][label::hgvs].addNode();
		docActiveGenomicVariant[label::transcriptAlleles][label::referenceSequence].addNode();
		docActiveGenomicVariant[label::transcriptAlleles][label::coordinates][label::start].addNode();
		docActiveGenomicVariant[label::transcriptAlleles][label::coordinates][label::end].addNode();
		docActiveGenomicVariant[label::transcriptAlleles][label::coordinates][label::startIntronOffset].addNode();
		docActiveGenomicVariant[label::transcriptAlleles][label::coordinates][label::startIntronDirection].addNode();
		docActiveGenomicVariant[label::transcriptAlleles][label::coordinates][label::endIntronOffset].addNode();
		docActiveGenomicVariant[label::transcriptAlleles][label::coordinates][label::endIntronDirection].addNode();
		docActiveGenomicVariant[label::transcriptAlleles][label::coordinates][label::referenceAllele].addNode();
		docActiveGenomicVariant[label::transcriptAlleles][label::coordinates][label::allele].addNode();
		docActiveGenomicVariant[label::transcriptAlleles][label::proteinHgvs].addNode();
		docActiveGenomicVariant[label::transcriptAlleles][label::proteinEffect][label::hgvs].addNode();
		docActiveGenomicVariant[label::transcriptAlleles][label::proteinEffect][label::hgvsWellDefined].addNode();
		docActiveGenomicVariant[label::transcriptAlleles][label::gene].addNode();
		docActiveGenomicVariant[label::transcriptAlleles][label::geneSymbol].addNode();
		docActiveGenomicVariant[label::transcriptAlleles][label::geneNCBI_id].addNode();
		docActiveGenomicVariant[label::aminoAcidAlleles][label::hgvs].addNode();
		docActiveGenomicVariant[label::aminoAcidAlleles][label::referenceSequence].addNode();
		docActiveGenomicVariant[label::aminoAcidAlleles][label::gene].addNode();
		docActiveGenomicVariant[label::aminoAcidAlleles][label::coordinates][label::start].addNode();
		docActiveGenomicVariant[label::aminoAcidAlleles][label::coordinates][label::end].addNode();
		docActiveGenomicVariant[label::aminoAcidAlleles][label::coordinates][label::referenceAllele].addNode();
		docActiveGenomicVariant[label::aminoAcidAlleles][label::coordinates][label::allele].addNode();
		docActiveGenomicVariant[label::aminoAcidAlleles][label::hgvsMatchingTranscriptVariant].addNode();
	} else if (docType == documentType::reference) {
		jsonBuilder::DocumentStructure<label> & docReference = pim->docSchema;
		docReference[label::_at_context].addNode();
		docReference[label::_at_id     ].addNode();
		docReference[label::type].addNode();
		docReference[label::gene].addNode();
		docReference[label::referenceGenome].addNode();
		docReference[label::chromosome].addNode();
		docReference[label::externalRecords][label::NCBI][label::_at_id].addNode();
		docReference[label::externalRecords][label::NCBI][label::id].addNode();
		docReference[label::externalRecords][label::Ensembl][label::_at_id].addNode();
		docReference[label::externalRecords][label::Ensembl][label::id].addNode();
		docReference[label::externalRecords][label::LRG][label::_at_id].addNode();
		docReference[label::externalRecords][label::LRG][label::id].addNode();
		docReference[label::externalRecords][label::GenBank][label::_at_id].addNode();
		docReference[label::externalRecords][label::GenBank][label::id].addNode();
			// undocumented
//			pim->fields["sequence"] = false;
//			pim->fields["splicedSequence"] = false;
//			pim->fields["CDS"][label::start] = false;
//			pim->fields["CDS"][label::end] = false;
	} else if (docType == documentType::gene) {
		jsonBuilder::DocumentStructure<label> & docGene = pim->docSchema;
		docGene[label::_at_context].addNode();
		docGene[label::_at_id     ].addNode();
		docGene[label::names].addNode();
		docGene[label::externalRecords][label::HGNC][label::_at_id].addNode();
		docGene[label::externalRecords][label::HGNC][label::id].addNode();
		docGene[label::externalRecords][label::HGNC][label::name].addNode();
		docGene[label::externalRecords][label::HGNC][label::symbol].addNode();
		docGene[label::externalRecords][label::NCBI][label::_at_id].addNode();
		docGene[label::externalRecords][label::NCBI][label::id].addNode();
		docGene[label::externalRecords][label::MANEPrefRefSeq][label::_at_id].addNode();
		docGene[label::externalRecords][label::MANEPrefRefSeq][label::id].addNode();
	} else if (docType == documentType::coordinateTransform) {
		jsonBuilder::DocumentStructure<label> & docCoordinates = pim->docSchema;
		docCoordinates[label::_at_context].addNode();
		docCoordinates[label::_at_id     ].addNode();
		docCoordinates[label::transformations].addNode();
		docCoordinates[label::transformations][label::referenceGenome].addNode();
		docCoordinates[label::transformations][label::chromosome].addNode();
		docCoordinates[label::transformations][label::start].addNode();
		docCoordinates[label::transformations][label::end].addNode();
	} else if (docType == documentType::ga4ghSeqMetadata){
		jsonBuilder::DocumentStructure<label> & docSeqMetadata = pim->docSchema;
		docSeqMetadata[label::metadata].addNode();
		docSeqMetadata[label::metadata][label::trunc512].addNode();
		docSeqMetadata[label::metadata][label::length].addNode();
		docSeqMetadata[label::metadata][label::aliases].addNode();
		docSeqMetadata[label::metadata][label::aliases][label::alias].addNode();
		docSeqMetadata[label::metadata][label::aliases][label::naming_authority].addNode();
	}
}


OutputFormatter::~OutputFormatter()
{
	delete pim;
}

void OutputFormatter::setFormat(responseFormat format, std::string const & fields)
{
	ASSERT(format != responseFormat::html);
	pim->format = format;

	// parse fields
	jsonBuilder::DocumentSettings docConf;
	try {
		if (pim->format == responseFormat::txt) {
			// for txt format everything is set to empty ("none")
			jsonBuilder::DocumentSettings empty;
			empty.layout = jsonBuilder::DocumentSettings::empty;
			docConf = empty;
			pim->extSrcConf = empty;
		} else {
			// for json formats the "fields" parameter is parsed
			docConf = jsonBuilder::DocumentSettings::parse(fields);
			pim->extSrcConf = docConf.extractSubdocument("externalSources");
		}
	} catch (std::exception const & e) {
		throw ExceptionIncorrectRequest(std::string("Incorrect value of 'fields' parameter: ") + std::string(e.what()));
	}

	// interpret fields
	switch (docConf.layout) {
	case jsonBuilder::DocumentSettings::empty:
		pim->docSchema.hideAllNodes();
		break;
	case jsonBuilder::DocumentSettings::standard:
		pim->docSchema[label::externalSources].setState(false);
		break;
	case jsonBuilder::DocumentSettings::full:
		// do nothing
		break;
	}
	for ( auto const & mw: docConf.modificators ) {
		bool const active = (mw.first == jsonBuilder::DocumentSettings::add);
		jsonBuilder::NodeOfDocumentStructure<label> n = pim->docSchema.root();
		for ( auto const & s: mw.second ) {
			n = n[toLabel(s)]; // TODO - incorrect label ?
		}
		n.setState(active);
	}
}

template<identifierType idType>
inline void identifier(jsonBuilder::NodeOfJsonTree<label> const & ob, IdentifierShort const & id);

template<>
inline void identifier<identifierType::dbSNP>(jsonBuilder::NodeOfJsonTree<label> const & ob, IdentifierShort const & id2)
{
	Identifier_dbSNP const & id = id2.as_dbSNP();
	ob[label::_at_id] = "http://www.ncbi.nlm.nih.gov/snp/" + boost::lexical_cast<std::string>(id.rs);
	ob[label::rs] = id.rs;
}

template<>
inline void identifier<identifierType::ClinVarAllele>(jsonBuilder::NodeOfJsonTree<label> const & ob, IdentifierShort const & id2)
{
	Identifier_ClinVarAllele const & id = id2.as_ClinVarAllele();
	ob[label::_at_id] = "http://www.ncbi.nlm.nih.gov/clinvar/?term=" + boost::lexical_cast<std::string>(id.alleleId) + "[alleleid]";
	ob[label::alleleId] = id.alleleId;
	if (! id.preferredName.empty()) ob[label::preferredName] = id.preferredName;
	else ob[label::preferredName] = boost::lexical_cast<std::string>(id.alleleId);
}

template<>
inline void identifier<identifierType::ClinVarVariant>(jsonBuilder::NodeOfJsonTree<label> const & ob, IdentifierShort const & id2)
{
	Identifier_ClinVarVariant const & id = id2.as_ClinVarVariant();
	ob[label::_at_id] = "http://www.ncbi.nlm.nih.gov/clinvar/variation/" + boost::lexical_cast<std::string>(id.variantId);
	ob[label::variationId] = id.variantId;
	for (auto r: id.RCVs) {
		std::string const number = boost::lexical_cast<std::string>(r);
		ob[label::RCV].push_back("RCV" + std::string(9-number.size(),'0') + number);
	}
}

template<>
inline void identifier<identifierType::AllelicEpigenome>(jsonBuilder::NodeOfJsonTree<label> const & ob, IdentifierShort const & id2)
{
	Identifier_AllelicEpigenome const & id = id2.as_AllelicEpigenome();
	std::ostringstream ss;
	ss << "http://genboree.org/genboreeKB/genboree_kbs?project_id=allelic-epigenome&coll=AllelicEpigenome-chr";
	ss << toString(id.chr) + "&doc=AE" << std::setw(7) << std::setfill('0') << id.ae;
	ob[label::_at_id] = ss.str();
}

template<>
inline void identifier<identifierType::COSMIC>(jsonBuilder::NodeOfJsonTree<label> const & ob, IdentifierShort const & id2)
{
	Identifier_COSMIC const & id = id2.as_COSMIC();
	if (id.active) {
		std::ostringstream ss;
		ss << "http://cancer.sanger.ac.uk/cosmic/";
		if (id.coding) ss << "mutation"; else ss << "ncv";
		ss << "/overview?id=" << id.id;
		ob[label::_at_id] = ss.str();
	}
	ob[label::id] = (id.coding ? "COSM" : "COSN") + boost::lexical_cast<std::string>(id.id);
	ob[label::active] = id.active;
}

template<identifierType idType>
inline void identifier(jsonBuilder::NodeOfJsonTree<label> const & ob, IdentifierWellDefined const & id, DocumentActiveGenomicVariant const & doc);

template<>
inline void identifier<identifierType::MyVariantInfo_hg19>(jsonBuilder::NodeOfJsonTree<label> const & ob, IdentifierWellDefined const & id, DocumentActiveGenomicVariant const & doc)
{
	std::string const hgvsId = buildIdentifierMyVariantInfo(OutputFormatter::refDb,id,doc.mainDefinition);
	ob[label::_at_id] = "http://myvariant.info/v1/variant/" + hgvsId + "?assembly=hg19";
	ob[label::id] = hgvsId;
}

template<>
inline void identifier<identifierType::MyVariantInfo_hg38>(jsonBuilder::NodeOfJsonTree<label> const & ob, IdentifierWellDefined const & id, DocumentActiveGenomicVariant const & doc)
{
	std::string const hgvsId = buildIdentifierMyVariantInfo(OutputFormatter::refDb,id,doc.mainDefinition);
	ob[label::_at_id] = "http://myvariant.info/v1/variant/" + hgvsId + "?assembly=hg38";
	ob[label::id] = hgvsId;
}

template<>
inline void identifier<identifierType::ExAC>(jsonBuilder::NodeOfJsonTree<label> const & ob, IdentifierWellDefined const & id, DocumentActiveGenomicVariant const & doc)
{
	std::string chr, pos, ref, alt;
	buildIdentifierExACgnomAD(OutputFormatter::refDb, id, doc.mainDefinition, chr, pos, ref, alt);
	std::string const sid =  chr + "-" + pos + "-" + ref + "-" + alt;
	ob[label::id] = sid;
	ob[label::_at_id] = "http://exac.broadinstitute.org/variant/" + sid;
	ob[label::variant] = chr + ":" + pos + " " + ref + " / " + alt;
}

template<>
inline void identifier<identifierType::gnomAD>(jsonBuilder::NodeOfJsonTree<label> const & ob, IdentifierWellDefined const & id, DocumentActiveGenomicVariant const & doc)
{
	std::string chr, pos, ref, alt;
	buildIdentifierExACgnomAD(OutputFormatter::refDb, id, doc.mainDefinition, chr, pos, ref, alt);
	std::string const sid = chr + "-" + pos + "-" + ref + "-" + alt;
	ob[label::id] = sid;
	ob[label::_at_id] = "http://gnomad.broadinstitute.org/variant/" + sid;
	ob[label::variant] = chr + ":" + pos + " " + ref + " / " + alt;
}

template<identifierType idType>
inline void identifiers(jsonBuilder::NodeOfJsonTree<label> const & arr, std::vector<IdentifierShort> const & ids)
{
	for (auto id: ids) arr.push_back( [&](jsonBuilder::NodeOfJsonTree<label> const & arr){ identifier<idType>(arr,id); } );
}

template<identifierType idType>
inline void identifiers(jsonBuilder::NodeOfJsonTree<label> const & arr, std::vector<IdentifierWellDefined> const & ids, DocumentActiveGenomicVariant const & doc)
{
	for (auto id: ids) try { arr.push_back( [&](jsonBuilder::NodeOfJsonTree<label> const & arr){ identifier<idType>(arr,id,doc); } ); } catch (...) {} // TODO - add to log
}

inline void coordinate(jsonBuilder::NodeOfJsonTree<label> const & ob, SplicedRegionCoordinates const & region, std::string const & refAllele, std::string const & newAllele)
{
	ob[label::start] = region.left ().position();
	ob[label::end  ] = region.right().position();
	if (region.left().offsetSize() || ! region.left().hasNegativeOffset()) {
		ob[label::startIntronOffset   ] = region.left().offsetSize();
		ob[label::startIntronDirection] = (region.left().hasNegativeOffset()) ? "-" : "+";
	}
	if (region.right().offsetSize() || region.right().hasNegativeOffset()) {
		ob[label::endIntronOffset   ] = region.right().offsetSize();
		ob[label::endIntronDirection] = (region.right().hasNegativeOffset()) ? "-" : "+";
	}
	ob[label::referenceAllele] = refAllele;
	ob[label::allele] = newAllele;
}

inline void coordinate(jsonBuilder::NodeOfJsonTree<label> const & ob, RegionCoordinates const & region, std::string const & refAllele, std::string const & newAllele)
{
	coordinate(ob, SplicedRegionCoordinates(region), refAllele, newAllele);
}

template<class tSeqMod>
inline void coordinates(jsonBuilder::NodeOfJsonTree<label> const & arr, std::vector<tSeqMod> const & mods)
{
	for (auto const & m: mods) arr.push_back( [&](jsonBuilder::NodeOfJsonTree<label> const & ob){ coordinate(ob,m.region,m.originalSequence,m.newSequence); } );
}


template<class tVariantDetails>
inline void OutputFormatter::Pim::variantCommon(jsonBuilder::NodeOfJsonTree<label> const & ob, tVariantDetails const & v)
{
	std::vector<std::string> names = OutputFormatter::refDb->getNames(v.definition.refId);
	std::string carRsId = "";
	for (auto & n: names) if (n.substr(0,2) == "RS") {
		carRsId = n;
		n = names.back();
		names.pop_back();
		break;
	}
	for (auto & n: names) ob[label::hgvs].push_back(n + ":" + v.hgvsDefs);
	ob[label::referenceSequence] = getURI("refseq/RS", boost::lexical_cast<unsigned>(carRsId.substr(2)));
}

inline void OutputFormatter::Pim::object(jsonBuilder::NodeOfJsonTree<label> const & ob, VariantDetailsGenomeBuild const & v)
{
	variantCommon(ob, v);
	coordinates(ob[label::coordinates], v.definition.modifications);
	ReferenceMetadata const & m = OutputFormatter::refDb->getMetadata(v.definition.refId);
	ob[label::referenceGenome] = m.genomeBuild;
	if (m.chromosome != chrUnknown) ob[label::chromosome] = toString(m.chromosome);
}

inline void OutputFormatter::Pim::object(jsonBuilder::NodeOfJsonTree<label> const & ob, VariantDetailsGeneRegion const & v)
{
	variantCommon(ob, v);
	coordinates(ob[label::coordinates], v.definition.modifications);
	ReferenceMetadata const & m = OutputFormatter::refDb->getMetadata(v.definition.refId);
	if (m.geneId != 0) {
		ob[label::gene] = getURI("gene/GN", m.geneId);
		ob[label::geneSymbol] = refDb->getGeneById(m.geneId).hgncSymbol;
		ob[label::geneNCBI_id] = refDb->getGeneById(m.geneId).refSeqId;
	}
}

inline void OutputFormatter::Pim::object(jsonBuilder::NodeOfJsonTree<label> const & ob, VariantDetailsTranscript const & v)
{
	variantCommon(ob, v);
	coordinates(ob[label::coordinates], v.definition.modifications);
	ReferenceMetadata const & m = OutputFormatter::refDb->getMetadata(v.definition.refId);
	if (m.geneId != 0) {
		ob[label::gene] = getURI("gene/GN", m.geneId);
		ob[label::geneSymbol] = refDb->getGeneById(m.geneId).hgncSymbol;
		ob[label::geneNCBI_id] = refDb->getGeneById(m.geneId).refSeqId;
	}
	if (! v.proteinHgvsDef.empty()) ob[label::proteinEffect][label::hgvs] = v.proteinHgvsDef;
	if (! v.proteinHgvsCanonical.empty()) ob[label::proteinEffect][label::hgvsWellDefined] = v.proteinHgvsCanonical;
}

inline void OutputFormatter::Pim::object(jsonBuilder::NodeOfJsonTree<label> const & ob, VariantDetailsProtein const & v)
{
	variantCommon(ob, v);
	coordinates(ob[label::coordinates], v.definition.modifications);
	ReferenceMetadata const & m = OutputFormatter::refDb->getMetadata(v.definition.refId);
	if (m.geneId != 0) {
		ob[label::gene] = getURI("gene/GN", m.geneId);
		ob[label::geneSymbol] = refDb->getGeneById(m.geneId).hgncSymbol;
		ob[label::geneNCBI_id] = refDb->getGeneById(m.geneId).refSeqId;
	}
	for (auto hgvs: v.hgvsMatchingTranscriptVariants) {
		ob[label::hgvsMatchingTranscriptVariant].push_back(hgvs);
	}
}

template<class tObject>
inline void OutputFormatter::Pim::objects(jsonBuilder::NodeOfJsonTree<label> const & node, std::vector<tObject> const & objects)
{
	if (objects.empty()) return;
	for (auto o: objects) node.push_back( [&](jsonBuilder::NodeOfJsonTree<label> const & n){object(n,o);} );
}


std::string OutputFormatter::createOutput(Document const & doc) const
{
	// ============================= TXT output
	if (pim->format == responseFormat::txt) {
		std::string response = "";
		if (doc.isError()) {
			DocumentError const & err = doc.error();
			response = "ERROR\t" + toString(err.type) + "\t" + err.message;
			for (auto const & kv: err.fields) {
				response.append( getTextLabels()[kv.first] + "=" + kv.second );
			}
		} else if (doc.isActiveGenomicVariant()) {
			DocumentActiveGenomicVariant const & var = doc.asActiveGenomicVariant();
			response = refDb->getNames(var.mainDefinition.refId).front() + ":" + toHgvsModifications(refDb,var.mainDefinition) + "\tCA" + std::to_string(var.caId.value);
		} else if (doc.isActiveProteinVariant()) {
			DocumentActiveProteinVariant const & var = doc.asActiveProteinVariant();
			response = refDb->getNames(var.mainDefinition.refId).front() + ":" + toHgvsModifications(refDb,var.mainDefinition) + "\tPA" + std::to_string(var.caId.value);
		} else {
			response = "ERROR\tDocumentCannotBeConvertedToTxt";
		}
		return response;
	}

	// ============================= JSON document

	jsonBuilder::JsonTree<label> jsonDoc(&(pim->docSchema));
	jsonBuilder::NodeOfJsonTree<label> out = jsonDoc.getContext();

	if (doc.isError()) {
		DocumentError const & err = doc.error();
		out[label::errorType] = toString(err.type);
		out[label::description] = description(err.type);
		if (err.message != "") out[label::message] = err.message;
		for (auto const & kv: err.fields) {
			out[kv.first] = kv.second;
		}
	} else if (doc.isInactiveVariant()) {
		DocumentInactiveVariant const & old = doc.inactiveVariant();
		//TODO - type & @id
		//for (auto & p: old.newCaIds) out["activeUris"].push_back(boost::lexical_cast<std::string>(p.second));
		// TODO - type & @id
	} else if (doc.isActiveGenomicVariant()) {
		DocumentActiveGenomicVariant const & var = doc.asActiveGenomicVariant();
		out[label::_at_context] = "http://" + pim->carURI + "/schema/allele.jsonld";
		out[label::_at_id] = (var.caId.isNull()) ? ("_:CA") : (pim->getURI("allele/CA", var.caId.value));
		out[label::type] = "nucleotide";

		// external records
		identifiers<identifierType::dbSNP             >(out[label::externalRecords][label::dbSNP             ], var.identifiers.getShortIds(identifierType::dbSNP         ));
		identifiers<identifierType::ClinVarAllele     >(out[label::externalRecords][label::ClinVarAlleles    ], var.identifiers.getShortIds(identifierType::ClinVarAllele ));
		identifiers<identifierType::ClinVarVariant    >(out[label::externalRecords][label::ClinVarVariations ], var.identifiers.getShortIds(identifierType::ClinVarVariant));
		identifiers<identifierType::MyVariantInfo_hg38>(out[label::externalRecords][label::MyVariantInfo_hg38], var.identifiers.getHgvsIds(identifierType::MyVariantInfo_hg38),var);
		identifiers<identifierType::MyVariantInfo_hg19>(out[label::externalRecords][label::MyVariantInfo_hg19], var.identifiers.getHgvsIds(identifierType::MyVariantInfo_hg19),var);
		identifiers<identifierType::AllelicEpigenome  >(out[label::externalRecords][label::AllelicEpigenome  ], var.identifiers.getShortIds(identifierType::AllelicEpigenome));
		identifiers<identifierType::COSMIC            >(out[label::externalRecords][label::COSMIC            ], var.identifiers.getShortIds(identifierType::COSMIC));

		identifiers<identifierType::ExAC  >(out[label::externalRecords][label::ExAC  ], var.identifiers.getHgvsIds(identifierType::ExAC  ),var);
		identifiers<identifierType::gnomAD>(out[label::externalRecords][label::gnomAD], var.identifiers.getHgvsIds(identifierType::gnomAD),var);

		// external sources
		if (! var.jsonExternalSources.empty()) {
			out[label::externalSources].bindSubdocument(var.jsonExternalSources);
		}

		// genomic alleles
		pim->objects(out[label::genomicAlleles], var.definitionsOnGenomeBuilds);
		pim->objects(out[label::genomicAlleles], var.definitionsOnGenesRegions);

		// transcript alleles
		pim->objects(out[label::transcriptAlleles], var.definitionsOnTranscripts);
	} else if (doc.isActiveProteinVariant()) {
		DocumentActiveProteinVariant const & var = doc.asActiveProteinVariant();
		out[label::_at_context] = "http://" + pim->carURI + "/schema/allele.jsonld";
		out[label::_at_id] = (var.caId.isNull()) ? ("_:PA") : (pim->getURI("allele/PA", var.caId.value));
		out[label::type] = "amino-acid";

		// external records
		identifiers<identifierType::ClinVarAllele >(out[label::externalRecords][label::ClinVarAlleles   ], var.identifiers.getShortIds(identifierType::ClinVarAllele ));
		identifiers<identifierType::ClinVarVariant>(out[label::externalRecords][label::ClinVarVariations], var.identifiers.getShortIds(identifierType::ClinVarVariant));

		// external sources
		if (! var.jsonExternalSources.empty()) {
			out[label::externalSources].bindSubdocument(var.jsonExternalSources);
		}

		// alleles
		if ( ! var.definitionOnProtein.definition.refId.isNull() ) {
			out[label::aminoAcidAlleles].push_back( [&](jsonBuilder::NodeOfJsonTree<label> const & n){ pim->object(n,var.definitionOnProtein); } );
		}

	} else if (doc.isReference()) {
		DocumentReference const & ref = doc.reference();
		out[label::_at_context] = "http://" + pim->carURI + "/schema/refseq.jsonld";
		out[label::_at_id] = pim->getURI("refseq/RS", ref.rsId);
		out[label::type] = (ref.transcript) ? ("transcript") : ( (ref.genomeBuild == ReferenceGenome::rgUnknown) ? ("gene") : ("chromosome") );
		if (ref.gnId > 0) out[label::gene] = pim->getURI("gene/GN", ref.gnId);
		if (ref.genomeBuild != ReferenceGenome::rgUnknown) out[label::referenceGenome] = toString(ref.genomeBuild);
		if (ref.chromosome  != Chromosome::chrUnknown    ) out[label::chromosome     ] = toString(ref.chromosome );
		if (ref.ncbiId != "") {
			out[label::externalRecords][label::NCBI][label::_at_id] = "http://www.ncbi.nlm.nih.gov/nuccore/" + ref.ncbiId;
			out[label::externalRecords][label::NCBI][label::id] = ref.ncbiId;
		}
		if (ref.ensemblId != "") {
			out[label::externalRecords][label::Ensembl][label::_at_id] = "http://ensembl.org/Homo_sapiens/Transcript/Summary?t=" + ref.ensemblId;
			out[label::externalRecords][label::Ensembl][label::id] = ref.ensemblId;
		}
		//TODO
		//		out[label::externalRecords][label::LRG][label::_at_id] = true;
		//		out[label::externalRecords][label::LRG][label::id] = true;
		//		out[label::externalRecords][label::GenBank][label::_at_id] = true;
		//		out[label::externalRecords][label::GenBank][label::id] = true;
		// ============== undocumented
		if (ref.sequence        != "") out[label::sequence       ] = ref.sequence;
		if (ref.splicedSequence != "") out[label::splicedSequence] = ref.splicedSequence;
		if (! ref.CDS.isNull()) {
			out[label::CDS][label::start] = ref.CDS.left ();
			out[label::CDS][label::end  ] = ref.CDS.right();
		}
	} else if (doc.isGene()) {
		DocumentGene const & gene = doc.gene();
		out[label::_at_context] = "http://" + pim->carURI + "/schema/gene.jsonld";
		out[label::_at_id] = pim->getURI("gene/GN", gene.gnId);
		out[label::externalRecords][label::HGNC][label::_at_id] = "http://www.genenames.org/cgi-bin/gene_symbol_report?hgnc_id=HGNC:" + boost::lexical_cast<std::string>(gene.gnId);
		out[label::externalRecords][label::HGNC][label::id    ] = "HGNC:" + boost::lexical_cast<std::string>(gene.gnId);
		out[label::externalRecords][label::HGNC][label::name  ] = gene.hgncName;
		out[label::externalRecords][label::HGNC][label::symbol] = gene.hgncSymbol;
		if (gene.ncbiId > 0) {
			out[label::externalRecords][label::NCBI][label::_at_id] = "http://www.ncbi.nlm.nih.gov/gene/" + boost::lexical_cast<std::string>(gene.ncbiId);
			out[label::externalRecords][label::NCBI][label::id    ] = gene.ncbiId;
		}
		if(gene.prefRefSeq != "") { 
			out[label::externalRecords][label::MANEPrefRefSeq][label::_at_id] = "https://www.ncbi.nlm.nih.gov/nuccore/" + boost::lexical_cast<std::string>(gene.prefRefSeq);
			out[label::externalRecords][label::MANEPrefRefSeq][label::id] = gene.prefRefSeq;
		}
	} else if (doc.isCoordinateTransform()) {
		DocumentCoordinates const & coordinates = doc.coordinateTransform();

		out[label::_at_id] = "_:CTRM";
		out[label::_at_context] = "http://" + pim->carURI + "/schema/allele.jsonld";
		if(coordinates.grch38Assembly != ""){	
			out[label::transformations].push_back([&](jsonBuilder::NodeOfJsonTree<label> map1){
					map1[label::referenceGenome] = coordinates.grch38Assembly;
					map1[label::chromosome] = coordinates.grch38Chr;
					map1[label::start] = coordinates.grch38Start;
					map1[label::end] = coordinates.grch38End;
			});
		}
		if(coordinates.grch37Assembly != ""){
			out[label::transformations].push_back([&](jsonBuilder::NodeOfJsonTree<label> map1){
					map1[label::referenceGenome] = coordinates.grch37Assembly;
					map1[label::chromosome] = coordinates.grch37Chr;
					map1[label::start] = coordinates.grch37Start;
					map1[label::end] = coordinates.grch37End;
			});
		}
		if(coordinates.ncbi36Assembly != ""){
			out[label::transformations].push_back([&](jsonBuilder::NodeOfJsonTree<label> map1){
					map1[label::referenceGenome] = coordinates.ncbi36Assembly;
					map1[label::chromosome] = coordinates.ncbi36Chr;
					map1[label::start] = coordinates.ncbi36Start;
					map1[label::end] = coordinates.ncbi36End;
			});
		}
  } else if(doc.isGa4ghSeqServiceInfo()) {
  	DocumentSequenceServiceInfo const & service_info =  doc.ga4ghSeqServiceInfo();
  	// out[lab]
  	out[label::service][label::circular_supported] = service_info.circular_supported;
  	
  	for(auto & algorithm: service_info.algorithms){ 
  	  		out[label::service][label::algorithms].push_back(algorithm);
  	}

  	out[label::service][label::subsequence_limit] = service_info.subsequence_limit;


  	for(auto & supported_api_version: service_info.supported_api_versions){
  	  		out[label::service][label::supported_api_versions].push_back(supported_api_version);
  	  	}
  	
  } else if(doc.isGa4ghSeqMetadata()) {
  	DocumentGa4ghSeqMetadata const & metadata = doc.ga4ghSeqMetadata();
  	out[label::metadata][label::trunc512] = metadata.trunc512;
  	out[label::metadata][label::length] = metadata.seqLength;
  	// add aliases
  	for(auto & alias: metadata.aliases){
	  	out[label::metadata][label::aliases].push_back([&](jsonBuilder::NodeOfJsonTree<label> map1){
	  		
	  		map1[label::alias] = alias;

		    switch(alias[0]){
		      case 'N' : map1[label::naming_authority] = "NCBI";
		        break;
		      case 'X' : map1[label::naming_authority] = "NCBI";
		        break;
		      case 'E' : map1[label::naming_authority] = "ENSEMBL";
		        break;
		      case 'L' : map1[label::naming_authority] = "LRG";
		        break;
		      default: map1[label::naming_authority] = "UNKNOWN"; 
		    }
	  	});
  	}
  };


	jsonDoc.removeEmptyArraysAndObjects();
	std::string r;
	if (pim->format == responseFormat::compressedJson) {
		r = jsonDoc.toString<false>(getTextLabels());
	} else {
		r = jsonDoc.toString<true>(getTextLabels());
	}
	if (r == "") r = "{}";
	return r;
}


bool OutputFormatter::returnsGenomeBuildsVariants() const
{
	return pim->docSchema.isActive(jsonBuilder::Path<label>() + label::genomicAlleles);
}

bool OutputFormatter::returnsGenesRegionsVariants() const
{
	return returnsGenomeBuildsVariants();
}

bool OutputFormatter::returnsTranscriptsVariants() const
{
	return pim->docSchema.isActive(jsonBuilder::Path<label>() + label::transcriptAlleles);
}

bool OutputFormatter::returnsProteinsVariants() const
{
	return pim->docSchema.isActive(jsonBuilder::Path<label>() + label::aminoAcidAlleles);
}

jsonBuilder::DocumentSettings OutputFormatter::externalSourcesConfiguration() const
{
	return pim->extSrcConf;
}

bool OutputFormatter::prettyJson() const
{
	return (pim->format == responseFormat::prettyJson);
}

bool OutputFormatter::prettyOrCompressedJson() const
{
	return (pim->format == responseFormat::prettyJson || pim->format == responseFormat::compressedJson);
}

/*
void xxx(NodeOfJsonTree const & node)
{
	node[label::referenceGenome] = "ref genome";
	node[label::chromosome] = 123ll;
	node[label::coordinates].push_back()[label::start] = 6789ll;
}

int main()
{
	init();
	DocumentStructure doc = init2();

	JsonTree root(&doc);

	NodeOfJsonTree n = root.getContext();

	n[label::type] = 1234ll;
	n[label::genomicAlleles].push_back(xxx);
	n[label::_at_id].push_back("AAA");
	n[label::_at_id].push_back("BBB");
	n[label::_at_id].push_back("CCC");

	n[label::_at_context] = "To jest ensembl";

	std::cout << root.toString() << std::endl;
	std::cout << root.toStringPretty(0) << std::endl;

	return 0;
}
*/
