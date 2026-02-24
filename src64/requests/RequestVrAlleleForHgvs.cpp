#include "requests.hpp"
#include "hgvs.hpp"
#include "canonicalization.hpp"

RequestVrAlleleForHgvs::RequestVrAlleleForHgvs(std::string const & pId)
: Request(documentType::activeGenomicVariant), fHgvs(pId){}

RequestVrAlleleForHgvs::~RequestVrAlleleForHgvs(){
  stopProcessingThread();
}

void RequestVrAlleleForHgvs::process(){
  HgvsVariant hgvsVar(true);
  decodeHgvs(referencesDb, fHgvs, hgvsVar);
  std::vector<Document> documents(1);
  std::vector<ReferenceId> current_refids;

  if (hgvsVar.isGenomic()) {
    documents[0] = DocumentActiveGenomicVariant(canonicalize(referencesDb, hgvsVar.genomic));
    mapVariantsToMainGenome(documents);
    allelesDb->fetchVariantsByDefinitionAndAddIdentifiers(documents);
    fillVariantsDetails(documents);
    // Convert to VR
    DocumentActiveGenomicVariant & d = documents[0].asActiveGenomicVariant();
    current_refids.push_back(hgvsVar.genomic.refId);
    // Check those are not supported yet
    if(d.mainDefinition.modifications.size() != 1){
      throw std::runtime_error("VR specifications for haplotypes are not supported yet!");
    } else if (d.mainDefinition.modifications.front().category != variantCategory::nonShiftable){
      throw std::runtime_error("The normalization schema suggested by VR (justification) is different from the one used by Allele registry (shifting after genome mapping). Currently, we are not able to provide VR representation of variant (shiftable in/del) you searched.");
    } else {
      std::vector<std::string> response = d.toVrString(referencesDb, current_refids);
      setResponse(response[0]);
    }
  } else {
    throw std::runtime_error("VR response for Amino-acid variations are not supported yet");
  }

  setResponse(documents[0]);
}