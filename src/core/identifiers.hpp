#ifndef CORE_IDENTIFIERS_HPP_
#define CORE_IDENTIFIERS_HPP_


#include "globals.hpp"
#include "../commonTools/assert.hpp"


enum identifierType : uint16_t
{
	  CA = 0
	, dbSNP = 1
	, ClinVarAllele = 2
	, ClinVarVariant = 3
	, MyVariantInfo_hg19 = 4
	, MyVariantInfo_hg38 = 5
	, ExAC = 6
	, gnomAD = 7
	, ClinVarRCV = 8
	, AllelicEpigenome = 9  // TODO - not used, to remove
	, COSMIC = 10
	, externalSource = 15
	, PA = 255
};

inline std::string toString(identifierType type)
{
	switch (type) {
		case identifierType::CA                : return "CA";
		case identifierType::dbSNP             : return "dbSNP";
		case identifierType::ClinVarAllele     : return "ClinVarAllele";
		case identifierType::ClinVarVariant    : return "ClinVarVariant";
		case identifierType::MyVariantInfo_hg19: return "MyVariantInfo_hg19";
		case identifierType::MyVariantInfo_hg38: return "MyVariantInfo_hg38";
		case identifierType::ExAC              : return "ExAC";
		case identifierType::gnomAD            : return "gnomAD";
		case identifierType::ClinVarRCV        : return "ClinVarRCV";
		case identifierType::AllelicEpigenome  : return "AllelicEpigenome";
		case identifierType::COSMIC            : return "COSMIC";
		case identifierType::externalSource    : return "externalSource";
		case identifierType::PA                : return "PA";
	}
	ASSERT(false);
}


inline bool isShortIdentifier(identifierType type)
{
	switch (type) {
		case identifierType::MyVariantInfo_hg19:
		case identifierType::MyVariantInfo_hg38:
		case identifierType::ExAC:
		case identifierType::gnomAD:
			return false;
		case identifierType::CA:
		case identifierType::dbSNP:
		case identifierType::ClinVarAllele:
		case identifierType::ClinVarVariant:
		case identifierType::ClinVarRCV:
		case identifierType::AllelicEpigenome:
		case identifierType::COSMIC:
		case identifierType::externalSource:
		case identifierType::PA:
			return true;
	}
	ASSERT(false);
}


struct IdentifierWellDefined
{
	identifierType idType;
	enum Type : uint8_t {
		  noHgvs                  = 0
		, deletion                = 1
		, deletionWithLength      = 2
		, deletionWithSequence    = 3
		, duplication             = 4
		, duplicationWithLength   = 5
		, duplicationWithSequence = 6
		, delins                  = 7
	} expressionType = noHgvs;
	uint16_t shiftToRightAligned = 0;  // distance between left position from id and left position of the most right-aligned
	uint16_t rightExtend = 0;          // extend from the right side
	unsigned dataLength() const;
	void saveData(uint8_t *& ptr) const;
	void loadData(uint8_t const *& ptr);
	// operator<
	inline bool operator<(IdentifierWellDefined const & id) const
	{
		if (idType     != id.idType    ) return (idType     < id.idType    );
		if (expressionType != id.expressionType) return (expressionType < id.expressionType);
		if (shiftToRightAligned != id.shiftToRightAligned) return (shiftToRightAligned < id.shiftToRightAligned);
		return (rightExtend < id.rightExtend);
	}
};
RELATIONAL_OPERATORS(IdentifierWellDefined);


struct Identifier_dbSNP
{
	uint32_t rs;
	Identifier_dbSNP(uint32_t rsId = 0) : rs(rsId) {}
	bool operator<(Identifier_dbSNP const & v) const { return (rs < v.rs); }
	std::string toString() const;
	unsigned dataLength() const;
	void saveData(uint8_t *& ptr) const;
	void loadData(uint8_t const *& ptr);
};

struct Identifier_ClinVarAllele
{
	uint32_t alleleId;
	std::string preferredName;
	Identifier_ClinVarAllele(uint32_t id = 0, std::string const & name = "") : alleleId(id), preferredName(name) {}
	bool operator<(Identifier_ClinVarAllele const & v) const { return (alleleId < v.alleleId); }
	std::string toString() const;
	unsigned dataLength() const;
	void saveData(uint8_t *& ptr) const;
	void loadData(uint8_t const *& ptr);
};

struct Identifier_ClinVarVariant
{
	uint32_t variantId;
	std::vector<uint32_t> RCVs;
	Identifier_ClinVarVariant(uint32_t id = 0, std::vector<uint32_t> const & rcvs = std::vector<uint32_t>()) : variantId(id), RCVs(rcvs) {}
	bool operator<(Identifier_ClinVarVariant const & v) const { return (variantId < v.variantId); }
	std::string toString() const;
	unsigned dataLength() const;
	void saveData(uint8_t *& ptr) const;
	void loadData(uint8_t const *& ptr);
};

struct Identifier_AllelicEpigenome
{
	uint32_t ae;
	Chromosome chr;
	Identifier_AllelicEpigenome(Chromosome pChr = Chromosome::chrUnknown, uint32_t aeId = 0) : ae(aeId), chr(pChr) {}
	bool operator<(Identifier_AllelicEpigenome const & v) const { return (chr < v.chr || (chr == v.chr && ae < v.ae)); }
	std::string toString() const;
	unsigned dataLength() const;
	void saveData(uint8_t *& ptr) const;
	void loadData(uint8_t const *& ptr);
};

struct Identifier_COSMIC
{
	uint32_t id;
	bool coding;
	bool active;
	Identifier_COSMIC(bool pCoding = false, uint32_t pId = 0, bool pActive = false) : id(pId), coding(pCoding), active(pActive) {}
	bool operator<(Identifier_COSMIC const & v) const { return (id < v.id || (id == v.id && coding < v.coding)); }
	std::string toString() const;
	unsigned dataLength() const;
	void saveData(uint8_t *& ptr) const;
	void loadData(uint8_t const *& ptr);
};

struct Identifier_externalSource
{
	uint8_t id;
	Identifier_externalSource(uint32_t pId = 0) : id(pId) {}
	bool operator<(Identifier_externalSource const & v) const { return (id < v.id); }
	std::string toString() const;
	unsigned dataLength() const;
	void saveData(uint8_t *& ptr) const;
	void loadData(uint8_t const *& ptr);
};

struct IdentifierShort {
	identifierType fIdType = identifierType::dbSNP;
	union internalData {
		Identifier_dbSNP            dbSNP = Identifier_dbSNP(0);
		Identifier_ClinVarAllele    ClinVarAllele;
		Identifier_ClinVarVariant   ClinVarVariant;
		Identifier_AllelicEpigenome AllelicEpigenome;
		Identifier_COSMIC           COSMIC;
		Identifier_externalSource   externalSource;
		internalData() {}
		~internalData() {}
	} d;
	void switchTo(identifierType idType) {
		if (idType == fIdType) return;
		switch(fIdType) {
		case identifierType::dbSNP: d.dbSNP.~Identifier_dbSNP(); break;
		case identifierType::ClinVarAllele: d.ClinVarAllele.~Identifier_ClinVarAllele(); break;
		case identifierType::ClinVarVariant: d.ClinVarVariant.~Identifier_ClinVarVariant(); break;
		case identifierType::AllelicEpigenome: d.AllelicEpigenome.~Identifier_AllelicEpigenome(); break;
		case identifierType::COSMIC: d.COSMIC.~Identifier_COSMIC(); break;
		case identifierType::externalSource: d.externalSource.~Identifier_externalSource(); break;
		}
		fIdType = idType;
		switch(fIdType) {
		case identifierType::dbSNP: new(&d.dbSNP) Identifier_dbSNP; break;
		case identifierType::ClinVarAllele: new(&d.ClinVarAllele) Identifier_ClinVarAllele; break;
		case identifierType::ClinVarVariant: new(&d.ClinVarVariant) Identifier_ClinVarVariant; break;
		case identifierType::AllelicEpigenome: new(&d.AllelicEpigenome) Identifier_AllelicEpigenome; break;
		case identifierType::COSMIC: new(&d.COSMIC) Identifier_COSMIC; break;
		case identifierType::externalSource: new(&d.externalSource) Identifier_externalSource; break;
		}
	}
	IdentifierShort(identifierType idType = identifierType::dbSNP) { switchTo(idType); }
	IdentifierShort & operator=(Identifier_dbSNP const & v) { switchTo(identifierType::dbSNP); d.dbSNP = v; return *this; }
	IdentifierShort & operator=(Identifier_ClinVarAllele const & v) { switchTo(identifierType::ClinVarAllele); d.ClinVarAllele = v; return *this; }
	IdentifierShort & operator=(Identifier_ClinVarVariant const & v) { switchTo(identifierType::ClinVarVariant); d.ClinVarVariant = v; return *this; }
	IdentifierShort & operator=(Identifier_AllelicEpigenome const & v) { switchTo(identifierType::AllelicEpigenome); d.AllelicEpigenome = v; return *this; }
	IdentifierShort & operator=(Identifier_COSMIC const & v) { switchTo(identifierType::COSMIC); d.COSMIC = v; return *this; }
	IdentifierShort & operator=(Identifier_externalSource const & v) { switchTo(identifierType::externalSource); d.externalSource = v; return *this; }
	IdentifierShort & operator=(IdentifierShort const & v)
	{
		switchTo(v.fIdType);
		switch(fIdType) {
		case identifierType::dbSNP: d.dbSNP = v.d.dbSNP; break;
		case identifierType::ClinVarAllele: d.ClinVarAllele = v.d.ClinVarAllele; break;
		case identifierType::ClinVarVariant: d.ClinVarVariant = v.d.ClinVarVariant; break;
		case identifierType::AllelicEpigenome: d.AllelicEpigenome = v.d.AllelicEpigenome; break;
		case identifierType::COSMIC: d.COSMIC = v.d.COSMIC; break;
		case identifierType::externalSource: d.externalSource = v.d.externalSource; break;
		}
		return *this;
	}
	IdentifierShort & operator=(IdentifierShort && v)
	{
		switchTo(v.fIdType);
		switch(fIdType) {
		case identifierType::dbSNP: d.dbSNP = v.d.dbSNP; break;
		case identifierType::ClinVarAllele: d.ClinVarAllele = v.d.ClinVarAllele; break;
		case identifierType::ClinVarVariant: d.ClinVarVariant = v.d.ClinVarVariant; break;
		case identifierType::AllelicEpigenome: d.AllelicEpigenome = v.d.AllelicEpigenome; break;
		case identifierType::COSMIC: d.COSMIC = v.d.COSMIC; break;
		case identifierType::externalSource: d.externalSource = v.d.externalSource; break;
		}
		return *this;
	}
	IdentifierShort(IdentifierShort const & v) { *this = v; }
	IdentifierShort(IdentifierShort && v) { *this = v; }
	IdentifierShort(Identifier_dbSNP const & v) { *this = v; }
	IdentifierShort(Identifier_ClinVarAllele const & v) { *this = v; }
	IdentifierShort(Identifier_ClinVarVariant const & v) { *this = v; }
	IdentifierShort(Identifier_AllelicEpigenome const & v) { *this = v; }
	IdentifierShort(Identifier_COSMIC const & v) { *this = v; }
	IdentifierShort(Identifier_externalSource const & v) { *this = v; }
	Identifier_dbSNP & as_dbSNP() { return d.dbSNP; }
	Identifier_ClinVarAllele & as_ClinVarAllele() { return d.ClinVarAllele; }
	Identifier_ClinVarVariant & as_ClinVarVariant() { return d.ClinVarVariant; }
	Identifier_AllelicEpigenome & as_AllelicEpigenome() { return d.AllelicEpigenome; }
	Identifier_COSMIC & as_COSMIC() { return d.COSMIC; }
	Identifier_externalSource & as_externalSource() { return d.externalSource; }
	Identifier_dbSNP const & as_dbSNP() const { return d.dbSNP; }
	Identifier_ClinVarAllele const & as_ClinVarAllele() const { return d.ClinVarAllele; }
	Identifier_ClinVarVariant const & as_ClinVarVariant() const { return d.ClinVarVariant; }
	Identifier_AllelicEpigenome const & as_AllelicEpigenome() const { return d.AllelicEpigenome; }
	Identifier_COSMIC const & as_COSMIC() const { return d.COSMIC; }
	Identifier_externalSource const & as_externalSource() const { return d.externalSource; }
	bool operator<(IdentifierShort const & v) const
	{
		if (fIdType == v.fIdType) {
			switch(fIdType) {
			case identifierType::dbSNP: return d.dbSNP < v.d.dbSNP;
			case identifierType::ClinVarAllele: return d.ClinVarAllele < v.d.ClinVarAllele;
			case identifierType::ClinVarVariant: return d.ClinVarVariant < v.d.ClinVarVariant;
			case identifierType::AllelicEpigenome: return d.AllelicEpigenome < v.d.AllelicEpigenome;
			case identifierType::COSMIC: return d.COSMIC < v.d.COSMIC;
			case identifierType::externalSource: return d.externalSource < v.d.externalSource;
			}
		}
		return (fIdType < v.fIdType);
	}
	void saveIdsToContainer(std::vector<std::pair<identifierType,uint32_t>> & container) const  // TODO - this is for indexes
	{
		switch(fIdType) {
			case identifierType::dbSNP:
				container.push_back(std::make_pair(identifierType::dbSNP, d.dbSNP.rs));
				break;
			case identifierType::ClinVarAllele:
				container.push_back(std::make_pair(identifierType::ClinVarAllele, d.ClinVarAllele.alleleId));
				break;
			case identifierType::ClinVarVariant:
				container.push_back(std::make_pair(identifierType::ClinVarVariant, d.ClinVarVariant.variantId));
				for (auto const & e: d.ClinVarVariant.RCVs) {
					container.push_back(std::make_pair(identifierType::ClinVarRCV, e));
				}
				break;
			case identifierType::COSMIC:
				container.push_back(std::make_pair(identifierType::COSMIC, d.COSMIC.id));
				break;
		}
	}
	std::string toString() const
	{
		switch(fIdType) {
		case identifierType::dbSNP: return d.dbSNP.toString();
		case identifierType::ClinVarAllele: return d.ClinVarAllele.toString();
		case identifierType::ClinVarVariant: return d.ClinVarVariant.toString();
		case identifierType::AllelicEpigenome: return d.AllelicEpigenome.toString();
		case identifierType::COSMIC: return d.COSMIC.toString();
		case identifierType::externalSource: return d.externalSource.toString();
		}
		return "";
	}
	unsigned dataLength() const
	{
		switch(fIdType) {
		case identifierType::dbSNP: return d.dbSNP.dataLength();
		case identifierType::ClinVarAllele: return d.ClinVarAllele.dataLength();
		case identifierType::ClinVarVariant: return d.ClinVarVariant.dataLength();
		case identifierType::AllelicEpigenome: return d.AllelicEpigenome.dataLength();
		case identifierType::COSMIC: return d.COSMIC.dataLength();
		case identifierType::externalSource: return d.externalSource.dataLength();
		}
		return 0;
	}
	void saveData(uint8_t *& ptr) const
	{
		switch(fIdType) {
		case identifierType::dbSNP: d.dbSNP.saveData(ptr); break;
		case identifierType::ClinVarAllele: d.ClinVarAllele.saveData(ptr);  break;
		case identifierType::ClinVarVariant: d.ClinVarVariant.saveData(ptr);  break;
		case identifierType::AllelicEpigenome: d.AllelicEpigenome.saveData(ptr); break;
		case identifierType::COSMIC: d.COSMIC.saveData(ptr); break;
		case identifierType::externalSource: d.externalSource.saveData(ptr); break;
		}
	}
	void loadData(uint8_t const *& ptr)
	{
		switch(fIdType) {
		case identifierType::dbSNP: d.dbSNP.loadData(ptr); break;
		case identifierType::ClinVarAllele: d.ClinVarAllele.loadData(ptr);  break;
		case identifierType::ClinVarVariant: d.ClinVarVariant.loadData(ptr);  break;
		case identifierType::AllelicEpigenome: d.AllelicEpigenome.loadData(ptr); break;
		case identifierType::COSMIC: d.COSMIC.loadData(ptr); break;
		case identifierType::externalSource: d.externalSource.loadData(ptr); break;
		}
	}
};
RELATIONAL_OPERATORS(IdentifierShort);


// it has at most one CA ID, all elements are unique, elements are sorted in reverse order (CA ID is at the end)
class Identifiers
{
private:
	std::vector<IdentifierShort> fShortIds;
	std::vector<IdentifierWellDefined> fHgvsIds;
public:
	// constructors
	Identifiers() {}
	explicit Identifiers( std::vector<IdentifierShort> const & shortIds
						, std::vector<IdentifierWellDefined> const & hgvsIds );
	// add identifier
	void add(IdentifierShort const & id);
	void add(IdentifierWellDefined const & id);
	// add identifiers
	void add(Identifiers const & ids);
	// remove identifiers
	void remove(Identifiers const & ids);
	// check if contains identifier
	bool has(identifierType type, uint32_t value) const;
	// delete all identifiers
	inline void clear() { fShortIds.clear(); fHgvsIds.clear(); }
	// get identifiers by type
	std::vector<IdentifierShort> getShortIds(identifierType) const;
	std::vector<IdentifierWellDefined> getHgvsIds(identifierType) const;
	// return raw data, identifiers are always sorted
	inline std::vector<IdentifierShort> const & rawShort() const { return fShortIds; }
	inline std::vector<IdentifierWellDefined> const & rawHgvs() const { return fHgvsIds; }
	// standard stuff
	std::string toString() const;
	inline bool operator<(Identifiers const & ids) const
	{
		if (fShortIds != ids.fShortIds) return (fShortIds < ids.fShortIds);
		return (fHgvsIds < ids.fHgvsIds);
	}
};
RELATIONAL_OPERATORS(Identifiers);


#endif /* CORE_IDENTIFIERS_HPP_ */
