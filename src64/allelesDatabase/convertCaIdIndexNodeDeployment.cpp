#include "IndexIdentifierCa32.hpp"
#include "IndexIdentifierCa.hpp"
#include "../apiDb/TasksManager.hpp"
#include "../flatDb/Convert.hpp"
#include <iostream>
#include <csignal>

void sighandler(int s)
{
	std::cout << "Caught signal " << s << ".\n"; // this is undefined behaviour
	std::cout << "try 'touch stop'" << std::endl;
}

int main(int argc, char ** argv)
{
	if(argc != 3) {
		std::cout << "try ./convertCaIdIndexNodeDeployment <in-dir> <out-dir>" << std::endl;
		exit(1);
	}

	std::string inDir = argv[1];
	std::string outDir = argv[2];

	std::signal(SIGINT, sighandler);

	flatDb::Convert<uint32_t,uint32_t> c;
	c.convertIndexNode(
		inDir+"/"+"idCa",
		outDir+"/"+"idCa",
		inDir+"/"+"idCa.index",
		outDir+"/"+"idCa.index"
	);
	return 0;
}
