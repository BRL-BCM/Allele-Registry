#include "IndexIdentifierCa.hpp"
#include "../apiDb/TasksManager.hpp"
#include "../flatDb/Convert.hpp"
#include <iostream>
#include <set>


int main(int argc, char ** argv)
{
	flatDb::Verify<uint64_t,uint64_t> c;
	std::set<uint64_t> loadSet = {};
	c.verify(
		"/data/WORKSPACE/antonk/deployment-scripts/dbAlleles-64bit-output/idClinVarAllele",
		"/data/WORKSPACE/antonk/deployment-scripts/dbAlleles-64bit-output-2/idClinVarAllele",
		"/data/WORKSPACE/antonk/deployment-scripts/dbAlleles-64bit-output/idClinVarAllele.index",
		"/data/WORKSPACE/antonk/deployment-scripts/dbAlleles-64bit-output-2/idClinVarAllele.index",
		//"/data/WORKSPACE/antonk/deployment-scripts/dbAlleles-64bit-output/sequence",
		//"/data/WORKSPACE/antonk/deployment-scripts/dbAlleles-64bit-output-2/sequence",
		//"/data/WORKSPACE/antonk/deployment-scripts/dbAlleles-64bit-output/sequence.index",
		//"/data/WORKSPACE/antonk/deployment-scripts/dbAlleles-64bit-output-2/sequence.index",
		//"/data/WORKSPACE/antonk/deployment-scripts/dbAlleles-64bit-output/idPa",
		//"/data/WORKSPACE/antonk/deployment-scripts/dbAlleles-64bit-output-2/idPa",
		//"/data/WORKSPACE/antonk/deployment-scripts/dbAlleles-64bit-output/idPa.index",
		//"/data/WORKSPACE/antonk/deployment-scripts/dbAlleles-64bit-output-2/idPa.index",
		//"/data/WORKSPACE/antonk/data/dbAlleles-convert-genomic-dataNode-64bit-target-backup/genomic",
		//"/data/WORKSPACE/antonk/data/dbAlleles-convert-genomic-dataNode-64bit-target-backup-2/genomic",
		//"/data/WORKSPACE/antonk/data/dbAlleles-convert-genomic-dataNode-64bit-target-backup/genomic.index",
		//"/data/WORKSPACE/antonk/data/dbAlleles-convert-genomic-dataNode-64bit-target-backup-2/genomic.index",
		loadSet,
		true, // full diagnostic
		false // repair
	);
	return 0;
}
