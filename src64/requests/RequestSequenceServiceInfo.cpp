/* This is a request handler to provide service-info for refget standards. 
The request is initialized with nothing as the response is static.
*/

#include "requests.hpp"

RequestSequenceServiceInfo::RequestSequenceServiceInfo():
Request(documentType::ga4ghSeqServiceInfo)
{

}

RequestSequenceServiceInfo::~RequestSequenceServiceInfo(){
  stopProcessingThread();
}

void RequestSequenceServiceInfo::process(){
  Document doc;

  doc = DocumentSequenceServiceInfo();

  doc.ga4ghSeqServiceInfo().algorithms.push_back("trunc512");

  doc.ga4ghSeqServiceInfo().supported_api_versions.push_back("1.0");
  
  setResponse(doc);
}