#ifndef HEADER_HPP
#define HEADER_HPP

struct Header {
	unsigned keySize;
	unsigned pageSize;
	unsigned indexNodePages;
	unsigned revision;
};

#endif
