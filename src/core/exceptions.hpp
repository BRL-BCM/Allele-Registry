#ifndef EXCEPTIONS_HPP_
#define EXCEPTIONS_HPP_

#include "globals.hpp"
#include "textLabels.hpp"
#include <map>
#include <boost/lexical_cast.hpp>

enum errorType : unsigned
{
	  NoErrors = 0
	, NotFound
	, RequestTerminated
	, AuthorizationError
	, RequestTooLarge
	, IncorrectRequest
	, InternalServerError
	, VariationTooLong
	, ProteinVariantTooLong
	, UnknownCDS
	, UnknownGene
	, UnknownGeneName
	, UnknownReferenceSequence
	, NoConsistentAlignment
	, CoordinateOutsideReference
	, HgvsParsingError
	, IncorrectHgvsPosition
	, IncorrectReferenceAllele
	, VcfParsingError
	, LineParsingError
	, ComplexAlleleWithOverlappingSimpleAlleles
	, UnknownFormatOfMyVariantInfoIdentifier
	, UnknownFormatOfExACOrGnomADIdentifier
	, UnknownFormatOfCOSMICIdentifier
	, SystemInReadOnlyMode
	, ExternalSourceDoesNotExists
	, IncorrectLinksParameters
};

std::string toString(errorType);
std::string description(errorType);


class ExceptionBase : public std::runtime_error
{
protected:
	errorType fType;
	std::map<label,std::string> fFields;
	ExceptionBase(errorType type) : std::runtime_error(toString(type)), fType(type) {}
	ExceptionBase(errorType type, std::string const & message)
	: std::runtime_error(message), fType(type) {}
public:
	errorType type() const { return fType; }
	std::map<label,std::string> const & fields() const { return fFields; }
//	void setField(std::string const & name, std::string const & value);
};


class ExceptionHgvsParsingError : public ExceptionBase {
//private:
//	std::string fMessage;
//	std::string fHgvs;
//	unsigned fPosition;
//	mutable char * fBuf;
public:
	ExceptionHgvsParsingError(std::string const & message) : ExceptionBase(errorType::HgvsParsingError, message) {}
//	virtual ~ExceptionHgvsParsingError();
	void setContext(std::string const & hgvs, unsigned position)
	{
		fFields[label::hgvs] = hgvs;
		fFields[label::position] = boost::lexical_cast<std::string>(position);
	}
//	std::string message() const noexcept { return fMessage; }
//	std::string hgvs() const noexcept { return fHgvs; }
//	unsigned position() const noexcept { return fPosition; }
//	virtual const char* what() const noexcept;
};


class ExceptionIncorrectHgvsPosition : public ExceptionBase {
//private:
//	std::string fMessage;
//	std::string fHgvs;
//	mutable char * fBuf;
public:
	ExceptionIncorrectHgvsPosition(std::string const & message) : ExceptionBase(errorType::IncorrectHgvsPosition, message) {}
//	~ExceptionIncorrectHgvsPosition();
	void setContext(std::string const & hgvs) { fFields[label::hgvs] = hgvs; }
//	std::string message() const noexcept { return fMessage; }
//	std::string hgvs() const noexcept { return fHgvs; }
//	virtual const char* what() const noexcept;
};


class ExceptionVariationTooLong : public ExceptionBase {
//private:
//	unsigned fSize;
//	std::string fHgvs;
public:
	ExceptionVariationTooLong(unsigned size, std::string const & hgvs)
	: ExceptionBase(errorType::VariationTooLong)
	{
		fFields[label::length] = boost::lexical_cast<std::string>(size);
		fFields[label::hgvs] = hgvs;
	}
};


class ExceptionProteinVariantTooLong : public ExceptionBase {
//private:
//	unsigned fSize;
//	std::string fHgvs;
public:
	ExceptionProteinVariantTooLong(unsigned size)
	: ExceptionBase(errorType::ProteinVariantTooLong)
	{
		fFields[label::length] = boost::lexical_cast<std::string>(size);
	}
};


class ExceptionIncorrectReferenceAllele : public ExceptionBase
{
//private:
//	std::string fSeqName;
//	SplicedRegionCoordinates fRegion;
//	std::string fGivenAllele;
//	std::string fActualAllele;
public:
	ExceptionIncorrectReferenceAllele
		( std::string const & name
		, SplicedRegionCoordinates const & region
		, std::string const & givenAllele
		, std::string const & actualAllele )
		: ExceptionBase(errorType::IncorrectReferenceAllele,"Reference allele does not match for " + name + region.toString() + ", given=" + givenAllele + ", found=" + actualAllele + ".")
	{
		fFields[label::referenceSequence] = name;
		fFields[label::region] = region.toString();
		fFields[label::givenAllele] = givenAllele;
		fFields[label::actualAllele] = actualAllele;
	}
//	std::string sequenceName() const noexcept { return fSeqName; }
//	SplicedRegionCoordinates region() const noexcept { return fRegion; }
//	std::string givenAllele() const noexcept { return fGivenAllele; }
//	std::string actualAllele() const noexcept { return fActualAllele; }
};


class ExceptionNoConsistentAlignment : public ExceptionBase
{
//private:
//	std::string fSeqName;
//	SplicedRegionCoordinates fRegion;
//	std::string fMessage;
public:
	ExceptionNoConsistentAlignment(std::string const & name, SplicedRegionCoordinates const & region, std::string const & msg = "")
	: ExceptionBase(errorType::NoConsistentAlignment, "Cannot align " + name + " " + region.toString() + ". " + msg)
	{
		fFields[label::sequenceName] = name;
		fFields[label::region] = region.toString();
	}
	ExceptionNoConsistentAlignment(std::string const & name, RegionCoordinates const & region, std::string const & msg = "")
	: ExceptionBase(errorType::NoConsistentAlignment, "Cannot align " + name + " " + region.toString() + ". " + msg)
	{
		fFields[label::sequenceName] = name;
		fFields[label::region] = region.toString();
	}
	ExceptionNoConsistentAlignment(std::string const & name, std::string const & msg)
	: ExceptionBase(errorType::NoConsistentAlignment, "Cannot align " + name + ". " + msg)
	{
		fFields[label::sequenceName] = name;
	}
//	std::string sequenceName() const noexcept { return fSeqName; }
//	SplicedRegionCoordinates region() const noexcept { return fRegion; }
//	std::string message() const noexcept { return fMessage; }
};


class ExceptionUnknownReferenceSequence : public ExceptionBase
{
//private:
//	std::string fName;
//	std::string fMessage;
public:
	ExceptionUnknownReferenceSequence(std::string const & name, std::string const & msg = "")
	: ExceptionBase(errorType::UnknownReferenceSequence, "The reference sequence named " + name + " is not known. " + msg)
	{
		fFields[label::sequenceName] = name;
	}
//	std::string name() const noexcept { return fName; }
//	std::string message() const noexcept { return fMessage; }
};


class ExceptionUnknownCDS : public ExceptionBase
{
//private:
//	std::string fName;
//	std::string fMessage;
public:
	ExceptionUnknownCDS(std::string const & name, std::string const & msg = "")
	: ExceptionBase(errorType::UnknownCDS, "The CDS for reference sequence named " + name + " is not known. " + msg)
	{
		fFields[label::sequenceName] = name;
	}
//	std::string name() const noexcept { return fName; }
//	std::string message() const noexcept { return fMessage; }
};


class ExceptionUnknownGene : public ExceptionBase
{
//private:
//	std::string fName;
//	std::string fMessage;
public:
	ExceptionUnknownGene(std::string const & name, std::string const & msg = "")
	: ExceptionBase(errorType::UnknownGene, "The gene for reference sequence named " + name + " is not known. " + msg)
	{
		fFields[label::sequenceName] = name;
	}
//	std::string name() const noexcept { return fName; }
//	std::string message() const noexcept { return fMessage; }
};


class ExceptionUnknownGeneName : public ExceptionBase
{
//private:
//	std::string fName;
//	std::string fMessage;
public:
	ExceptionUnknownGeneName(std::string const & name)
	: ExceptionBase(errorType::UnknownGeneName, "The gene symbol '" + name + "' is not known.")
	{
		fFields[label::geneSymbol] = name;
	}
//	std::string name() const noexcept { return fName; }
//	std::string message() const noexcept { return fMessage; }
};


class ExceptionCoordinateOutsideReference : public ExceptionBase
{
public:
	ExceptionCoordinateOutsideReference()
	: ExceptionBase(errorType::CoordinateOutsideReference) {}
};


class ExceptionComplexAlleleWithOverlappingSimpleAlleles : public ExceptionBase
{
public:
	ExceptionComplexAlleleWithOverlappingSimpleAlleles()
	: ExceptionBase(errorType::ComplexAlleleWithOverlappingSimpleAlleles) {}
};


class ExceptionIncorrectRequest : public ExceptionBase
{
public:
	ExceptionIncorrectRequest(std::string const & message)
	: ExceptionBase(errorType::IncorrectRequest, message) {}
};


class ExceptionVcfParsingError : public ExceptionBase
{
public:
	ExceptionVcfParsingError(unsigned lineNumber, std::string const & message)
	: ExceptionBase(errorType::VcfParsingError
		, "Error in VCF file in line " + boost::lexical_cast<std::string>(lineNumber) + ": " + message) {}
};


class ExceptionLineParsingError : public ExceptionBase
{
public:
	ExceptionLineParsingError(std::string const & message)
	: ExceptionBase(errorType::LineParsingError, message) {}
};


class ExceptionRequestTerminated : public ExceptionBase
{
public:
	ExceptionRequestTerminated()
	: ExceptionBase(errorType::RequestTerminated) {}
};


class ExceptionAuthorizationError : public ExceptionBase
{
public:
	ExceptionAuthorizationError(std::string const & message)
	: ExceptionBase(errorType::AuthorizationError,message) {}
};


class ExceptionUnknownFormatOfMyVariantInfoIdentifier : public ExceptionBase
{
public:
	ExceptionUnknownFormatOfMyVariantInfoIdentifier(std::string const & message)
	: ExceptionBase(UnknownFormatOfMyVariantInfoIdentifier, message) {}
};


class ExceptionUnknownFormatOfExACOrGnomADIdentifier : public ExceptionBase
{
public:
	ExceptionUnknownFormatOfExACOrGnomADIdentifier(std::string const & message)
	: ExceptionBase(UnknownFormatOfExACOrGnomADIdentifier, message) {}
};


class ExceptionUnknownFormatOfCOSMICIdentifier : public ExceptionBase
{
public:
	ExceptionUnknownFormatOfCOSMICIdentifier(std::string const & message)
	: ExceptionBase(UnknownFormatOfCOSMICIdentifier, message) {}
};


class ExceptionSystemInReadOnlyMode : public ExceptionBase
{
public:
	ExceptionSystemInReadOnlyMode()
	: ExceptionBase(SystemInReadOnlyMode) {}
};


class ExceptionExternalSourceDoesNotExists : public ExceptionBase
{
public:
	ExceptionExternalSourceDoesNotExists() : ExceptionBase(ExternalSourceDoesNotExists) {}
};


class ExceptionIncorrectLinksParameters : public ExceptionBase
{
public:
	ExceptionIncorrectLinksParameters(std::string const & message) : ExceptionBase(IncorrectLinksParameters,message) {}
};


#endif /* EXCEPTIONS_HPP_ */
