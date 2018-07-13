#include "RecordVariant.hpp"
#include "tools.hpp"
#include "../commonTools/bytesLevel.hpp"
#include "../commonTools/setsOperators.hpp"
#include <boost/lexical_cast.hpp>

	// definition has at least one element (with position)
	inline bool isShortVariant(std::vector<BinaryNucleotideSequenceModification> const & definition)
	{
		if ( definition.size() > 1 ) return false;
		BinaryNucleotideSequenceModification const & sr = definition.front();
		switch (sr.category) {
			case variantCategory::nonShiftable:
				switch (sr.lengthChangeOrSeqLength) {
					case 0: return (sr.lengthBefore > 0 && sr.lengthBefore < 5);
					case 1:
					case 2: return (sr.lengthBefore < 2);
					case 3: return (sr.lengthBefore == 0);
				}
				return false;
			case  variantCategory::shiftableDeletion:
				return (sr.lengthBefore == sr.lengthChangeOrSeqLength + 1 && sr.lengthChangeOrSeqLength < 5);
			case variantCategory::duplication:
				return (sr.lengthChangeOrSeqLength < 3 && sr.lengthBefore < sr.lengthChangeOrSeqLength + 4 );
			case variantCategory::shiftableInsertion:
				return (sr.lengthBefore < 3 && sr.lengthChangeOrSeqLength == 1);
		}
		return false;
	}

	BinaryGenomicVariantDefinition::BinaryGenomicVariantDefinition(uint32_t firstPosition) : simpleVariants(1)
	{
		simpleVariants[0].position = firstPosition;
	}

	BinaryGenomicVariantDefinition::BinaryGenomicVariantDefinition(std::vector<BinaryNucleotideSequenceModification> & definition)
	{
		if (definition.empty()) throw std::logic_error("BinaryVariantDefinition: empty definition");
		std::sort(definition.begin(), definition.end());
		for ( unsigned i = 0;  i < definition.size();  ++i ) {
			BinaryNucleotideSequenceModification const & def = definition[i];
			switch (def.category) {
				case variantCategory::nonShiftable:
					if ( shiftRight(def.sequence, 2 * def.lengthChangeOrSeqLength) != 0 ) throw std::logic_error("BinaryVariantDefinition: incorrect sequence");
					break;
				case variantCategory::shiftableInsertion:
					if ( def.lengthBefore == 0 || def.lengthChangeOrSeqLength == 0 ) throw std::logic_error("BinaryVariantDefinition: incorrect parameters");
					if ( shiftRight(def.sequence, 2 * def.lengthChangeOrSeqLength) != 0 ) throw std::logic_error("BinaryVariantDefinition: incorrect sequence");
					break;
				case variantCategory::duplication:
					if ( def.lengthBefore == 0 || def.lengthChangeOrSeqLength > def.lengthBefore ) throw std::logic_error("BinaryVariantDefinition: incorrect parameters");
					if ( def.sequence != 0 ) throw std::logic_error("BinaryVariantDefinition: incorrect sequence");
					break;
				case variantCategory::shiftableDeletion:
					if ( def.lengthBefore <= def.lengthChangeOrSeqLength || def.lengthChangeOrSeqLength == 0 ) throw std::logic_error("BinaryVariantDefinition: incorrect parameters");
					if ( def.sequence != 0 ) throw std::logic_error("BinaryVariantDefinition: incorrect sequence");
					break;
			}
			if ( (i > 0) && (definition[i-1].position >= definition[i].position) ) {
				throw std::logic_error("BinaryVariantDefinition: simple definitions with overlapping regions");
			}
		}
		simpleVariants.swap(definition);
	}

	// definition has at least one element (with position)
	unsigned BinaryGenomicVariantDefinition::dataLength() const
	{
		std::vector<BinaryNucleotideSequenceModification> const & definition = this->simpleVariants;
		if (isShortVariant(definition)) return 1;
		unsigned length = 0;
		for (auto const & vr2: definition) {
			length += 4;  // position
			length += 4;
			if (vr2.category == variantCategory::nonShiftable || vr2.category == variantCategory::shiftableInsertion) {
				length += std::min( (vr2.lengthChangeOrSeqLength + 3) / 4,  4 );
			}
		}
		length -= 4; // first position is save as a key
		return length;
	}

	// definition has at least one element (with position)
	void BinaryGenomicVariantDefinition::saveData(uint8_t *& ptr) const
	{
		std::vector<BinaryNucleotideSequenceModification> const & definition = this->simpleVariants;
		// ================================ short variants
		if (isShortVariant(definition)) {
			BinaryNucleotideSequenceModification const & sr = definition.front();
			*ptr = static_cast<uint8_t>(0);
			switch (sr.category) {
				case variantCategory::nonShiftable:
					switch (sr.lengthChangeOrSeqLength) {
						case 0:
							setBits<1,2,4>(*ptr);
							setValue<6,2>(*ptr, sr.lengthBefore-1);
							break;
						case 1:
							setBits<1,2>(*ptr);
							setValue<5,1>(*ptr, sr.lengthBefore);
							setValue<6,2>(*ptr, sr.sequence);
							break;
						case 2:
							setBits<1>(*ptr);
							setValue<3,1>(*ptr, sr.lengthBefore);
							setValue<4,4>(*ptr, sr.sequence);
							break;
						case 3:
							setValue<2,6>(*ptr, sr.sequence);
							break;
					}
					break;
				case variantCategory::shiftableDeletion:
					setBits<1,2,4,5>(*ptr);
					setValue<6,2>(*ptr, sr.lengthChangeOrSeqLength-1);
					break;
				case variantCategory::duplication:
					setBits<1,2,3>(*ptr);
					setValue<5,1>(*ptr, sr.lengthChangeOrSeqLength-1);
					setValue<6,2>(*ptr, sr.lengthBefore-sr.lengthChangeOrSeqLength);
					break;
				case variantCategory::shiftableInsertion:
					setBits<1,2,3,4>(*ptr);
					setValue<5,1>(*ptr, sr.lengthBefore-1);
					setValue<6,2>(*ptr, sr.sequence);
					break;
			}
			++ptr;
			return;
		}
		// =================================== standard variants
		// --- complex alleles
		for (auto const & vr2: definition) {
			// --- position
			if (vr2.position != definition.front().position) writeUnsignedInteger<4>(ptr, vr2.position);
			// --- definition
			uint32_t def = 0;
			setBits<0>(def);
			if (vr2.position != definition.back().position) setBits<1>(def);
			setValue<2,2>(def, vr2.category);
			if (vr2.category == variantCategory::shiftableDeletion) {
				setValue<4,14>(def, vr2.lengthBefore - vr2.lengthChangeOrSeqLength);
			} else {
				setValue<4,14>(def, vr2.lengthBefore);
			}
			setValue<18,14>(def, vr2.lengthChangeOrSeqLength);
			writeUnsignedInteger<4>(ptr, def);
			// --- sequence
			if (vr2.category == variantCategory::nonShiftable || vr2.category == variantCategory::shiftableInsertion) {
				unsigned const seqLength = std::min( (vr2.lengthChangeOrSeqLength + 3) / 4,  4 );
				writeUnsignedInteger(ptr, vr2.sequence, seqLength);
			}
		}
	}

	// vr exists and is empty (just constructed)
	// definition has at least one element (with position)
	void BinaryGenomicVariantDefinition::loadData(uint8_t const *& ptr)
	{
		this->simpleVariants.resize(1);
		this->simpleVariants[0].sequence = 0;
		std::vector<BinaryNucleotideSequenceModification> & definition = this->simpleVariants;
		// ================================ short variants
		if ( ! checkBits<0>(*ptr) ) {
			uint8_t const byte = *ptr;
			BinaryNucleotideSequenceModification & sr = definition.front();
			++ptr;
			if ( ! checkBits<1>(byte) ) {
				sr.category = variantCategory::nonShiftable;
				sr.lengthBefore = 0;
				sr.lengthChangeOrSeqLength = 3;
				sr.sequence = getValue<2,6>(byte);
				return;
			}
			if ( ! checkBits<2>(byte) ) {
				sr.category = variantCategory::nonShiftable;
				sr.lengthBefore = getValue<3,1>(byte);
				sr.lengthChangeOrSeqLength = 2;
				sr.sequence = getValue<4,4>(byte);
				return;
			}
			if ( ! checkBits<3>(byte) ) {
				if ( ! checkBits<4>(byte) ) {
					sr.category = variantCategory::nonShiftable;
					sr.lengthBefore = getValue<5,1>(byte);
					sr.lengthChangeOrSeqLength = 1;
					sr.sequence = getValue<6,2>(byte);
					return;
				}
				if ( ! checkBits<5>(byte) ) {
					sr.category = variantCategory::nonShiftable;
					sr.lengthBefore = getValue<6,2>(byte) + 1;
					sr.lengthChangeOrSeqLength = 0;
					return;
				}
				sr.category = variantCategory::shiftableDeletion;
				sr.lengthChangeOrSeqLength = getValue<6,2>(byte) + 1;
				sr.lengthBefore = sr.lengthChangeOrSeqLength + 1;
				return;
			}
			if ( ! checkBits<4>(byte) ) {
				sr.category = variantCategory::duplication;
				sr.lengthChangeOrSeqLength = getValue<5,1>(byte) + 1;
				sr.lengthBefore = getValue<6,2>(byte) + sr.lengthChangeOrSeqLength;
				return;
			}
			sr.category = variantCategory::shiftableInsertion;
			sr.lengthBefore = getValue<5,1>(byte) + 1;
			sr.lengthChangeOrSeqLength = 1;
			sr.sequence = getValue<6,2>(byte);
			return;
		}
		// =================================== standard variants
		for ( bool hasNextDef = true;  hasNextDef;  ) {
			BinaryNucleotideSequenceModification & vr2 = definition.back();
			// --- position
			if (definition.size() > 1) vr2.position = readUnsignedInteger<4,uint32_t>(ptr);
			// --- definition
			uint32_t const def = readUnsignedInteger<4,uint32_t>(ptr);
			hasNextDef = checkBits<1>(def);
			vr2.category = static_cast<variantCategory>(getValue<2,2>(def));
			vr2.lengthBefore = getValue<4,14>(def);
			vr2.lengthChangeOrSeqLength = getValue<18,14>(def);
			if (vr2.category == variantCategory::shiftableDeletion) vr2.lengthBefore += vr2.lengthChangeOrSeqLength;
			// --- sequence
			if (vr2.category == variantCategory::nonShiftable || vr2.category == variantCategory::shiftableInsertion) {
				unsigned const seqLength = std::min( (vr2.lengthChangeOrSeqLength + 3) / 4,  4 );
				vr2.sequence = readUnsignedInteger<uint32_t>(ptr, seqLength);
			}
			// --- next part
			if (hasNextDef) definition.resize( definition.size()+1 );
		}
	}


	std::string BinaryGenomicVariantDefinition::toString() const
	{
		std::string s = "[";
		for (auto const & m: simpleVariants) {
			s += "(";
			s += boost::lexical_cast<std::string>(m.position) + "+" + boost::lexical_cast<std::string>(m.lengthBefore);
			s += "," + boost::lexical_cast<std::string>(m.category);
			s += "," + boost::lexical_cast<std::string>(m.lengthChangeOrSeqLength);
			s += "," + boost::lexical_cast<std::string>(m.sequence);
			s += "),";
		}
		if (s.size() > 1) s.pop_back();
		s += "]";
		return s;
	}


	unsigned RecordGenomicVariant::dataLength() const
	{
		unsigned length = this->definition.dataLength();
//for (auto & c:complexDefinition) std::cout << static_cast<unsigned>(c.category) << " " << c.lengthChangeOrSeqLength << std::endl;
//std::cout << "lenght 1=" << length << std::endl;
		// revision
		length += lengthUnsignedIntVarSize<2,1>(revision);
//std::cout << "lenght 2=" << length << std::endl;
		// identifiers
		length += identifiers.dataLength();
//std::cout << "lenght 3=" << length << std::endl;
		return length;
	}

	void RecordGenomicVariant::saveData(uint8_t *& ptr) const
	{
//uint8_t * const p = ptr;
//		std::cout << "BEGIN - RecordGenomicVariant" << std::endl;
		this->definition.saveData(ptr);
//std::cout << definition.toString() << "\n";
//std::cout << "Written 1=" << (ptr-p) << "\n";
		writeUnsignedIntVarSize<2,1>(ptr,revision);
//std::cout << "Written 2=" << (ptr-p) << "\n";
		identifiers.saveData(ptr);
//std::cout << "Written 3=" << (ptr-p) << "\n";
//		std::cout << "END - RecordGenomicVariant" << std::endl;
	}

	void RecordGenomicVariant::loadData(uint8_t const *& ptr)
	{
		this->definition.loadData(ptr);
		revision = readUnsignedIntVarSize<2,1,uint32_t>(ptr);
		this->identifiers.loadData(ptr);
	}

	// ========================== Protein records - TODO

	// definition has at least one element (with position)
	inline bool isShortVariant(std::vector<BinaryAminoAcidSequenceModification> const & definition)
	{
		if ( definition.size() > 1 ) return false;
		BinaryAminoAcidSequenceModification const & sr = definition.front();
		switch (sr.category) {
			case variantCategory::nonShiftable:
				switch (sr.lengthChangeOrSeqLength) {
					case 0: return (sr.lengthBefore <= 8);
					case 1: return (sr.lengthBefore <= 2);
				}
				return false;
			case  variantCategory::shiftableDeletion:
				return (sr.lengthBefore == sr.lengthChangeOrSeqLength + 1 && sr.lengthChangeOrSeqLength <= 8);
			case variantCategory::duplication:
				return (sr.lengthChangeOrSeqLength <= 4 && sr.lengthBefore < sr.lengthChangeOrSeqLength + 4 );
			case variantCategory::shiftableInsertion:
				break;
		}
		return false;
	}

	BinaryProteinVariantDefinition::BinaryProteinVariantDefinition(uint64_t proteinIdAndFirstPosition) : simpleVariants(1)
	{
		fProteinAccessionIdentifier = (proteinIdAndFirstPosition >> 16);
		simpleVariants[0].position = proteinIdAndFirstPosition % (256 * 256);
	}

	BinaryProteinVariantDefinition::BinaryProteinVariantDefinition(uint64_t proteinAccId, std::vector<BinaryAminoAcidSequenceModification> & definition)
	: fProteinAccessionIdentifier(proteinAccId)
	{
		if (definition.empty()) throw std::logic_error("BinaryVariantDefinition: empty definition");
		std::sort(definition.begin(), definition.end());
		for ( unsigned i = 0;  i < definition.size();  ++i ) {
			BinaryAminoAcidSequenceModification const & def = definition[i];
			switch (def.category) {
				case variantCategory::nonShiftable:
//TODO - change to AA	if ( shiftRight(def.sequence, 2 * def.lengthChangeOrSeqLength) != 0 ) throw std::logic_error("BinaryVariantDefinition: incorrect sequence");
					break;
				case variantCategory::shiftableInsertion:
					if ( def.lengthBefore == 0 || def.lengthChangeOrSeqLength == 0 ) throw std::logic_error("BinaryVariantDefinition: incorrect parameters");
//TODO - change to AA	if ( shiftRight(def.sequence, 2 * def.lengthChangeOrSeqLength) != 0 ) throw std::logic_error("BinaryVariantDefinition: incorrect sequence");
					break;
				case variantCategory::duplication:
					if ( def.lengthBefore == 0 || def.lengthChangeOrSeqLength > def.lengthBefore ) throw std::logic_error("BinaryVariantDefinition: incorrect parameters");
					if ( def.sequence != 0 ) throw std::logic_error("BinaryVariantDefinition: incorrect sequence");
					break;
				case variantCategory::shiftableDeletion:
					if ( def.lengthBefore <= def.lengthChangeOrSeqLength || def.lengthChangeOrSeqLength == 0 ) throw std::logic_error("BinaryVariantDefinition: incorrect parameters");
					if ( def.sequence != 0 ) throw std::logic_error("BinaryVariantDefinition: incorrect sequence");
					break;
			}
			if ( (i > 0) && (definition[i-1].position >= definition[i].position) ) {
				throw std::logic_error("BinaryVariantDefinition: simple definitions with overlapping regions");
			}
		}
		simpleVariants.swap(definition);
	}

	// definition has at least one element (with position)
	unsigned BinaryProteinVariantDefinition::dataLength() const
	{
		std::vector<BinaryAminoAcidSequenceModification> const & definition = this->simpleVariants;
		if (isShortVariant(definition)) return 1;
		unsigned length = 0;
		for (auto const & vr2: definition) {
			length += 2;  // position
			length += 4;  // variant description (1 + 1.5 + 1.5)
			if (vr2.category == variantCategory::nonShiftable || vr2.category == variantCategory::shiftableInsertion) {
				length += std::min( lengthOfBinaryAminoAcidSequence(vr2.lengthChangeOrSeqLength), 4u );
			}
		}
		length -= 2; // the first position is a part of the key
		return length;
	}

	// definition has at least one element (with position)
	void BinaryProteinVariantDefinition::saveData(uint8_t *& ptr) const
	{
		std::vector<BinaryAminoAcidSequenceModification> const & definition = this->simpleVariants;
//for (auto x: definition) std::cout << "|" << (int)(x.category) << "," << x.position << "," << x.lengthBefore << "," << x.lengthChangeOrSeqLength;
//std::cout << std::endl;
		// ================================ short variants
		if (isShortVariant(definition)) {
//			std::cout << "BEGIN2 " << std::endl;
			BinaryAminoAcidSequenceModification const & sr = definition.front();
			*ptr = static_cast<uint8_t>(0);
			switch (sr.category) {
				case variantCategory::nonShiftable:
					if (sr.lengthChangeOrSeqLength) {  // == 1
						switch (sr.lengthBefore) {
							case 1: setBits<2>(*ptr); break;
							case 2: setBits<1>(*ptr); break;
						}
						setValue<3,5>(*ptr, sr.sequence);
					} else {
						setBits<1,2>(*ptr);
						setValue<5,3>(*ptr, sr.lengthBefore-1);
					}
					break;
				case variantCategory::shiftableDeletion:
					setBits<1,2,4>(*ptr);
					setValue<5,3>(*ptr, sr.lengthChangeOrSeqLength-1);
					break;
				case variantCategory::duplication:
					setBits<1,2,3>(*ptr);
					setValue<4,2>(*ptr, sr.lengthChangeOrSeqLength-1);
					setValue<6,2>(*ptr, sr.lengthBefore-sr.lengthChangeOrSeqLength);
					break;
			}
			++ptr;
//			std::cout << "END2 " << std::endl;
			return;
		}
		// =================================== standard variants
//		std::cout << "BEGIN1 " << std::endl;
		// --- complex alleles
		for (auto const & vr2: definition) {
			// --- position
			if (vr2.position != definition.front().position) writeUnsignedInteger<2>(ptr, vr2.position);
			// --- definition
			uint32_t def = 0;
			setBits<0>(def);
			if (vr2.position != definition.back().position) setBits<1>(def);
			setValue<6,2>(def, vr2.category);
			if (vr2.category == variantCategory::shiftableDeletion) {
				setValue<8,12>(def, vr2.lengthBefore - vr2.lengthChangeOrSeqLength);
			} else {
				setValue<8,12>(def, vr2.lengthBefore);
			}
			setValue<20,12>(def, vr2.lengthChangeOrSeqLength);
			writeUnsignedInteger<4>(ptr, def);
			// --- sequence
			if (vr2.category == variantCategory::nonShiftable || vr2.category == variantCategory::shiftableInsertion) {
				unsigned const seqLength = lengthOfBinaryAminoAcidSequence(vr2.lengthChangeOrSeqLength);
				writeUnsignedInteger(ptr, vr2.sequence, seqLength);
			}
		}
//		std::cout << "END1 " << std::endl;
	}

	// vr exists and is empty (just constructed)
	// definition has at least one element (with position)
	void BinaryProteinVariantDefinition::loadData(uint8_t const *& ptr)
	{
		this->simpleVariants.resize(1);
		this->simpleVariants[0].sequence = 0;
		std::vector<BinaryAminoAcidSequenceModification> & definition = this->simpleVariants;
		// ================================ short variants
		if ( ! checkBits<0>(*ptr) ) {
			uint8_t const byte = *ptr;
			BinaryAminoAcidSequenceModification & sr = definition.front();
			++ptr;
			if ( ! checkBits<1>(byte) ) {
				sr.category = variantCategory::nonShiftable;
				sr.lengthBefore = getValue<2,1>(byte);
				sr.lengthChangeOrSeqLength = 1;
				sr.sequence = getValue<3,5>(byte);
				return;
			} else {
				if ( ! checkBits<2>(byte) ) {
					sr.category = variantCategory::nonShiftable;
					sr.lengthBefore = 2;
					sr.lengthChangeOrSeqLength = 1;
					sr.sequence = getValue<3,5>(byte);
					return;
				} else {
					if ( ! checkBits<3>(byte) ) {
						if ( ! checkBits<4>(byte) ) {
							sr.category = variantCategory::nonShiftable;
							sr.lengthBefore = getValue<5,3>(byte) + 1;
							sr.lengthChangeOrSeqLength = 0;
							return;
						} else {
							sr.category = variantCategory::shiftableDeletion;
							sr.lengthChangeOrSeqLength = getValue<5,3>(byte) + 1;
							sr.lengthBefore = sr.lengthChangeOrSeqLength + 1;
							return;
						}
					} else {
						sr.category = variantCategory::duplication;
						sr.lengthChangeOrSeqLength = getValue<4,2>(byte) + 1;
						sr.lengthBefore = getValue<6,2>(byte) + sr.lengthChangeOrSeqLength;
						return;
					}
				}
			}
		}
		// =================================== standard variants
		for ( bool hasNextDef = true;  hasNextDef;  ) {
			BinaryAminoAcidSequenceModification & vr2 = definition.back();
			// --- position
			if (definition.size() > 1) vr2.position = readUnsignedInteger<2,uint32_t>(ptr);
			// --- definition
			uint32_t const def = readUnsignedInteger<4,uint32_t>(ptr);
			hasNextDef = checkBits<1>(def);
			vr2.category = static_cast<variantCategory>(getValue<6,2>(def));
			vr2.lengthBefore = getValue<8,12>(def);
			vr2.lengthChangeOrSeqLength = getValue<20,12>(def);
			if (vr2.category == variantCategory::shiftableDeletion) vr2.lengthBefore += vr2.lengthChangeOrSeqLength;
			// --- sequence
			if (vr2.category == variantCategory::nonShiftable || vr2.category == variantCategory::shiftableInsertion) {
				unsigned const seqLength = std::min( lengthOfBinaryAminoAcidSequence(vr2.lengthChangeOrSeqLength), 4u );
				vr2.sequence = readUnsignedInteger<uint32_t>(ptr, seqLength);
			}
			// --- next part
			if (hasNextDef) definition.resize( definition.size()+1 );
		}
	}


	std::string BinaryProteinVariantDefinition::toString() const
	{
		std::string s = "[" + boost::lexical_cast<std::string>(fProteinAccessionIdentifier) + ",";
		for (auto const & m: simpleVariants) {
			s += "(";
			s += boost::lexical_cast<std::string>(m.position) + "+" + boost::lexical_cast<std::string>(m.lengthBefore);
			s += "," + boost::lexical_cast<std::string>(m.category);
			s += "," + boost::lexical_cast<std::string>(m.lengthChangeOrSeqLength);
			s += "," + boost::lexical_cast<std::string>(m.sequence);
			s += "),";
		}
		if (s.size() > 1) s.pop_back();
		s += "]";
		return s;
	}


	unsigned RecordProteinVariant::dataLength() const
	{
		unsigned length = this->definition.dataLength();
//for (auto & c:complexDefinition) std::cout << static_cast<unsigned>(c.category) << " " << c.lengthChangeOrSeqLength << std::endl;
//std::cout << "lenght 1=" << length << std::endl;
		// revision
		length += lengthUnsignedIntVarSize<2,1>(revision);
//std::cout << "lenght 2=" << length << std::endl;
		// identifiers
		length += identifiers.dataLength();
//std::cout << "lenght 3=" << length << std::endl;
		return length;
	}

	void RecordProteinVariant::saveData(uint8_t *& ptr) const
	{
//std::cout << "BEGIN - RecordProteinVariant" << std::endl;
//uint8_t * const p = ptr;
		this->definition.saveData(ptr);
//for (auto & c:complexDefinition) std::cout << static_cast<unsigned>(c.category) << " " << c.lengthChangeOrSeqLength << std::endl;
//std::cout << "Written 1=" << (ptr-p) << std::endl;
		writeUnsignedIntVarSize<2,1>(ptr,revision);
//std::cout << "Written 2=" << (ptr-p) << std::endl;
		identifiers.saveData(ptr);
//std::cout << "Written 3=" << (ptr-p) << std::endl;
//std::cout << "END - RecordProteinVariant" << std::endl;
	}

	void RecordProteinVariant::loadData(uint8_t const *& ptr)
	{
		this->definition.loadData(ptr);
		revision = readUnsignedIntVarSize<2,1,uint32_t>(ptr);
		this->identifiers.loadData(ptr);
	}


	// ========================== Binary Identifiers

	BinaryIdentifiers::BinaryIdentifiers( identifierType lastIdType
										, std::vector<IdentifierShort> const & shortIds
										, std::vector<IdentifierWellDefined> const & hgvsIds
										) : fLastIdType(lastIdType), fShortIds(shortIds), fHgvsIds(hgvsIds)
	{
		std::sort( fShortIds.begin(), fShortIds.end() );
		std::sort( fHgvsIds .begin(), fHgvsIds .end() );
		fShortIds.erase( std::unique(fShortIds.begin(),fShortIds.end()), fShortIds.end() );
		fHgvsIds .erase( std::unique(fHgvsIds .begin(),fHgvsIds .end()), fHgvsIds .end() );
	}

	unsigned BinaryIdentifiers::dataLength() const
	{
		unsigned length = fShortIds.size() + fHgvsIds.size();
		for (auto const & id: fShortIds) length += id.dataLength();
		for (auto const & id: fHgvsIds ) length += id.dataLength();
		return (length + 1 + 4);
	}

	void BinaryIdentifiers::saveData(uint8_t *& ptr) const
	{
		for (auto const & id: fShortIds) {
			writeUnsignedInteger<1>(ptr, static_cast<uint8_t>(id.fIdType));
			id.saveData(ptr);
		}
		for (auto const & id: fHgvsIds) {
			writeUnsignedInteger<1>( ptr, static_cast<uint8_t>(id.idType) );
			id.saveData(ptr);
		}
		writeUnsignedInteger<1>(ptr, static_cast<uint8_t>(fLastIdType));
		writeUnsignedInteger<4>(ptr, fLastId);
	}

	void BinaryIdentifiers::loadData(uint8_t const *& ptr)
	{
		clear();
		while (true) {
			identifierType const idType = static_cast<identifierType>(readUnsignedInteger<1,uint8_t>(ptr));
			if (idType == fLastIdType) {
				fLastId = readUnsignedInteger<4,uint32_t>(ptr);
				break;
			}
			if (isShortIdentifier(idType)) {
				fShortIds.push_back(IdentifierShort(idType));
				fShortIds.back().loadData(ptr);
			} else {
				fHgvsIds.push_back(IdentifierWellDefined());
				fHgvsIds.back().idType = idType;
				fHgvsIds.back().loadData(ptr);
			}
		}
		std::sort(fShortIds.begin(),fShortIds.end());
		std::sort(fHgvsIds .begin(),fHgvsIds .end());
	}

	// check if there is at least one identifier with type belonging to given vector
	bool BinaryIdentifiers::hasOneOfIds(std::vector<identifierType> idTypes) const
	{
		std::sort(idTypes.begin(),idTypes.end());
		auto iType = idTypes.begin();
		auto const iTypeEnd = idTypes.end();
		auto iId1 = fShortIds.begin();
		auto const iId1End = fShortIds.end();
		while (iType != iTypeEnd && iId1 != iId1End) {
			if (*iType < iId1->fIdType) {
				++iType;
			} else if (*iType > iId1->fIdType) {
				++iId1;
			} else {
				return true;
			}
		}
		iType = idTypes.begin();
		auto iId2 = fHgvsIds.begin();
		auto const iId2End = fHgvsIds.end();
		while (iType != iTypeEnd && iId2 != iId2End) {
			if (*iType < iId2->idType) {
				++iType;
			} else if (*iType > iId2->idType) {
				++iId1;
			} else {
				return true;
			}
		}
		return false;
	}

	// add short id
	void BinaryIdentifiers::add(IdentifierShort const & e)
	{
		auto it = std::lower_bound(fShortIds.begin(),fShortIds.end(),e);
		if (it == fShortIds.end() || *it != e) {
			fShortIds.insert(it,e);
		} else {
			*it = e;
		}
	}

	// exchange identifiers between list, at the end both list are equal
	void BinaryIdentifiers::exchange(BinaryIdentifiers & ids)
	{
		fShortIds = set_union(fShortIds, ids.fShortIds);
		fHgvsIds  = set_union(fHgvsIds , ids.fHgvsIds );
		ids.fShortIds = fShortIds;
		ids.fHgvsIds  = fHgvsIds;
	}

	// add identifiers given in another list, returns list of new (added) identifiers
	BinaryIdentifiers BinaryIdentifiers::add(BinaryIdentifiers const & ids)
	{
		BinaryIdentifiers r(fLastIdType);
		r.fShortIds = set_difference(ids.fShortIds, fShortIds);
		r.fHgvsIds  = set_difference(ids.fHgvsIds , fHgvsIds );
		fShortIds = set_union(fShortIds, r.fShortIds);
		fHgvsIds  = set_union(fHgvsIds , r.fHgvsIds );
		return r;
	}

	// remove identifiers given in another list, returns list of matched (deleted) identifiers
	BinaryIdentifiers BinaryIdentifiers::remove(BinaryIdentifiers const & ids)
	{
		BinaryIdentifiers r(fLastIdType);
		r.fShortIds = set_intersection(fShortIds, ids.fShortIds);
		r.fHgvsIds  = set_intersection(fHgvsIds , ids.fHgvsIds );
		fShortIds = set_difference(fShortIds, r.fShortIds);
		fHgvsIds  = set_difference(fHgvsIds , r.fHgvsIds );
		return r;
	}

	// remove identifiers of given type
	BinaryIdentifiers BinaryIdentifiers::remove(identifierType idType)
	{
		BinaryIdentifiers r(fLastIdType);
		for (auto & i: fShortIds) if (i.fIdType == idType) r.fShortIds.push_back(i);
		for (auto & i: fHgvsIds ) if (i.idType  == idType) r.fHgvsIds .push_back(i);
		fShortIds = set_difference(fShortIds, r.fShortIds);
		fHgvsIds  = set_difference(fHgvsIds , r.fHgvsIds );
		return r;
	}

	// save short ids to given map
	void BinaryIdentifiers::saveShortIdsToContainer
		( std::map<identifierType,std::vector<std::pair<uint32_t,RecordVariantPtr>>> & container, RecordVariantPtr record ) const
	{
		std::vector<std::pair<identifierType,uint32_t>> ids;
		for (auto const & e: fShortIds) e.saveIdsToContainer(ids);
		for (auto const & e: ids) container[e.first].push_back(std::make_pair(e.second,record));
	}
