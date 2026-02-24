#include "IndexIdentifierCa.hpp"
#include "../apiDb/TasksManager.hpp"
#include "../flatDb/Convert.hpp"
#include <iostream>



int main(int argc, char ** argv)
{
	if(argc != 3) {
		std::cout << "try ./convertProteinIndexNodeDeployment <in-dir> <out-dir>" << std::endl;
		exit(1);
	}
	std::string inDir = argv[1];
	std::string outDir = argv[2];
	flatDb::Convert<uint64_t,uint64_t> c2;
	c2.convertIndexNode(
		inDir+"/"+"protein",
		outDir+"/"+"protein",
		inDir+"/"+"protein.index",
		outDir+"/"+"protein.index"
	);
	return 0;
}
