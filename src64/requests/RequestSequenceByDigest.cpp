#include "requests.hpp"

RequestSequenceByDigest::RequestSequenceByDigest(std::string const & pId, std::string const & cstart, std::string const & cend)
:Request(documentType::ga4ghSeqServiceInfo), sequenceDigestId(pId), cstart(cstart), cend(cend)
{
  // OutputFormatter formatter(documentType::ga4ghSeqServiceInfo);
  // formatter.setFormat(responseFormat::txt, "txt");
};

RequestSequenceByDigest::~RequestSequenceByDigest(){
  stopProcessingThread();
};

void RequestSequenceByDigest::process(){
/* use the reference sequence pointer to find out the sequence, construct response and send it back */
  // boost::lexical_cast<unsigned>(cstart), 
  // boost::lexical_cast<unsigned>(cend)
  std::string const sequence_to_send = referencesDb->getSubSequenceFromDigest(sequenceDigestId, boost::lexical_cast<unsigned>(cstart), boost::lexical_cast<unsigned>(cend));
  if((boost::lexical_cast<unsigned>(cend) - boost::lexical_cast<unsigned>(cstart)) < 2000){
    setResponse(sequence_to_send);
  } else {
    std::runtime_error("Current implementation do not support to retrieve sequence of more than 2000 residues");
  }
  // std::cerr << sequence_to_send << std::endl;
  // Document doc;
};
