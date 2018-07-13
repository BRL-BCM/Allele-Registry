#ifndef SMITHWATERMAN_HPP_
#define SMITHWATERMAN_HPP_

#include "../core/variants.hpp"


// return true <=> the protein variant can be created
// results contains variant def <=> the function returned true
// results is cleared upon entry
bool tryDescribeAsProteinVariant
	( std::string const & originalSequence
	, std::string const & modifiedSequence
	, unsigned const offset
	, std::vector<PlainSequenceModification> & results
	);



#endif /* SMITHWATERMAN_HPP_ */
