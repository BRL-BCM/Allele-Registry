#include "PagesManager.hpp"
#include "../commonTools/Stopwatch.hpp"
#include <iostream>

template<unsigned pageSize>
void testPM()
{
	unsigned const numberOfPages = 1024*1024 / pageSize;
	PagesManager<pageSize> * pagesManager = new PagesManager<pageSize>("testFile",false);
	std::vector<BufferT<pageSize>> pages;
	for (unsigned i = 0; i < numberOfPages; ++i) {
		BufferT<pageSize> bb = pagesManager->allocateBuffer(1);
		uint8_t* b = bb.data;
		b[0] = i % 256;
		b[1] = (i >> 8) % 256;
		b[2] = (i >> 16) % 256;
		b[3] = (i >> 24) % 256;
		pages.push_back(bb);
	}
	Stopwatch stopwatch;
	pagesManager->synchBuffers(pages);
	pagesManager->synchronize();
	std::cout << "Time for pageSize=" << pageSize << ": " << stopwatch.get_time_ms() << " miliseconds" << std::endl;
	delete pagesManager;
}

int main(int argc, char ** argv)
{
//	testPM<1024*1024>();
//	testPM<512*1024>();
//	testPM<256*1024>();
//	testPM<128*1024>();
//	testPM<64*1024>();
//	testPM<32768>();
//	testPM<16384>();
//	testPM< 8192>();
//	testPM< 4096>();

	std::cout << "Create file..." << std::endl;
	PagesManager<8192> * pagesManager = new PagesManager<8192>("testFile",false,128);
	PagesManager<8192>::Buffer b = pagesManager->allocateBuffer(256*1024); // 4GB
	pagesManager->synchBuffer(b);
	pagesManager->releaseBuffer(b);
	std::cout << "Run tests..." << std::endl;
	while (true) {
		for (unsigned i = 0; i < 255*1024; ++i) {
			b = pagesManager->loadBuffer(i*128+13,128);
			pagesManager->releaseBuffer(b);
		}
	}
	std::cout << "Done" << std::endl;
	return 0;
}
