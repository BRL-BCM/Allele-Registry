#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/interprocess/sync/named_semaphore.hpp>
#include <iostream>


void printStatus(std::string const & prefix, bool result)
{
	std::cout << prefix << ": " << (result ? ("Success") : ("Failure")) << std::endl;
}

int main(int argc, char ** argv)
{
	try {
		printStatus("Try to remove mutex: ", boost::interprocess::named_mutex::remove("genomeIndex_mutex"));
		printStatus("Try to remove semaphore: ", boost::interprocess::named_semaphore::remove("genomeIndex_semaphore"));
		printStatus("Try to remove shared memory: ", boost::interprocess::shared_memory_object::remove("genomeIndex_sharedMemory"));
//		std::cout << "Try to open mutex: " << std::endl;
//		{
//			boost::interprocess::named_mutex m(boost::interprocess::open_only, "genomeIndex_mutex");
//		}
//		std::cout << "Try to free mutex: " << std::endl;
//		if ( boost::interprocess::named_mutex::remove("genomeIndex_mutex") ) {
//			std::cout << "Success!" << std::endl;
//		} else {
//			std::cout << "Failure!" << std::endl;
//		}
//		std::cout << "Done" << std::endl;
	} catch (std::exception const & e) {
		std::cerr << "EXCEPTION: " << e.what() << std::endl;
		return -1;
	}
	return 0;
}
