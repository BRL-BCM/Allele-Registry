#ifndef IDENTIFIERSTOOLS_HPP_
#define IDENTIFIERSTOOLS_HPP_

#include "../core/variants.hpp"
#include "../referencesDatabase/referencesDatabase.hpp"


void parseIdentifierMyVariantInfo
		( ReferencesDatabase    const * refDb
		, ReferenceGenome               refGenome
		, std::string           const & id
		, IdentifierWellDefined       & outIdentifier
		, NormalizedGenomicVariant           & outVariant
		);

void parseIdentifierExACgnomAD
		( ReferencesDatabase    const * refDb
		, bool                          isExAC        // true - ExAC, false -gnomAD
		, std::string           const & id
		, IdentifierWellDefined       & outIdentifier
		, NormalizedGenomicVariant           & outVariant
		);


std::string buildIdentifierMyVariantInfo
		( ReferencesDatabase    const * refDb
		, IdentifierWellDefined const & identifier
		, NormalizedGenomicVariant     const & variantDef
		);

void buildIdentifierExACgnomAD
		( ReferencesDatabase    const * refDb
		, IdentifierWellDefined const & identifier
		, NormalizedGenomicVariant     const & variantDef
		, std::string                 & outChr
		, std::string                 & outPos
		, std::string                 & outRef
		, std::string                 & outAlt
		);


#endif /* IDENTIFIERSTOOLS_HPP_ */
