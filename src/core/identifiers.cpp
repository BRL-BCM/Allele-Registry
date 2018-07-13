#include "variants.hpp"
#include "../commonTools/bytesLevel.hpp"
#include "../commonTools/setsOperators.hpp"
#include "../commonTools/parseTools.hpp"
#include <limits>



	unsigned IdentifierWellDefined::dataLength() const
	{
		if (idType == identifierType::ExAC || idType == identifierType::gnomAD) return 0;
		if (shiftToRightAligned < 4        && rightExtend < 4  ) return 1;
		if (shiftToRightAligned < 8*16     && rightExtend < 16 ) return 2;
		if (shiftToRightAligned < 4*256    && rightExtend < 256) return 3;
		return 5;
	}

	void IdentifierWellDefined::saveData(uint8_t *& ptr) const
	{
		if (idType == identifierType::ExAC || idType == identifierType::gnomAD) return;
		(*ptr) = static_cast<uint8_t>(expressionType) << 5;
		if        (shiftToRightAligned < 4     && rightExtend < 4  ) {
			setBits<3>(*ptr);
			(*ptr) += static_cast<uint8_t>(shiftToRightAligned << 2);
			(*ptr) += static_cast<uint8_t>(rightExtend);
			++ptr;
		} else if (shiftToRightAligned < 8*16  && rightExtend < 16 ) {
			setBits<4>(*ptr);
			(*ptr) += static_cast<uint8_t>(shiftToRightAligned >> 4);
			++ptr;
			writeUnsignedInteger<1>(ptr, (shiftToRightAligned << 4) + rightExtend);
		} else if (shiftToRightAligned < 4*256 && rightExtend < 256) {
			setBits<5>(*ptr);
			(*ptr) += static_cast<uint8_t>(shiftToRightAligned >> 8);
			++ptr;
			writeUnsignedInteger<1>(ptr, shiftToRightAligned);
			writeUnsignedInteger<1>(ptr, rightExtend);
		} else {
			++ptr;
			writeUnsignedInteger<2>(ptr, shiftToRightAligned);
			writeUnsignedInteger<2>(ptr, rightExtend);
		}
	}

	void IdentifierWellDefined::loadData(uint8_t const *& ptr)
	{
		if (idType == identifierType::ExAC || idType == identifierType::gnomAD) {
			expressionType = Type::noHgvs;
			shiftToRightAligned = rightExtend = 0;
			return;
		}
		expressionType = static_cast<Type>(*ptr >> 5);
		if (checkBits<3>(*ptr)) {
			shiftToRightAligned = ((*ptr) >> 2) % 4;
			rightExtend = (*ptr) % 4;
			++ptr;
		} else if (checkBits<4>(*ptr)) {
			shiftToRightAligned = ((*ptr) % 8) << 4;
			++ptr;
			rightExtend = readUnsignedInteger<1,uint16_t>(ptr);
			shiftToRightAligned += (rightExtend >> 4);
			rightExtend %= 16;
		} else if (checkBits<5>(*ptr)) {
			shiftToRightAligned = ((*ptr) % 4) << 8;
			++ptr;
			shiftToRightAligned += readUnsignedInteger<1,uint16_t>(ptr);
			rightExtend          = readUnsignedInteger<1,uint16_t>(ptr);
		} else {
			++ptr;
			shiftToRightAligned = readUnsignedInteger<2,uint16_t>(ptr);
			rightExtend         = readUnsignedInteger<2,uint16_t>(ptr);
		}
	}


	std::string Identifier_dbSNP::toString() const
	{
		return "(" + boost::lexical_cast<std::string>(rs) + ")";
	}
	unsigned Identifier_dbSNP::dataLength() const
	{
		return 4;
	}
	void Identifier_dbSNP::saveData(uint8_t *& ptr) const
	{
		writeUnsignedInteger(ptr, rs, 4);
	}
	void Identifier_dbSNP::loadData(uint8_t const *& ptr)
	{
		rs = readUnsignedInteger<uint32_t>(ptr, 4);
	}


	std::string Identifier_ClinVarAllele::toString() const
	{
		return "(" + boost::lexical_cast<std::string>(alleleId) + "," + preferredName + ")";
	}
	unsigned Identifier_ClinVarAllele::dataLength() const
	{
		return (3 + preferredName.size() + 1);
	}
	void Identifier_ClinVarAllele::saveData(uint8_t *& ptr) const
	{
		writeUnsignedInteger(ptr, alleleId, 3);
		for (unsigned i = 0; i < preferredName.size(); ++i) {
			uint8_t const v = static_cast<uint8_t>(preferredName[i]);
			if (v == 0) break;
			*ptr = v;
			++ptr;
		}
		*ptr = 0;
		++ptr;
	}
	void Identifier_ClinVarAllele::loadData(uint8_t const *& ptr)
	{
		alleleId = readUnsignedInteger<uint32_t>(ptr, 3);
		preferredName = "";
		for ( ;  *ptr != 0;  ++ptr ) {
			preferredName.push_back( static_cast<char>(*ptr) );
		}
		++ptr;
	}


	std::string Identifier_ClinVarVariant::toString() const
	{
		return "(" + boost::lexical_cast<std::string>(variantId) + "," + ::toString(RCVs) + ")";
	}
	unsigned Identifier_ClinVarVariant::dataLength() const
	{
		unsigned const rcvsCount = std::min(static_cast<unsigned>(RCVs.size()), 255u);
		return (3 + 1 + 3*rcvsCount);
	}
	void Identifier_ClinVarVariant::saveData(uint8_t *& ptr) const
	{
		unsigned const rcvsCount = std::min(static_cast<unsigned>(RCVs.size()), 255u);
		writeUnsignedInteger(ptr, variantId, 3);
		writeUnsignedInteger(ptr, rcvsCount, 1);
		for (unsigned i = 0; i < rcvsCount; ++i) {
			writeUnsignedInteger(ptr, RCVs[i], 3);
		}
	}
	void Identifier_ClinVarVariant::loadData(uint8_t const *& ptr)
	{
		variantId = readUnsignedInteger<uint32_t>(ptr, 3);
		RCVs.resize( readUnsignedInteger<unsigned>(ptr,1) );
		for (unsigned i = 0; i < RCVs.size(); ++i) {
			RCVs[i] = readUnsignedInteger<uint32_t>(ptr, 3);
		}
	}


	std::string Identifier_AllelicEpigenome::toString() const
	{
		return "(" + ::toString(chr) + "," + boost::lexical_cast<std::string>(ae) + ")";
	}
	unsigned Identifier_AllelicEpigenome::dataLength() const
	{
		return 4;
	}
	void Identifier_AllelicEpigenome::saveData(uint8_t *& ptr) const
	{
		uint32_t v = chr;
		v <<= 27;
		v += ae;
		writeUnsignedInteger(ptr, v, 4);
	}
	void Identifier_AllelicEpigenome::loadData(uint8_t const *& ptr)
	{
		ae = readUnsignedInteger<uint32_t>(ptr, 4);
		chr = static_cast<Chromosome>(ae >> 27);
		ae <<= 5;
		ae >>= 5;
	}


	std::string Identifier_COSMIC::toString() const
	{
		return "(" + boost::lexical_cast<std::string>(coding) + "," + boost::lexical_cast<std::string>(id) + "," + boost::lexical_cast<std::string>(active) + ")";
	}
	unsigned Identifier_COSMIC::dataLength() const
	{
		return 4;
	}
	void Identifier_COSMIC::saveData(uint8_t *& ptr) const
	{
		uint32_t v = id;
		v <<= 2;
		if (coding) v |= 2u;
		if (active) v |= 1u;
		writeUnsignedInteger(ptr, v, 4);
	}
	void Identifier_COSMIC::loadData(uint8_t const *& ptr)
	{
		id = readUnsignedInteger<uint32_t>(ptr, 4);
		coding = (id & 2u);
		active = (id & 1u);
		id >>= 2;
	}


	std::string Identifier_externalSource::toString() const
	{
		return "(" + boost::lexical_cast<std::string>(id) + ")";
	}
	unsigned Identifier_externalSource::dataLength() const
	{
		return 1;
	}
	void Identifier_externalSource::saveData(uint8_t *& ptr) const
	{
		writeUnsignedInteger<1,uint8_t>(ptr, id);
	}
	void Identifier_externalSource::loadData(uint8_t const *& ptr)
	{
		id = readUnsignedInteger<1,uint8_t>(ptr);
	}


	Identifiers::Identifiers
		( std::vector<IdentifierShort> const & shortIds
		, std::vector<IdentifierWellDefined> const & hgvsIds
		) : fShortIds(shortIds), fHgvsIds(hgvsIds)
	{
		std::sort(fShortIds.begin(), fShortIds.end());
		std::sort(fHgvsIds .begin(), fHgvsIds .end());
		fShortIds.erase( std::unique(fShortIds.begin(),fShortIds.end()), fShortIds.end() );
		fHgvsIds .erase( std::unique(fHgvsIds .begin(),fHgvsIds .end()), fHgvsIds .end() );
	}

	void Identifiers::add(IdentifierShort const & e)
	{
		auto it = std::lower_bound(fShortIds.begin(),fShortIds.end(),e);
		if (it == fShortIds.end() || *it != e) {
			fShortIds.insert(it, e);
		} else {
			*it = e;
		}
	}

	void Identifiers::add(IdentifierWellDefined const & id)
	{
		ASSERT(!isShortIdentifier(id.idType));
		auto it = std::lower_bound(fHgvsIds.begin(),fHgvsIds.end(),id);
		if (it == fHgvsIds.end() || *it != id) fHgvsIds.insert(it, id);
	}

	void Identifiers::add(Identifiers const & ids)
	{
		Identifiers r;
		r.fShortIds = set_union(ids.fShortIds, fShortIds);
		r.fHgvsIds  = set_union(ids.fHgvsIds , fHgvsIds );
		*this = std::move(r);
	}

	void Identifiers::remove(Identifiers const & ids)
	{
		Identifiers r;
		r.fShortIds = set_difference(fShortIds, ids.fShortIds);
		r.fHgvsIds  = set_difference(fHgvsIds , ids.fHgvsIds );
		*this = std::move(r);
	}

	bool Identifiers::has(identifierType type, uint32_t value) const
	{
		ASSERT(isShortIdentifier(type));
		std::vector<std::pair<identifierType,uint32_t>> ids;
		for (auto const & e: fShortIds) {
			e.saveIdsToContainer(ids);
			for (auto const & id: ids) if (id.first == type && id.second == value) return true;
			ids.clear();
		}
		return false;
	}

	std::vector<IdentifierShort> Identifiers::getShortIds(identifierType type) const
	{
		std::vector<IdentifierShort> r;
		for (auto const & vi: fShortIds) if (vi.fIdType == type) r.push_back(vi);
		return r;
	}

	std::vector<IdentifierWellDefined> Identifiers::getHgvsIds(identifierType type) const
	{
		std::vector<IdentifierWellDefined> r;
		for (auto const & vi: fHgvsIds) if (vi.idType == type) r.push_back(vi);
		return r;
	}

	std::string Identifiers::toString() const
	{
		std::string s = "[";
		for (unsigned i = 0; i < fShortIds.size(); ++i) {
			if (s.size() > 1) s += ",";
			s += fShortIds[i].toString();
		}
		s += "]";
		return s;
	}

