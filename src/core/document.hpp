#ifndef DOCUMENT_HPP_
#define DOCUMENT_HPP_


#include "variants.hpp"
#include "exceptions.hpp"
// This included is added by Ronak to support the ReferencesDatabase Type access for toVrString
#include "../referencesDatabase/referencesDatabase.hpp"
// For digest computation
#include "commonDigestUtils.hpp"

struct DocumentError
{
	errorType type = NoErrors;
	std::string message = "";
	std::map<label,std::string> fields;
	static DocumentError createFromCurrentException();
	DocumentError() {}
	DocumentError(errorType pType) : type(pType) {}
	DocumentError(std::string const & msg) : type(errorType::InternalServerError), message(msg) {}
	inline bool operator<(DocumentError const & doc) const
	{
		if (type != doc.type) return (type < doc.type);
		return (message < doc.message);
	}
	std::string toString() const;
};

struct DocumentActiveGenomicVariant
{
	NormalizedGenomicVariant mainDefinition;
	CanonicalId caId;
	Identifiers identifiers;
	std::vector<VariantDetailsGenomeBuild> definitionsOnGenomeBuilds;
	std::vector<VariantDetailsGeneRegion > definitionsOnGenesRegions;
	std::vector<VariantDetailsTranscript > definitionsOnTranscripts;
	std::string jsonExternalSources;
	DocumentActiveGenomicVariant() {}
	DocumentActiveGenomicVariant(NormalizedGenomicVariant const & d) : mainDefinition(d) {}
	inline bool operator<(DocumentActiveGenomicVariant const & doc) const
	{
		return (mainDefinition < doc.mainDefinition);
	}
	std::string toString() const;
	// Function added to get VrString of representation
	// The vector of reference sequence identifier is provided as parameter to selectively generate vr representation for those sequences
	std::vector<std::string> toVrString(ReferencesDatabase const * refDb, std::vector<ReferenceId> const & reference_sequence_ids) const;
};

struct DocumentActiveProteinVariant
{
	NormalizedProteinVariant mainDefinition;
	CanonicalId caId;
	Identifiers identifiers;
	VariantDetailsProtein definitionOnProtein;
	std::string jsonExternalSources;
	DocumentActiveProteinVariant() {}
	DocumentActiveProteinVariant(NormalizedProteinVariant const & d) : mainDefinition(d) {}
	inline bool operator<(DocumentActiveProteinVariant const & doc) const
	{
		return (mainDefinition < doc.mainDefinition);
	}
	std::string toString() const;
};

struct DocumentInactiveVariant
{
	CanonicalId caId;
	std::vector<std::pair<ReferenceId,uint32_t>> newCaIds;
	inline bool operator<(DocumentInactiveVariant const & doc) const { return (caId < doc.caId); }
	std::string toString() const;
};

struct DocumentReference
{
	uint32_t    rsId = 0;
	std::string ensemblId = "";
	std::string ncbiId = "";
	bool transcript = false;
	uint32_t    gnId = 0;
	ReferenceGenome genomeBuild = ReferenceGenome::rgUnknown;
	Chromosome chromosome = Chromosome::chrUnknown;
	std::string sequence = "";
	std::string splicedSequence = "";
	RegionCoordinates CDS;
	inline bool operator<(DocumentReference const & doc) const
	{
		return (rsId < doc.rsId);
	}
	std::string toString() const;
};

struct DocumentGene
{
	uint32_t    gnId = 0;
	uint32_t    ncbiId = 0;
	std::string hgncName = "";
	std::string hgncSymbol = "";
	std::string prefRefSeq = "";
	inline bool operator<(DocumentGene const & doc) const
	{
		return (gnId < doc.gnId);
	}
	std::string toString() const;
};

// Document Coordinates defines the document that is sent back when coordinate transformation is requested
struct DocumentCoordinates 
{
    uint32_t    grch38Start = 0;
    uint32_t    grch38End = 0;
    std::string grch38Assembly = "";
    std::string grch38Chr = "";

    uint32_t    grch37Start = 0;
    uint32_t    grch37End = 0;
    std::string grch37Assembly = "";
    std::string grch37Chr = "";

    uint32_t    ncbi36Start = 0;
    uint32_t    ncbi36End = 0;
    std::string ncbi36Assembly = "";
    std::string ncbi36Chr = "";

    // std::cerr << "Inside DocumentCorrdinatoes in document.hpp" << std::endl;

    inline bool operator<(DocumentCoordinates const & doc) const
    {
        return (grch38Assembly < doc.grch38Assembly);
    }
    std::string toString() const;
};

struct DocumentSequenceServiceInfo
{
	bool circular_supported = false;

	// Hard coded limit of sequence length
	uint32_t subsequence_limit = 2000;

	std::vector<std::string> algorithms;

	std::vector<std::string> supported_api_versions;

	inline bool operator<(DocumentSequenceServiceInfo const & doc) const
	{
		return (false);
	}

	std::string toString() const;
};

struct DocumentGa4ghSeqMetadata
{
	std::vector<std::string> aliases;
	unsigned seqLength;
	std::string trunc512;

	inline bool operator<(DocumentGa4ghSeqMetadata const & doc) const
	{
		return (false);
	}

	std::string toString() const;
};

enum documentType
{
	  null = 0
	, error = 1
	, activeGenomicVariant = 2
	, activeProteinVariant = 3
	, inactiveVariant
	, reference
	, gene
	, coordinateTransform
	, ga4ghSeqServiceInfo
	, ga4ghSeqMetadata
};

class Document
{
private:
	documentType fType = documentType::null;
	union dataSpace {
		uint64_t zero = 0;
		DocumentError           error;
		DocumentActiveGenomicVariant activeGenomicVariant;
		DocumentActiveProteinVariant activeProteinVariant;
		DocumentInactiveVariant inactiveVariant;
		DocumentReference       reference;
		DocumentGene            gene;
		DocumentCoordinates    coordinateTransform;
		// This is the document that will provide service info based on GA4GH refget
		DocumentSequenceServiceInfo ga4ghSeqServiceInfo;
		// Document to send metadata of RefGet sequence
		DocumentGa4ghSeqMetadata ga4ghSeqMetadata;
		dataSpace() {}
		~dataSpace() {}
	} data;
	inline void init(Document const & doc)
	{
		fType = doc.fType;
		switch (fType) {
		case documentType::null           : /* do nothing */ break;
		case documentType::error          : new (&data.error          ) DocumentError          (doc.data.error          ); break;
		case documentType::activeGenomicVariant: new (&data.activeGenomicVariant) DocumentActiveGenomicVariant(doc.data.activeGenomicVariant); break;
		case documentType::activeProteinVariant: new (&data.activeProteinVariant) DocumentActiveProteinVariant(doc.data.activeProteinVariant); break;
		case documentType::inactiveVariant: new (&data.inactiveVariant) DocumentInactiveVariant(doc.data.inactiveVariant); break;
		case documentType::reference      : new (&data.reference      ) DocumentReference      (doc.data.reference      ); break;
		case documentType::gene           : new (&data.gene           ) DocumentGene           (doc.data.gene           ); break;
		case documentType::coordinateTransform : new (&data.coordinateTransform) DocumentCoordinates (doc.data.coordinateTransform) ; break;
		case documentType::ga4ghSeqServiceInfo : new (&data.ga4ghSeqServiceInfo) DocumentSequenceServiceInfo (doc.data.ga4ghSeqServiceInfo) ; break;
		case documentType::ga4ghSeqMetadata : new (&data.ga4ghSeqMetadata) DocumentGa4ghSeqMetadata (doc.data.ga4ghSeqMetadata) ; break;
		}
	}
	inline void init(Document && doc)
	{
		fType = doc.fType;
		switch (fType) {
		case documentType::null           : /* do nothing */ break;
		case documentType::error          : new (&data.error          ) DocumentError          (doc.data.error          ); break;
		case documentType::activeGenomicVariant  : new (&data.activeGenomicVariant  ) DocumentActiveGenomicVariant  (doc.data.activeGenomicVariant  ); break;
		case documentType::activeProteinVariant  : new (&data.activeProteinVariant  ) DocumentActiveProteinVariant  (doc.data.activeProteinVariant  ); break;
		case documentType::inactiveVariant: new (&data.inactiveVariant) DocumentInactiveVariant(doc.data.inactiveVariant); break;
		case documentType::reference      : new (&data.reference      ) DocumentReference      (doc.data.reference      ); break;
		case documentType::gene           : new (&data.gene           ) DocumentGene           (doc.data.gene           ); break;
		case documentType::coordinateTransform : new (&data.coordinateTransform) DocumentCoordinates (doc.data.coordinateTransform) ; break;
		case documentType::ga4ghSeqServiceInfo : new (&data.ga4ghSeqServiceInfo) DocumentSequenceServiceInfo (doc.data.ga4ghSeqServiceInfo) ; break;
		case documentType::ga4ghSeqMetadata : new (&data.ga4ghSeqMetadata) DocumentGa4ghSeqMetadata (doc.data.ga4ghSeqMetadata) ; break;
		}
	}
	inline void switchTo(documentType newType)
	{
		switch (fType) {
			case documentType::null           : /* do nothing */ break;
			case documentType::error          : data.error          .~DocumentError()               ; break;
			case documentType::activeGenomicVariant  : data.activeGenomicVariant  .~DocumentActiveGenomicVariant(); break;
			case documentType::activeProteinVariant  : data.activeProteinVariant  .~DocumentActiveProteinVariant(); break;
			case documentType::inactiveVariant: data.inactiveVariant.~DocumentInactiveVariant()     ; break;
			case documentType::reference      : data.reference      .~DocumentReference()           ; break;
			case documentType::gene           : data.gene           .~DocumentGene()                ; break;
			case documentType::coordinateTransform : data.coordinateTransform.~DocumentCoordinates() ; break;
			case documentType::ga4ghSeqServiceInfo : data.ga4ghSeqServiceInfo.~DocumentSequenceServiceInfo() ; break;
			case documentType::ga4ghSeqMetadata : data.ga4ghSeqMetadata.~DocumentGa4ghSeqMetadata() ; break;
		}
		fType = newType;
		switch (fType) {
			case documentType::null           : /* do nothing */ break;
			case documentType::error          : new (&data.error          ) DocumentError          ; break;
			case documentType::activeGenomicVariant  : new (&data.activeGenomicVariant  ) DocumentActiveGenomicVariant  ; break;
			case documentType::activeProteinVariant  : new (&data.activeProteinVariant  ) DocumentActiveProteinVariant  ; break;
			case documentType::inactiveVariant: new (&data.inactiveVariant) DocumentInactiveVariant; break;
			case documentType::reference      : new (&data.reference      ) DocumentReference      ; break;
			case documentType::gene           : new (&data.gene           ) DocumentGene           ; break;
			case documentType::coordinateTransform : new (&data.coordinateTransform) DocumentCoordinates ; break;
			case documentType::ga4ghSeqServiceInfo : new (&data.ga4ghSeqServiceInfo) DocumentSequenceServiceInfo ; break;
			case documentType::ga4ghSeqMetadata : new (&data.ga4ghSeqMetadata) DocumentGa4ghSeqMetadata ; break;
		}
	}
	inline void check(documentType givenType) const
	{
		if (givenType != fType) {
			throw std::logic_error("Wrong doc type");
		}
	}
public:
	static Document const null;
	Document() = default;
	explicit Document(documentType pType) { switchTo(pType); }
	~Document() { switchTo(documentType::null); }
	// copy constructors
	Document(Document                const & doc) { init(doc); }
	Document(Document                     && doc) { init(doc); }
	Document(DocumentError           const & doc) { (*this) = doc; }
	Document(DocumentError                && doc) { (*this) = doc; }
	Document(DocumentActiveGenomicVariant   const & doc) { (*this) = doc; }
	Document(DocumentActiveGenomicVariant        && doc) { (*this) = doc; }
	Document(DocumentActiveProteinVariant   const & doc) { (*this) = doc; }
	Document(DocumentActiveProteinVariant        && doc) { (*this) = doc; }
	Document(DocumentInactiveVariant const & doc) { (*this) = doc; }
	Document(DocumentInactiveVariant      && doc) { (*this) = doc; }
	Document(DocumentReference       const & doc) { (*this) = doc; }
	Document(DocumentReference            && doc) { (*this) = doc; }
	Document(DocumentGene            const & doc) { (*this) = doc; }
	Document(DocumentGene                 && doc) { (*this) = doc; }
	Document(DocumentCoordinates            const & doc) { (*this) = doc; }
	Document(DocumentCoordinates                 && doc) { (*this) = doc; }
	Document(DocumentSequenceServiceInfo            const & doc) { (*this) = doc; }
	Document(DocumentSequenceServiceInfo                 && doc) { (*this) = doc; }
	Document(DocumentGa4ghSeqMetadata            const & doc) { (*this) = doc; }
	Document(DocumentGa4ghSeqMetadata                 && doc) { (*this) = doc; }
	inline documentType type() const { return fType; }
	// assignment operator
	inline Document & operator=(Document                const & doc) { switchTo(documentType::null           ); init(doc)                 ; return (*this); }
	inline Document & operator=(Document                     && doc) { switchTo(documentType::null           ); init(doc)                 ; return (*this); }
	inline Document & operator=(DocumentError           const & doc) { switchTo(documentType::error          ); data.error           = doc; return (*this); }
	inline Document & operator=(DocumentError                && doc) { switchTo(documentType::error          ); data.error           = doc; return (*this); }
	inline Document & operator=(DocumentActiveGenomicVariant const & doc) { switchTo(documentType::activeGenomicVariant  ); data.activeGenomicVariant   = doc; return (*this); }
	inline Document & operator=(DocumentActiveGenomicVariant      && doc) { switchTo(documentType::activeGenomicVariant  ); data.activeGenomicVariant   = doc; return (*this); }
	inline Document & operator=(DocumentActiveProteinVariant const & doc) { switchTo(documentType::activeProteinVariant  ); data.activeProteinVariant   = doc; return (*this); }
	inline Document & operator=(DocumentActiveProteinVariant      && doc) { switchTo(documentType::activeProteinVariant  ); data.activeProteinVariant   = doc; return (*this); }
	inline Document & operator=(DocumentInactiveVariant const & doc) { switchTo(documentType::inactiveVariant); data.inactiveVariant = doc; return (*this); }
	inline Document & operator=(DocumentInactiveVariant      && doc) { switchTo(documentType::inactiveVariant); data.inactiveVariant = doc; return (*this); }
	inline Document & operator=(DocumentReference       const & doc) { switchTo(documentType::reference      ); data.reference       = doc; return (*this); }
	inline Document & operator=(DocumentReference            && doc) { switchTo(documentType::reference      ); data.reference       = doc; return (*this); }
	inline Document & operator=(DocumentGene            const & doc) { switchTo(documentType::gene           ); data.gene            = doc; return (*this); }
	inline Document & operator=(DocumentGene                 && doc) { switchTo(documentType::gene           ); data.gene            = doc; return (*this); }
	inline Document & operator=(DocumentCoordinates            const & doc) { switchTo(documentType::coordinateTransform           ); data.coordinateTransform            = doc; return (*this); }
	inline Document & operator=(DocumentCoordinates                 && doc) { switchTo(documentType::coordinateTransform           ); data.coordinateTransform            = doc; return (*this); }
	inline Document & operator=(DocumentSequenceServiceInfo            const & doc) { switchTo(documentType::ga4ghSeqServiceInfo           ); data.ga4ghSeqServiceInfo            = doc; return (*this); }
	inline Document & operator=(DocumentSequenceServiceInfo                 && doc) { switchTo(documentType::ga4ghSeqServiceInfo           ); data.ga4ghSeqServiceInfo            = doc; return (*this); }
	inline Document & operator=(DocumentGa4ghSeqMetadata            const & doc) { switchTo(documentType::ga4ghSeqMetadata           ); data.ga4ghSeqMetadata            = doc; return (*this); }
	inline Document & operator=(DocumentGa4ghSeqMetadata                 && doc) { switchTo(documentType::ga4ghSeqMetadata           ); data.ga4ghSeqMetadata            = doc; return (*this); }
	// checking document type
	inline bool isNull           () const { return (fType == documentType::null           ); }
	inline bool isError          () const { return (fType == documentType::error          ); }
	inline bool isActiveGenomicVariant  () const { return (fType == documentType::activeGenomicVariant  ); }
	inline bool isActiveProteinVariant  () const { return (fType == documentType::activeProteinVariant  ); }
	inline bool isInactiveVariant() const { return (fType == documentType::inactiveVariant); }
	inline bool isReference      () const { return (fType == documentType::reference      ); }
	inline bool isGene           () const { return (fType == documentType::gene           ); }
	inline bool isCoordinateTransform          () const { return (fType == documentType::coordinateTransform           ); }
	inline bool isGa4ghSeqServiceInfo          () const { return (fType == documentType::ga4ghSeqServiceInfo           ); }
	inline bool isGa4ghSeqMetadata          () const { return (fType == documentType::ga4ghSeqMetadata           ); }
	// accessing data
	inline DocumentError           const & error          () const { check(documentType::error          ); return data.error          ; }
	inline DocumentError                 & error          ()       { check(documentType::error          ); return data.error          ; }
	inline DocumentActiveGenomicVariant const & asActiveGenomicVariant() const { check(documentType::activeGenomicVariant  ); return data.activeGenomicVariant  ; }
	inline DocumentActiveGenomicVariant       & asActiveGenomicVariant()       { check(documentType::activeGenomicVariant  ); return data.activeGenomicVariant  ; }
	inline DocumentActiveProteinVariant const & asActiveProteinVariant() const { check(documentType::activeProteinVariant  ); return data.activeProteinVariant  ; }
	inline DocumentActiveProteinVariant       & asActiveProteinVariant()       { check(documentType::activeProteinVariant  ); return data.activeProteinVariant  ; }
	inline DocumentInactiveVariant const & inactiveVariant() const { check(documentType::inactiveVariant); return data.inactiveVariant; }
	inline DocumentInactiveVariant       & inactiveVariant()       { check(documentType::inactiveVariant); return data.inactiveVariant; }
	inline DocumentReference       const & reference      () const { check(documentType::reference      ); return data.reference      ; }
	inline DocumentReference             & reference      ()       { check(documentType::reference      ); return data.reference      ; }
	inline DocumentGene            const & gene           () const { check(documentType::gene           ); return data.gene           ; }
	inline DocumentGene                  & gene           ()       { check(documentType::gene           ); return data.gene           ; }
	inline DocumentCoordinates            const & coordinateTransform           () const { check(documentType::coordinateTransform           ); return data.coordinateTransform           ; }
	inline DocumentCoordinates                  & coordinateTransform           ()       { check(documentType::coordinateTransform           ); return data.coordinateTransform           ; }
	inline DocumentSequenceServiceInfo            const & ga4ghSeqServiceInfo           () const { check(documentType::ga4ghSeqServiceInfo           ); return data.ga4ghSeqServiceInfo           ; }
	inline DocumentSequenceServiceInfo                  & ga4ghSeqServiceInfo           ()       { check(documentType::ga4ghSeqServiceInfo           ); return data.ga4ghSeqServiceInfo           ; }
	inline DocumentGa4ghSeqMetadata            const & ga4ghSeqMetadata           () const { check(documentType::ga4ghSeqMetadata           ); return data.ga4ghSeqMetadata           ; }
	inline DocumentGa4ghSeqMetadata                  & ga4ghSeqMetadata           ()       { check(documentType::ga4ghSeqMetadata           ); return data.ga4ghSeqMetadata           ; }
	// less operator
	inline bool operator<(Document const & doc) const
	{
		if (fType != doc.fType) return (fType < doc.fType);
		switch (fType) {
			case documentType::null           : return false;
			case documentType::error          : return (error          () < doc.error          ());
			case documentType::activeGenomicVariant  : return (asActiveGenomicVariant  () < doc.asActiveGenomicVariant  ());
			case documentType::activeProteinVariant  : return (asActiveProteinVariant  () < doc.asActiveProteinVariant  ());
			case documentType::inactiveVariant: return (inactiveVariant() < doc.inactiveVariant());
			case documentType::reference      : return (reference      () < doc.reference      ());
			case documentType::gene           : return (gene           () < doc.gene           ());
			case documentType::coordinateTransform           : return (coordinateTransform           () < doc.coordinateTransform           ());
			case documentType::ga4ghSeqServiceInfo           : return (ga4ghSeqServiceInfo           () < doc.ga4ghSeqServiceInfo           ());
			case documentType::ga4ghSeqMetadata           : return (ga4ghSeqMetadata           () < doc.ga4ghSeqMetadata           ());
		}
		throw std::logic_error("Unknown document type");
	}
	// toString
	std::string toString() const;
};
RELATIONAL_OPERATORS(Document);
inline std::ostream & operator<<(std::ostream & s, Document const & d) { s << d.toString(); return s; }

#endif /* DOCUMENT_HPP_ */
