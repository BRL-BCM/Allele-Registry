#ifndef REQUESTS_PROTEINVARIANTSTOGENOMIC_HPP_
#define REQUESTS_PROTEINVARIANTSTOGENOMIC_HPP_

#include "../core/variants.hpp"

std::vector< std::vector<PlainVariant> > searchForMatchingTranscriptVariants
	( ReferenceId const transcriptId
	, unsigned const transcriptStartCodon
	, std::string const & transcriptSequence
	, std::vector<PlainSequenceModification> & transcriptVariants  // the table is sorted
	, std::string const proteinSequence
	, std::vector<PlainVariant> const & proteinVariants
	);



#endif /* REQUESTS_PROTEINVARIANTSTOGENOMIC_HPP_ */
