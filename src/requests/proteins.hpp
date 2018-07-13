#ifndef PROTEINS_HPP_
#define PROTEINS_HPP_

#include "../referencesDatabase/referencesDatabase.hpp"
#include "../core/variants.hpp"

// true if can be represented as proteinVar, false otherwise
bool calculateProteinVariation
    ( ReferencesDatabase const * refDb
    , NormalizedGenomicVariant const & transcriptVar
    , std::string & outGeneralHgvs
    , std::string & outCanonicalHgvs
    );


#endif /* PROTEINS_HPP_ */
