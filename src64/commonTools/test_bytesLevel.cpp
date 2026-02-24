#include "bytesLevel.hpp"
#include <limits>
#include <vector>
#include <stdexcept>
#include <iostream>

template <unsigned firstChunkInBytes, unsigned nextChunkInBytes, uint8_t pattern = 0xaa, unsigned margin = 4, typename tUnsigned>
void test_internal(tUnsigned value)
{
	unsigned const length = lengthUnsignedIntVarSize<firstChunkInBytes,nextChunkInBytes,tUnsigned>(value);
	std::vector<uint8_t> buf( 2*margin + length, pattern );
	uint8_t * ptr = &(buf[margin]);
	writeUnsignedIntVarSize<firstChunkInBytes,nextChunkInBytes>(ptr,value);
	if (ptr != &(buf[margin+length])) throw std::logic_error("Incorrect pointer position (1)");
	uint8_t const * ptr2 = &(buf[margin]);
	tUnsigned value2 = readUnsignedIntVarSize<firstChunkInBytes,nextChunkInBytes,tUnsigned>(ptr2);
	if (ptr2 != &(buf[margin+length])) throw std::logic_error("Incorrect pointer position (2)");
	if (value != value2) throw std::logic_error("Written and read values do not match");
	for (unsigned i = 0; i < margin; ++i) {
		if (buf[i] != pattern) throw std::logic_error("Buffer before the written area is broken");
		if (buf[buf.size()-1-i] != pattern) throw std::logic_error("Buffer after the written area is broken");
	}
}

template <typename tUnsigned>
void test_value(tUnsigned v)
{
	test_internal<1,1>(v);
	test_internal<2,1>(v);
	test_internal<3,1>(v);
	test_internal<1,2>(v);
	test_internal<2,2>(v);
	test_internal<3,2>(v);
	test_internal<1,3>(v);
	test_internal<2,3>(v);
	test_internal<3,3>(v);
}

template <typename tUnsigned, tUnsigned step = 11>
void test_step()
{
	tUnsigned v = 0;
	for (; std::numeric_limits<tUnsigned>::max() - step >= v; v += step) {
		test_value(v);
	}
}


int main()
{
	std::cout << "Testing uin8_t ..." << std::endl;
	test_step<uint8_t,1>();
	std::cout << "Testing uin16_t ..." << std::endl;
	test_step<uint16_t,157>();
	std::cout << "Testing uin32_t ..." << std::endl;
	test_step<uint32_t,1234567>();
	std::cout << "Testing uin64_t ..." << std::endl;
	test_step<uint64_t,345678901201234567ull>();
	std::cout << "Test for small numbers ..." << std::endl;
	for (unsigned i = 0; i < 100123; ++i) {
		test_value<uint16_t>(i);
		test_value<uint32_t>(i);
		test_value<uint64_t>(i);
	}
	return 0;
}
