#ifndef CANONICALIZATION_HPP_
#define CANONICALIZATION_HPP_

#include "hgvs.hpp"

// ================================= functions

NormalizedGenomicVariant canonicalizeGenomic(ReferencesDatabase const * refDb, PlainVariant const & variant);
NormalizedProteinVariant canonicalizeProtein(ReferencesDatabase const * refDb, PlainVariant const & variant);

NormalizedGenomicVariant canonicalize(ReferencesDatabase const * refDb, HgvsGenomicVariant const & variant);
NormalizedProteinVariant canonicalize(ReferencesDatabase const * refDb, HgvsProteinVariant const & variant);



#endif /* CANONICALIZATION_HPP_ */
