#include "IndexIdentifierCa.hpp"
#include "../apiDb/TasksManager.hpp"
#include "../flatDb/Convert.hpp"
#include <iostream>



int main(int argc, char ** argv)
{
	//flatDb::Convert<uint64_t,uint64_t> c;
	//c.convertIndexNode(
	//	"/data/WORKSPACE/antonk/data/dbAlleles-32bit/protein",
	//	"/data/WORKSPACE/antonk/data/dbAlleles-64bit/protein",
	//	"/data/WORKSPACE/antonk/data/dbAlleles-32bit/protein.index",
	//	"/data/WORKSPACE/antonk/data/dbAlleles-64bit/protein.index"
	//);
	flatDb::Convert<uint32_t,uint64_t> c2;
	c2.convertIndexNode(
		"/data/WORKSPACE/antonk/data/dbAlleles-32bit/idPa",
		"/data/WORKSPACE/antonk/data/dbAlleles-64bit/idPa",
		"/data/WORKSPACE/antonk/data/dbAlleles-32bit/idPa.index",
		"/data/WORKSPACE/antonk/data/dbAlleles-64bit/idPa.index"
	);
	return 0;
}
