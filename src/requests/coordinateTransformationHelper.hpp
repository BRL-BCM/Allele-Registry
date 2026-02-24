/*
Header file for key types and genomicCoordinateTransformer class:
Author: Ronak Patel.
*/
#ifndef COORDINATE_TRNAFORMATION_HELPER_HPP_
#define COORDINATE_TRNAFORMATION_HELPER_HPP_
#include "requests.hpp"
#include "hgvs.hpp"
#include "canonicalization.hpp"
#include "parsers.hpp"

// text input is a simple type that takes assembly, chr, cstart, cend without using any logic.
struct textInput {
	std::string assembly;
	std::string chr;
	unsigned int cstart;
	unsigned int cend;
};

// Piotr implemented nice enums and RegionCoordinates class for storing regions. To use the methods on top of that, we need to conver textInput provided to Acse (accronym for Assembly, Coordinate, Start, End)
struct Acse {
	ReferenceGenome  assembly;
	Chromosome  chr;
	RegionCoordinates region;
};

// Key class for handling transformation requests.
class genomicCoordinateTransformer {
private:
	ReferencesDatabase const * seqDb;
	Acse inputCoordinates;
	Acse grch38Coordinates;
	void getProjectionsToGrch38();
	void getProjectionsToOtherAssembly();
public:
	genomicCoordinateTransformer(ReferencesDatabase const *, textInput const &);
	virtual ~genomicCoordinateTransformer();
	std::vector<Acse> transformation;
	bool mappableToMainAssembly;
};

#endif