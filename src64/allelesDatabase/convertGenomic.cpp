#include "IndexIdentifierCa.hpp"
#include "../apiDb/TasksManager.hpp"
#include "../flatDb/Convert.hpp"
#include <iostream>



int main(int argc, char ** argv)
{
	flatDb::Convert<uint32_t,uint64_t> c;
	c.convertIndexNode(
		"/data/WORKSPACE/antonk/data/dbAlleles-32bit/genomic",
		"/data/WORKSPACE/antonk/data/dbAlleles-64bit/genomic",
		"/data/WORKSPACE/antonk/data/dbAlleles-32bit/genomic.index",
		"/data/WORKSPACE/antonk/data/dbAlleles-64bit/genomic.index"
	);
	return 0;
}
