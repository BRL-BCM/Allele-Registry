#ifndef RECORDVARIANT_HPP_
#define RECORDVARIANT_HPP_

#include "../apiDb/db.hpp"
#include "../core/variants.hpp"
#include <map>

	struct BinaryNucleotideSequenceModification
	{
		uint32_t position;         // position
		uint16_t lengthBefore = 0; // after for shifted deletion
		uint16_t lengthChangeOrSeqLength = 0; // for shiftable deletion or duplication, sequence size for others
		uint32_t sequence = 0;
		variantCategory category = nonShiftable;
		inline bool operator<(BinaryNucleotideSequenceModification const & v) const
		{
			if (position != v.position) return (position < v.position);
			if (lengthBefore != v.lengthBefore) return (lengthBefore < v.lengthBefore);
			if (category != v.category) return (category < v.category);
			if (lengthChangeOrSeqLength != v.lengthChangeOrSeqLength) return (lengthChangeOrSeqLength < v.lengthChangeOrSeqLength);
			return (sequence < v.sequence);
		}
	};
	RELATIONAL_OPERATORS(BinaryNucleotideSequenceModification);


	class BinaryGenomicVariantDefinition
	{
	private:
		 // must be sorted by position
		std::vector<BinaryNucleotideSequenceModification> simpleVariants;
	public:
		explicit BinaryGenomicVariantDefinition(uint32_t firstPosition); // create definition with simple variant
		explicit BinaryGenomicVariantDefinition(std::vector<BinaryNucleotideSequenceModification> & definition);
		inline std::vector<BinaryNucleotideSequenceModification> const & raw() const { return simpleVariants; }
		inline uint32_t firstPosition() const { return simpleVariants.at(0).position; }
		// these 3 functions below omit position of the first simple variant (it is used as a key)
		unsigned dataLength() const;
		void saveData(uint8_t *& ptr) const;
		void loadData(uint8_t const *& ptr);
		// toString
		std::string toString() const;
		// less
		inline bool operator<(BinaryGenomicVariantDefinition const & v) const
		{
			return (simpleVariants < v.simpleVariants);
		}
	};
	RELATIONAL_OPERATORS(BinaryGenomicVariantDefinition);


	class RecordVariantPtr;

	class BinaryIdentifiers
	{
	private:
		identifierType fLastIdType;
		uint32_t fLastId = CanonicalId::null.value;
		// both vectors are sorted
		std::vector<IdentifierShort> fShortIds;
		std::vector<IdentifierWellDefined> fHgvsIds;
	public:
		BinaryIdentifiers( identifierType lastIdType ) : fLastIdType(lastIdType) {}
		BinaryIdentifiers( identifierType lastIdType, std::vector<IdentifierShort> const & shortIds
						 , std::vector<IdentifierWellDefined> const & hgvsIds );
		// serialization
		unsigned dataLength() const;
		void saveData(uint8_t *& ptr) const;
		void loadData(uint8_t const *& ptr);
		// return raw data, identifiers are always sorted
		inline std::vector<IdentifierShort> const & rawShort() const { return fShortIds; }
		inline std::vector<IdentifierWellDefined>      const & rawHgvs () const { return fHgvsIds ; }
		inline uint32_t const & lastId() const { return fLastId; }
		inline uint32_t & lastId() { return fLastId; }
		// check if there is at least one identifier with type belonging to given vector
		bool hasOneOfIds(std::vector<identifierType> idTypes) const;
		// add short id
		void add(IdentifierShort const &);
		// check if contains any ids
		inline bool empty() const { return (fShortIds.empty() && fHgvsIds.empty()); }
		// remove all identifiers
		inline void clear() { fLastId = CanonicalId::null.value; fShortIds.clear(); fHgvsIds.clear(); }
		// exchange identifiers between list, at the end both list are equal
		void exchange(BinaryIdentifiers &);
		// add identifiers given in another list, returns list of new (added) identifiers
		BinaryIdentifiers add(BinaryIdentifiers const &);
		// remove identifiers given in another list, returns list of matched (deleted) identifiers
		BinaryIdentifiers remove(BinaryIdentifiers const &);
		BinaryIdentifiers remove(identifierType);
		// save short ids to given map
		void saveShortIdsToContainer( std::map<identifierType,std::vector<std::pair<uint32_t,RecordVariantPtr>>> & container, RecordVariantPtr) const;
		// operator<
		inline bool operator<(BinaryIdentifiers const & v) const
		{
			if (fLastIdType != v.fLastIdType) return (fLastIdType < v.fLastIdType);
			if (fLastId != v.fLastId) return (fLastId < v.fLastId);
			if (fShortIds != v.fShortIds) return (fShortIds < v.fShortIds);
			return (fHgvsIds < v.fHgvsIds);
		}
	};
	RELATIONAL_OPERATORS(BinaryIdentifiers);


	class RecordGenomicVariant : public RecordT<uint32_t>
	{
	public:
		BinaryGenomicVariantDefinition definition;
		//CanonicalId caId;
		BinaryIdentifiers identifiers;
		uint32_t revision = 0;
		explicit RecordGenomicVariant(uint32_t pKey) : RecordT<uint32_t>(pKey), definition(pKey), identifiers(identifierType::CA) {}
		explicit RecordGenomicVariant(BinaryGenomicVariantDefinition const & def) : RecordT<uint32_t>(def.firstPosition()), definition(def), identifiers(identifierType::CA) {}
		virtual unsigned dataLength() const;
		virtual void saveData(uint8_t *& ptr) const;
		virtual void loadData(uint8_t const *& ptr);
		virtual ~RecordGenomicVariant() {}
	};


	// ====================================================== protein variants

	struct BinaryAminoAcidSequenceModification
	{
		uint16_t position;         // position
		uint16_t lengthBefore = 0; // after for shifted deletion
		uint16_t lengthChangeOrSeqLength = 0; // for shiftable deletion or duplication, sequence size for others
		uint32_t sequence = 0;
		variantCategory category = nonShiftable;
		inline bool operator<(BinaryAminoAcidSequenceModification const & v) const
		{
			if (position != v.position) return (position < v.position);
			if (lengthBefore != v.lengthBefore) return (lengthBefore < v.lengthBefore);
			if (category != v.category) return (category < v.category);
			if (lengthChangeOrSeqLength != v.lengthChangeOrSeqLength) return (lengthChangeOrSeqLength < v.lengthChangeOrSeqLength);
			return (sequence < v.sequence);
		}
	};
	RELATIONAL_OPERATORS(BinaryAminoAcidSequenceModification);


	class BinaryProteinVariantDefinition
	{
	private:
		uint64_t fProteinAccessionIdentifier;
		 // must be sorted by position
		std::vector<BinaryAminoAcidSequenceModification> simpleVariants;
	public:
		explicit BinaryProteinVariantDefinition(uint64_t proteinAccIdAndFirstPosition); // create definition with simple variant
		explicit BinaryProteinVariantDefinition(uint64_t proteinAccId, std::vector<BinaryAminoAcidSequenceModification> & definition);
		inline std::vector<BinaryAminoAcidSequenceModification> const & raw() const { return simpleVariants; }
		inline uint64_t proteinAccIdAndFirstPosition() const { return ((fProteinAccessionIdentifier << 16) + simpleVariants.at(0).position); }
		inline uint64_t proteinAccessionId() const { return fProteinAccessionIdentifier; }
		// these 3 functions below omit position of the first simple variant (it is used as a key)
		unsigned dataLength() const;
		void saveData(uint8_t *& ptr) const;
		void loadData(uint8_t const *& ptr);
		// toString
		std::string toString() const;
		// less
		inline bool operator<(BinaryProteinVariantDefinition const & v) const
		{
			if (fProteinAccessionIdentifier != v.fProteinAccessionIdentifier) {
				return (fProteinAccessionIdentifier < v.fProteinAccessionIdentifier);
			}
			return (simpleVariants < v.simpleVariants);
		}
	};
	RELATIONAL_OPERATORS(BinaryProteinVariantDefinition);


	class RecordProteinVariant : public RecordT<uint64_t>
	{
	public:
		BinaryProteinVariantDefinition definition;
		BinaryIdentifiers identifiers;
		uint32_t revision = 0;
		explicit RecordProteinVariant(uint64_t pKey) : RecordT<uint64_t>(pKey), definition(pKey), identifiers(identifierType::PA) {}
		explicit RecordProteinVariant(BinaryProteinVariantDefinition const & def) : RecordT<uint64_t>(def.proteinAccIdAndFirstPosition()), definition(def), identifiers(identifierType::PA) {}
		virtual unsigned dataLength() const;
		virtual void saveData(uint8_t *& ptr) const;
		virtual void loadData(uint8_t const *& ptr);
		virtual ~RecordProteinVariant() {}
	};

	class RecordVariantPtr
	{
	private:
		union {
			RecordGenomicVariant * genomicPtr = nullptr;
			RecordProteinVariant * proteinPtr;
		};
		bool fGenomic = true;
	public:
		RecordVariantPtr(RecordGenomicVariant * p) : genomicPtr(p), fGenomic(true ) {}
		RecordVariantPtr(RecordProteinVariant * p) : proteinPtr(p), fGenomic(false) {}
		RecordGenomicVariant * asRecordGenomicVariantPtr() { ASSERT( fGenomic); return genomicPtr; }
		RecordProteinVariant * asRecordProteinVariantPtr() { ASSERT(!fGenomic); return proteinPtr; }
		RecordGenomicVariant const * asRecordGenomicVariantPtr() const { ASSERT( fGenomic); return genomicPtr; }
		RecordProteinVariant const * asRecordProteinVariantPtr() const { ASSERT(!fGenomic); return proteinPtr; }
		bool isRecordGenomicVariantPtr() const { return fGenomic; }
		bool isRecordProteinVariantPtr() const { return (! fGenomic); }
	};

#endif /* RECORDVARIANT_HPP_ */
