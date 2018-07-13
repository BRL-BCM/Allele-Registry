#include "TableProtein.hpp"
#include "tools.hpp"
#include "../apiDb/TasksManager.hpp"
#include "../commonTools/bytesLevel.hpp"
#include <iostream>


uint16_t getLengthParam(unsigned val, variantCategory vc, unsigned regionLength)
{
	unsigned min = 0;
	unsigned max = 7;
	switch (vc) {
		case variantCategory::nonShiftable      : break;
		case variantCategory::shiftableInsertion: min = 1; break;
		case variantCategory::duplication       : min = 1; max = regionLength; break;
		case variantCategory::shiftableDeletion : min = 1; max = regionLength-1; break;
	}
	return ((val-min) % (max-min+1) + min);
}

int main(int argc, char ** argv)
{
	std::atomic<uint32_t> nextFreeCaId(1);
	TasksManager taskManager(1);
	TasksManager taskManager2(1);
	TableProtein * tab = new TableProtein("./dbTest/", &taskManager, &taskManager2, 128, nextFreeCaId);

	std::vector<RecordProteinVariant*> records1;
	std::vector<RecordProteinVariant*> records2;
	std::vector<RecordProteinVariant*> records3;

	for (unsigned i = 0; i < 1000000; ++i) {

		std::vector<BinaryAminoAcidSequenceModification> rawDef(1);
		BinaryAminoAcidSequenceModification & sr = rawDef.front();
		sr.position = i % (15*256);
		sr.category = static_cast<variantCategory>(i%4);
		sr.lengthBefore = (i % 25) + 2;
		sr.lengthChangeOrSeqLength = getLengthParam(i%67,sr.category,sr.lengthBefore);
		if (sr.category == variantCategory::nonShiftable || sr.category == variantCategory::shiftableInsertion) {
			sr.sequence = i;
			shiftLeftInPlace (sr.sequence, 8*(4-lengthOfBinaryAminoAcidSequence(sr.lengthChangeOrSeqLength))+7);
			shiftRightInPlace(sr.sequence, 8*(4-lengthOfBinaryAminoAcidSequence(sr.lengthChangeOrSeqLength))+7);
		}
		if (i % 17 == 3) {
			for (unsigned j = 0; j < i % 4; ++j) {
				BinaryAminoAcidSequenceModification sr;
				sr.position = i % (15*256) + 30*(j+1);
				sr.category = static_cast<variantCategory>(i%4);
				sr.lengthBefore = (i % 25) + 2;
				sr.lengthChangeOrSeqLength = getLengthParam(i+j,sr.category,sr.lengthBefore);
				if (sr.category == variantCategory::nonShiftable || sr.category == variantCategory::shiftableInsertion) {
					sr.sequence = i;
					shiftLeftInPlace (sr.sequence, 8*(4-lengthOfBinaryAminoAcidSequence(sr.lengthChangeOrSeqLength))+7);
					shiftRightInPlace(sr.sequence, 8*(4-lengthOfBinaryAminoAcidSequence(sr.lengthChangeOrSeqLength))+7);
				}
				rawDef.push_back(sr);
			}
		}
		RecordProteinVariant * r = new RecordProteinVariant( BinaryProteinVariantDefinition(i,rawDef) );
		r->revision = i;
		records1.push_back(r);
		records2.push_back(new RecordProteinVariant(*r));
		records3.push_back(new RecordProteinVariant(*r));
		records1.back()->identifiers.lastId() = i;
		records2.back()->identifiers.add( Identifier_dbSNP(i+1000000) );
		records3.back()->identifiers.lastId() = i;
		records3.back()->identifiers.add( Identifier_dbSNP(i+1000000) );
	}

	std::map<identifierType,std::vector<std::pair<uint32_t,RecordVariantPtr>>> changesInIndexes;
	tab->fetchAndAdd(records1, changesInIndexes);
	if (changesInIndexes.size() != 1 || changesInIndexes.at(identifierType::PA).size() != records1.size()) throw std::logic_error("Assertion 1");

	changesInIndexes.clear();
	tab->fetchAndAdd(records2, changesInIndexes);
//	std::cout << records2.size() << " " << changesInIndexes.at(identifierType::dbSNP).size() << " " << changesInIndexes.at(identifierType::PA).size() << std::endl;
	if (changesInIndexes.size() != 1 || changesInIndexes.at(identifierType::dbSNP).size() != records1.size()) throw std::logic_error("Assertion 2");

	for (unsigned i = 0; i < records2.size(); ++i) {
		if (records2[i]->identifiers != records3[i]->identifiers) throw std::logic_error("Assertion 3");
	}

	std::vector<RecordProteinVariant*> records4;
	auto callback = [&records4](std::vector<RecordProteinVariant*> & records, bool & lastCall)
	{
		records4.insert( records4.end(), records.begin(), records.end() );
		records.clear();
	};
	unsigned skip = 0;
	tab->query(callback, skip);

	if (records3.size() != records4.size()) {
		std::cerr << "ERROR: incorrect records count: " << records3.size() << " != " <<  records4.size() << std::endl;
	}

	for (unsigned i = 0; i < records3.size(); ++i) {
		try {
			if (records3[i]->definition  != records4[i]->definition ) throw std::logic_error("Assertion 4");
			if (records3[i]->identifiers != records4[i]->identifiers) throw std::logic_error("Assertion 6");
		} catch (std::exception const & e) {
			std::cerr << "EXCEPTION: " << e.what() << " i=" << i << " " << records3[i]->definition.toString() << " " << records4[i]->definition.toString() << std::endl;
			break;
		}
	}

	delete tab;

	std::cout << "OK!" << std::endl;

	return 0;
}
