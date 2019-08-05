#include "document.hpp"
#include <boost/lexical_cast.hpp>


DocumentError DocumentError::createFromCurrentException()
{
	DocumentError d;
	try {
		std::rethrow_exception( std::current_exception() );
		d.type = errorType::InternalServerError;
		d.message = "Unknown error occurred";
		d.fields[label::debug] = "Empty exception container!";
	} catch (ExceptionBase const & e) {
		d.type = e.type();
		d.fields = e.fields();
		d.message = e.what();
	} catch (std::exception const & e) {
		d.type = errorType::InternalServerError;
		d.message = e.what();
	} catch (...) {
		d.type = errorType::InternalServerError;
		d.message = "Unknown error occurred";
	}
	return d;
}


std::string DocumentError::toString() const
{
	return "[" + boost::lexical_cast<std::string>(type) + "," + message + "]";
}

std::string DocumentActiveGenomicVariant::toString() const
{
	return "[" + mainDefinition.toString() + ",CA" + boost::lexical_cast<std::string>(caId.value) + ","+ identifiers.toString() + "]";
}

std::string DocumentActiveProteinVariant::toString() const
{
	return "[" + mainDefinition.toString() + ",PA" + boost::lexical_cast<std::string>(caId.value) + ","+ identifiers.toString() + "]";
}

std::string DocumentInactiveVariant::toString() const
{
	std::string s = "[";
	bool first = true;
	for (auto const & nc: newCaIds) {
		if (first) {
			first = false;
		} else {
			s += ",";
		}
		s += "(" + boost::lexical_cast<std::string>(nc.first.value) + "," + boost::lexical_cast<std::string>(nc.second) + ")";
	}
	return (s + "]");
}

std::string DocumentReference::toString() const
{
	return "[RS" + boost::lexical_cast<std::string>(rsId) + "]";
}

std::string DocumentGene::toString() const
{
	return "[GN" + boost::lexical_cast<std::string>(gnId) + "]";
}

Document const Document::null;

std::string Document::toString() const
{
	switch (fType) {
		case documentType::null           : return "{}";
		case documentType::error          : return "{error:"           + error          ().toString() + "}";
		case documentType::activeGenomicVariant  : return "{activeGenomicVariant:"   + asActiveGenomicVariant().toString() + "}";
		case documentType::activeProteinVariant  : return "{activeProteinVariant:"   + asActiveProteinVariant().toString() + "}";
		case documentType::inactiveVariant: return "{inactiveVariant:" + inactiveVariant().toString() + "}";
		case documentType::reference      : return "{reference:"       + reference      ().toString() + "}";
		case documentType::gene           : return "{gene:"            + gene           ().toString() + "}";
	}
	throw std::logic_error("Unknown document type");
}

/*
Following functions are added by Ronak Patel.
A very preliminary JSON builder for simple intervals
Complext interval needs an implementation
*/
std::string vrJSONBuilder(std::string const & sequence_digest, unsigned cstart, unsigned cend, std::string const & alternate)
{

	// Bad and ugly JSON generation.
	// Just to get the specs rolling
	std::string interval = "\"interval\":{\"end\":" + boost::lexical_cast<std::string>(cend) + 
                                      ",\"start\":" + boost::lexical_cast<std::string>(cstart) + 
                                      ",\"type\":" + "\"SimpleInterval\"}";

  std::string location = "{" + interval + ",\"sequence_id\":\"" + sequence_digest + "\"," + "\"type\":\"SequenceLocation\"}";
  std::string location_digest = trunc512(location);
  std::string state = "\"state\":{\"sequence\":\"" + alternate + "\",\"type\":\"SequenceState\"},\"type\":\"Allele\"";
  std::string allele_string = "{\"location\":\"" + location_digest + "\"," + state + "}";
  std::string allele_digest = trunc512(allele_string);


  std::string final_response = "{\"id\":\"ga4gh:VA."+allele_digest+"\"," 
  + "\"location\":" + "{\"id\":\"ga4gh:SL." + location_digest + "\"," + 
  interval + 
  ",\"sequence_id\":\"ga4gh:SQ." + 
  sequence_digest + 
  "\"," + 
  "\"type\":\"SequenceLocation\"}," +
  state + 
  "}";

  return(final_response);

}

std::vector<std::string> DocumentActiveGenomicVariant::toVrString(ReferencesDatabase const * refDb, 
	std::vector<ReferenceId> const & reference_sequence_ids) const {
		// This is the place where each JSON string will be pushed.
		std::vector<std::string> json_strings;

		for(auto & rseqid: reference_sequence_ids){
			std::string digest = "";
			for(auto & def: definitionsOnGenomeBuilds){
				if(rseqid == def.definition.refId){
					std::vector<std::string> ref_seqs = refDb->getNames(rseqid);
					digest = refDb->getDigestFromSequenceAccession(ref_seqs);
					if(def.definition.modifications.size() != 1){
						throw std::runtime_error("Haplotypes not supported yet!");
					} else {
						json_strings.push_back(vrJSONBuilder(digest, def.definition.modifications.front().region.left(), def.definition.modifications.front().region.right(), def.definition.modifications.front().newSequence));
					}
				}
			}
			for(auto & def: definitionsOnTranscripts){
				if(rseqid == def.definition.refId){
					std::vector<std::string> ref_seqs = refDb->getNames(rseqid);
					digest = refDb->getDigestFromSequenceAccession(ref_seqs);
					if(def.definition.modifications.size() != 1){
						throw std::runtime_error("Haplotypes not supported yet!");
					} else {
						json_strings.push_back(vrJSONBuilder(digest, def.definition.modifications.front().region.left().position(), def.definition.modifications.front().region.right().position(), def.definition.modifications.front().newSequence));
					}						
				}
			}
			for(auto & def: definitionsOnGenesRegions){
				if(rseqid == def.definition.refId){
					std::vector<std::string> ref_seqs = refDb->getNames(rseqid);
					digest = refDb->getDigestFromSequenceAccession(ref_seqs);
					if(def.definition.modifications.size() != 1){
						throw std::runtime_error("Haplotypes not supported yet!");
					} else {
						json_strings.push_back(vrJSONBuilder(digest, def.definition.modifications.front().region.left(), def.definition.modifications.front().region.right(), def.definition.modifications.front().newSequence));
					}
				}
			}	
		}
		return(json_strings);
}