/*
Request handler for endpoint to query by digest and produce refget compatible response:
*/

#include "requests.hpp"
#include "hgvs.hpp"
#include "canonicalization.hpp"

RequestMetadataForSequenceByDigest::RequestMetadataForSequenceByDigest(std::string const & qDigest)
:Request(documentType::ga4ghSeqMetadata), digest(qDigest)
{}

RequestMetadataForSequenceByDigest::~RequestMetadataForSequenceByDigest(){
  stopProcessingThread();
}

void RequestMetadataForSequenceByDigest::process(){

  std::vector<DocumentGa4ghSeqMetadata> documents;

  std::vector<std::string> accessions = referencesDb->getSequenceIdentifiersForDigest(digest);
  // Get index of the sequence
  documents.push_back(DocumentGa4ghSeqMetadata());


  ReferenceId local_id;
  std::string type = "";

  for(auto & accession: accessions){
    if(local_id.isNull()){
      try {
        local_id = referencesDb->getReferenceId(accession);
      } catch (const std::runtime_error& error){};
    }
    documents[0].aliases.push_back(accession);
  }

  if(local_id.isNull()){
    throw std::runtime_error("Could not map digest to allele registry internal identifiers, hence we can not determing the length of sequence. Please ask administrator to add this sequence in allele registry database.");
  } else {
    documents[0].seqLength = referencesDb->getSequenceLength(local_id);
    documents[0].trunc512 = digest;
  }

  setResponse(documents[0]);
}