#include "exceptions.hpp"
#include <boost/lexical_cast.hpp>


std::string toString(errorType type)
{
	switch (type) {
		case errorType::NoErrors                  : return "NoErrors";
		case errorType::HgvsParsingError          : return "HgvsParsingError";
		case errorType::IncorrectHgvsPosition     : return "IncorrectHgvsPosition";
		case errorType::IncorrectReferenceAllele  : return "IncorrectReferenceAllele";
		case errorType::NoConsistentAlignment     : return "NoConsistentAlignment";
		case errorType::UnknownCDS                : return "UnknownCDS";
		case errorType::UnknownGene               : return "UnknownGene";
		case errorType::UnknownGeneName           : return "UnknownGeneName";
		case errorType::UnknownReferenceSequence  : return "UnknownReferenceSequence";
		case errorType::InternalServerError       : return "InternalServerError";
		case errorType::RequestTerminated         : return "RequestTerminated";
		case errorType::VcfParsingError           : return "VcfParsingError";
		case errorType::LineParsingError          : return "LineParsingError";
		case errorType::AuthorizationError        : return "AuthorizationError";
		case errorType::NotFound                  : return "NotFound";
		case errorType::RequestTooLarge           : return "RequestTooLarge";
		case errorType::VariationTooLong          : return "VariationTooLong";
		case errorType::ProteinVariantTooLong     : return "ProteinVariantTooLong";
		case errorType::CoordinateOutsideReference: return "CoordinateOutsideReference";
		case errorType::ComplexAlleleWithOverlappingSimpleAlleles: return "ComplexAlleleWithOverlappingSimpleAlleles";
		case errorType::IncorrectRequest          : return "IncorrectRequest";
		case errorType::UnknownFormatOfMyVariantInfoIdentifier : return "UnknownFormatOfMyVariantInfoIdentifier";
		case errorType::UnknownFormatOfExACOrGnomADIdentifier : return "UnknownFormatOfExACOrGnomADIdentifier";
		case errorType::UnknownFormatOfCOSMICIdentifier : return "UnknownFormatOfCOSMICIdentifier";
		case errorType::SystemInReadOnlyMode            : return "SystemInReadOnlyMode";
		case errorType::ExternalSourceDoesNotExists     : return "ExternalSourceDoesNotExists";
		case errorType::IncorrectLinksParameters        : return "IncorrectLinksParameters";
	}
	return "UnknownErrorType";
}

std::string description(errorType type)
{
	switch (type) {
		case errorType::NoErrors                  : return "No error type was set";
		case errorType::HgvsParsingError          : return "Given HGVS expressions cannot be parsed. It is incorrect or not supported.";
		case errorType::IncorrectHgvsPosition     : return "Position given in HGVS expression is incorrect.";
		case errorType::IncorrectReferenceAllele  : return "Given allele from reference sequence is incorrect. It does not match actual sequence at given position.";
		case errorType::NoConsistentAlignment     : return "Given allele cannot be mapped in consistent way to reference genome.";
		case errorType::UnknownCDS                : return "The boundary of coding sequence for given transcript is not known.";
		case errorType::UnknownGene               : return "Given reference sequence is not assigned to any gene.";
		case errorType::UnknownGeneName           : return "Unknown gene symbol.";
		case errorType::UnknownReferenceSequence  : return "Given reference sequence is not known.";
		case errorType::InternalServerError       : return "Internal error occurred. Please, report it as an error.";
		case errorType::VcfParsingError           : return "Sent VCF file cannot be parsed. It is incorrect or contains unsupported features.";
		case errorType::LineParsingError          : return "A line in sent payload cannot be parsed.";
		case errorType::AuthorizationError        : return "Access denied because of authorization failure.";
		case errorType::NotFound                  : return "The system does not contain any data about requested resource.";
		case errorType::RequestTooLarge           : return "The request size cannot exceed 2000 records. It means that the 'limit' parameter in queries must be set to value <= 2000. Also size of any bulk requests cannot exceed 2000 alleles.";
		case errorType::VariationTooLong          : return "Variation given on the input is too long. There is 10000 bp limit for variation's length.";
		case errorType::ProteinVariantTooLong     : return "Protein variant is too long (max 7 amino-acids are allowed)";
		case errorType::CoordinateOutsideReference: return "Coordinate outside reference";
		case errorType::ComplexAlleleWithOverlappingSimpleAlleles: return "Given definition of complex allele contains overlapping simple alleles";
		case errorType::IncorrectRequest          : return "HTTP request is incorrect";
		case errorType::RequestTerminated         : return "HTTP request has been terminated by client";
		case errorType::UnknownFormatOfMyVariantInfoIdentifier : return "Given MyVariant.Info identifier could not be parsed. It is incorrect or currently unsupported.";
		case errorType::UnknownFormatOfExACOrGnomADIdentifier : return "Given ExAC or gnomAD identifier could not be parsed. It is incorrect or currently unsupported.";
		case errorType::UnknownFormatOfCOSMICIdentifier : return "Given COSMIC identifier could not be parsed. It is incorrect or currently unsupported.";
		case errorType::SystemInReadOnlyMode            : return "The system is currently in read-only mode and all PUT and DELETE requests are dropped. This is a temporary situation and it is caused by on-going system maintenance.";
		case errorType::ExternalSourceDoesNotExists     : return "External source with given name does not exists.";
		case errorType::IncorrectLinksParameters        : return "Given link's parameters do not match parameters' format defined in link's external source.";
	}
	return "Unknown error type.";
}


//ExceptionHgvsParsingError::ExceptionHgvsParsingError(std::string const & message) : std::runtime_error(""), fMessage(message), fHgvs(""), fPosition(0), fBuf(nullptr) {}
//ExceptionHgvsParsingError::~ExceptionHgvsParsingError() { delete [] fBuf; }
//void ExceptionHgvsParsingError::setContext(std::string const & hgvs, unsigned position) { fHgvs = hgvs; fPosition = position; delete [] fBuf; fBuf = nullptr; }
//const char* ExceptionHgvsParsingError::what() const noexcept
//{
//	if (fBuf == nullptr) {
//		std::string msg = "HGVS parse error: " + fMessage + ".";
//		if (fHgvs != "") msg += " Error occured for " + fHgvs + " at position " + boost::lexical_cast<std::string>(fPosition) + ".";
//		fBuf = new char[msg.size() + 1];
//		strcpy(fBuf, msg.c_str());
//	}
//	return fBuf;
//}


//ExceptionIncorrectHgvsPosition::ExceptionIncorrectHgvsPosition(std::string const & message) : std::runtime_error(""), fMessage(message), fHgvs(""), fBuf(nullptr) {}
//ExceptionIncorrectHgvsPosition::~ExceptionIncorrectHgvsPosition() { delete [] fBuf; }
//void ExceptionIncorrectHgvsPosition::setContext(std::string const & hgvs) { fHgvs = hgvs; delete [] fBuf; fBuf = nullptr; }
//const char* ExceptionIncorrectHgvsPosition::what() const noexcept
//{
//	if (fBuf == nullptr) {
//		std::string msg = "HGVS parse error: " + fMessage + ".";
//		if (fHgvs != "") msg += " Error occured for " + fHgvs + ".";
//		fBuf = new char[msg.size() + 1];
//		strcpy(fBuf, msg.c_str());
//	}
//	return fBuf;
//}
