#ifndef RECORDVARIANT32_HPP_
#define RECORDVARIANT32_HPP_

#include "../apiDb/db.hpp"
#include "../core/variants.hpp"
#include <map>

	struct BinaryNucleotideSequenceModification32
	{
		uint64_t position;         // position
		uint16_t lengthBefore = 0; // after for shifted deletion
		uint16_t lengthChangeOrSeqLength = 0; // for shiftable deletion or duplication, sequence size for others
		uint32_t sequence = 0;
		variantCategory category = nonShiftable;
		inline bool operator<(BinaryNucleotideSequenceModification32 const & v) const
		{
			if (position != v.position) return (position < v.position);
			if (lengthBefore != v.lengthBefore) return (lengthBefore < v.lengthBefore);
			if (category != v.category) return (category < v.category);
			if (lengthChangeOrSeqLength != v.lengthChangeOrSeqLength) return (lengthChangeOrSeqLength < v.lengthChangeOrSeqLength);
			return (sequence < v.sequence);
		}
	};
	RELATIONAL_OPERATORS(BinaryNucleotideSequenceModification32);


	class BinaryGenomicVariantDefinition32
	{
	private:
		 // must be sorted by position
		std::vector<BinaryNucleotideSequenceModification32> simpleVariants;
	public:
		explicit BinaryGenomicVariantDefinition32(uint64_t firstPosition); // create definition with simple variant
		explicit BinaryGenomicVariantDefinition32(std::vector<BinaryNucleotideSequenceModification32> & definition);
		inline std::vector<BinaryNucleotideSequenceModification32> const & raw() const { return simpleVariants; }
		inline uint64_t firstPosition() const { return simpleVariants.at(0).position; }
		// these 3 functions below omit position of the first simple variant (it is used as a key)
		unsigned dataLength() const;
		void saveData(uint8_t *& ptr) const;
		void loadData(uint8_t const *& ptr);
		// toString
		std::string toString() const;
		// less
		inline bool operator<(BinaryGenomicVariantDefinition32 const & v) const
		{
			return (simpleVariants < v.simpleVariants);
		}
	};
	RELATIONAL_OPERATORS(BinaryGenomicVariantDefinition32);


	class RecordVariantPtr32;

	class BinaryIdentifiers32
	{
	public:
		identifierType fLastIdType;
		uint64_t fLastId = CanonicalId::null.value;
		// both vectors are sorted
		std::vector<IdentifierShort> fShortIds;
		std::vector<IdentifierWellDefined> fHgvsIds;
	public:
		BinaryIdentifiers32( identifierType lastIdType ) : fLastIdType(lastIdType) {}
		BinaryIdentifiers32( identifierType lastIdType, std::vector<IdentifierShort> const & shortIds
						 , std::vector<IdentifierWellDefined> const & hgvsIds );
		// serialization
		unsigned dataLength() const;
		void saveData(uint8_t *& ptr) const;
		void loadData(uint8_t const *& ptr);
		// return raw data, identifiers are always sorted
		inline std::vector<IdentifierShort> const & rawShort() const { return fShortIds; }
		inline std::vector<IdentifierWellDefined>      const & rawHgvs () const { return fHgvsIds ; }
		inline uint64_t const & lastId() const { return fLastId; }
		inline uint64_t & lastId() { return fLastId; }
		// check if there is at least one identifier with type belonging to given vector
		bool hasOneOfIds(std::vector<identifierType> idTypes) const;
		// add short id
		void add(IdentifierShort const &);
		// check if contains any ids
		inline bool empty() const { return (fShortIds.empty() && fHgvsIds.empty()); }
		// remove all identifiers
		inline void clear() { fLastId = CanonicalId::null.value; fShortIds.clear(); fHgvsIds.clear(); }
		// exchange identifiers between list, at the end both list are equal
		void exchange(BinaryIdentifiers32 &);
		// add identifiers given in another list, returns list of new (added) identifiers
		BinaryIdentifiers32 add(BinaryIdentifiers32 const &);
		// remove identifiers given in another list, returns list of matched (deleted) identifiers
		BinaryIdentifiers32 remove(BinaryIdentifiers32 const &);
		BinaryIdentifiers32 remove(identifierType);
		// save short ids to given map
		void saveShortIdsToContainer( std::map<identifierType,std::vector<std::pair<uint64_t,RecordVariantPtr32>>> & container, RecordVariantPtr32) const;
		// operator<
		inline bool operator<(BinaryIdentifiers32 const & v) const
		{
			if (fLastIdType != v.fLastIdType) return (fLastIdType < v.fLastIdType);
			if (fLastId != v.fLastId) return (fLastId < v.fLastId);
			if (fShortIds != v.fShortIds) return (fShortIds < v.fShortIds);
			return (fHgvsIds < v.fHgvsIds);
		}
	};
	RELATIONAL_OPERATORS(BinaryIdentifiers32);


	class RecordGenomicVariant32 : public RecordT<uint64_t>
	{
	public:
		BinaryGenomicVariantDefinition32 definition;
		//CanonicalId caId;
		BinaryIdentifiers32 identifiers;
		uint32_t revision = 0;
		explicit RecordGenomicVariant32(uint64_t pKey) : RecordT<uint64_t>(pKey), definition(pKey), identifiers(identifierType::CA) {}
		explicit RecordGenomicVariant32(BinaryGenomicVariantDefinition32 const & def) : RecordT<uint64_t>(def.firstPosition()), definition(def), identifiers(identifierType::CA) {}
		virtual unsigned dataLength() const;
		virtual void saveData(uint8_t *& ptr) const;
		virtual void loadData(uint8_t const *& ptr);
		virtual ~RecordGenomicVariant32() {}
	};


	// ====================================================== protein variants

	struct BinaryAminoAcidSequenceModification32
	{
		uint16_t position;         // position
		uint16_t lengthBefore = 0; // after for shifted deletion
		uint16_t lengthChangeOrSeqLength = 0; // for shiftable deletion or duplication, sequence size for others
		uint32_t sequence = 0;
		variantCategory category = nonShiftable;
		inline bool operator<(BinaryAminoAcidSequenceModification32 const & v) const
		{
			if (position != v.position) return (position < v.position);
			if (lengthBefore != v.lengthBefore) return (lengthBefore < v.lengthBefore);
			if (category != v.category) return (category < v.category);
			if (lengthChangeOrSeqLength != v.lengthChangeOrSeqLength) return (lengthChangeOrSeqLength < v.lengthChangeOrSeqLength);
			return (sequence < v.sequence);
		}
	};
	RELATIONAL_OPERATORS(BinaryAminoAcidSequenceModification32);


	class BinaryProteinVariantDefinition32
	{
	private:
		uint64_t fProteinAccessionIdentifier;
		 // must be sorted by position
		std::vector<BinaryAminoAcidSequenceModification32> simpleVariants;
	public:
		explicit BinaryProteinVariantDefinition32(uint64_t proteinAccIdAndFirstPosition); // create definition with simple variant
		explicit BinaryProteinVariantDefinition32(uint64_t proteinAccId, std::vector<BinaryAminoAcidSequenceModification32> & definition);
		inline std::vector<BinaryAminoAcidSequenceModification32> const & raw() const { return simpleVariants; }
		inline uint64_t proteinAccIdAndFirstPosition() const { return ((fProteinAccessionIdentifier << 16) + simpleVariants.at(0).position); }
		inline uint64_t proteinAccessionId() const { return fProteinAccessionIdentifier; }
		// these 3 functions below omit position of the first simple variant (it is used as a key)
		unsigned dataLength() const;
		void saveData(uint8_t *& ptr) const;
		void loadData(uint8_t const *& ptr);
		// toString
		std::string toString() const;
		// less
		inline bool operator<(BinaryProteinVariantDefinition32 const & v) const
		{
			if (fProteinAccessionIdentifier != v.fProteinAccessionIdentifier) {
				return (fProteinAccessionIdentifier < v.fProteinAccessionIdentifier);
			}
			return (simpleVariants < v.simpleVariants);
		}
	};
	RELATIONAL_OPERATORS(BinaryProteinVariantDefinition32);


	class RecordProteinVariant32 : public RecordT<uint64_t>
	{
	public:
		BinaryProteinVariantDefinition32 definition;
		BinaryIdentifiers32 identifiers;
		uint32_t revision = 0;
		explicit RecordProteinVariant32(uint64_t pKey) : RecordT<uint64_t>(pKey), definition(pKey), identifiers(identifierType::PA) {}
		explicit RecordProteinVariant32(BinaryProteinVariantDefinition32 const & def) : RecordT<uint64_t>(def.proteinAccIdAndFirstPosition()), definition(def), identifiers(identifierType::PA) {}
		virtual unsigned dataLength() const;
		virtual void saveData(uint8_t *& ptr) const;
		virtual void loadData(uint8_t const *& ptr);
		virtual ~RecordProteinVariant32() {}
	};

	class RecordVariantPtr32
	{
	private:
		union {
			RecordGenomicVariant32 * genomicPtr = nullptr;
			RecordProteinVariant32 * proteinPtr;
		};
		bool fGenomic = true;
	public:
		RecordVariantPtr32(RecordGenomicVariant32 * p) : genomicPtr(p), fGenomic(true ) {}
		RecordVariantPtr32(RecordProteinVariant32 * p) : proteinPtr(p), fGenomic(false) {}
		RecordGenomicVariant32 * asRecordGenomicVariant32Ptr() { ASSERT( fGenomic); return genomicPtr; }
		RecordProteinVariant32 * asRecordProteinVariant32Ptr() { ASSERT(!fGenomic); return proteinPtr; }
		RecordGenomicVariant32 const * asRecordGenomicVariant32Ptr() const { ASSERT( fGenomic); return genomicPtr; }
		RecordProteinVariant32 const * asRecordProteinVariant32Ptr() const { ASSERT(!fGenomic); return proteinPtr; }
		bool isRecordGenomicVariant32Ptr() const { return fGenomic; }
		bool isRecordProteinVariant32Ptr() const { return (! fGenomic); }
	};

#endif /* RECORDVARIANT32_HPP_ */
