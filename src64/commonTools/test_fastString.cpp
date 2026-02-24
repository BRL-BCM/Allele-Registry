#include "fastString.hpp"
#include <iostream>

int main()
{
	std::string s = "Ala ma kota";
	fastString fs = s;

	std::cout << std::string(fs) << std::endl;

	return 0;
}
