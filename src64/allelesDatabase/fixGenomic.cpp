#include "IndexIdentifierCa.hpp"
#include "../apiDb/TasksManager.hpp"
#include "../flatDb/Convert.hpp"
#include <iostream>



int main(int argc, char ** argv)
{
	flatDb::Convert<uint64_t,uint64_t> c;
	c.convertIndexNode(
		"/data/WORKSPACE/antonk/data/dbAlleles-convert-genomic-dataNode-64bit-target-backup/genomic",
		"/data/WORKSPACE/antonk/data/dbAlleles-convert-genomic-dataNode-64bit-target-backup-2/genomic",
		"/data/WORKSPACE/antonk/data/dbAlleles-convert-genomic-dataNode-64bit-target-backup/genomic.index",
		"/data/WORKSPACE/antonk/data/dbAlleles-convert-genomic-dataNode-64bit-target-backup-2/genomic.index"
	);
	return 0;
}
