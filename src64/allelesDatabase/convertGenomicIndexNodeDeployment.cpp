#include "IndexIdentifierCa.hpp"
#include "../apiDb/TasksManager.hpp"
#include "../flatDb/Convert.hpp"
#include <iostream>



int main(int argc, char ** argv)
{
	if(argc != 3) {
		std::cout << "try ./convertGenomicIndexNodeDeployment <in-dir> <out-dir>" << std::endl;
		exit(1);
	}
	std::string inDir = argv[1];
	std::string outDir = argv[2];
	flatDb::Convert<uint32_t,uint64_t> c;
	c.convertIndexNode(
		inDir+"/"+"genomic",
		outDir+"/"+"genomic",
		inDir+"/"+"genomic.index",
		outDir+"/"+"genomic.index"
	);
	return 0;
}
