#ifndef SHMEMCONTAINERS_HPP_
#define SHMEMCONTAINERS_HPP_

#include <string>
#include <vector>
#include <cstring>

#include "stringPointer.hpp"

class SharedString {
private:
	std::string::size_type fSize;
	char * fPtr;
public:
	SharedString(void *& ptr);
	SharedString(void *& ptr, std::string const & data);
	inline std::string::size_type size() const { return fSize; }
	inline StringPointer substr(std::string::size_type offset, std::string::size_type length = std::string::npos) const
	{
		if (offset > fSize) throw std::out_of_range("SharedString");
		if (fSize - offset < length) length = fSize - offset;
		return StringPointer( fPtr+offset, fPtr+offset+length );
	}
	inline std::string toString() const { return std::string( fPtr, fPtr+fSize ); }
};

SharedString::SharedString(void *& ptr)
{
	std::string::size_type * p1 = reinterpret_cast<std::string::size_type*>(ptr);
	fSize = *p1;
	fPtr = reinterpret_cast<char*>(++p1);
	ptr = reinterpret_cast<void*>(fPtr+fSize);
}

SharedString::SharedString(void *& ptr, std::string const & data)
{
	std::string::size_type * p1 = reinterpret_cast<std::string::size_type*>(ptr);
	fSize = *p1 = data.size();
	fPtr = reinterpret_cast<char*>(++p1);
	strncpy(fPtr, data.data(), fSize);
	ptr = reinterpret_cast<void*>(fPtr+fSize);
}


class SharedVectorOfStrings {
private:
	std::vector<SharedString> fData;
public:
	SharedVectorOfStrings() {};
	SharedVectorOfStrings(void *& ptr);
	SharedVectorOfStrings(void *& ptr, std::vector<std::string> const & data);
	inline std::vector<SharedString>::size_type size() const { return fData.size(); }
	inline SharedString & operator[](std::vector<SharedString>::size_type offset) { return fData[offset]; }
	inline SharedString const & operator[](std::vector<SharedString>::size_type offset) const { return fData[offset]; }
};

SharedVectorOfStrings::SharedVectorOfStrings(void *& ptr)
{
	std::vector<SharedString>::size_type * p1 = reinterpret_cast<std::vector<SharedString>::size_type*>(ptr);
	std::vector<SharedString>::size_type lSize = *p1;
	ptr = reinterpret_cast<void*>(++p1);
	for ( ;  lSize;  lSize-- ) {
		fData.push_back( SharedString( ptr ) );
	}
}

SharedVectorOfStrings::SharedVectorOfStrings(void *& ptr, std::vector<std::string> const & data)
{
	std::vector<SharedString>::size_type * p1 = reinterpret_cast<std::vector<SharedString>::size_type*>(ptr);
	std::vector<SharedString>::size_type lSize = *p1 = data.size();
	ptr = reinterpret_cast<void*>(++p1);
	for ( ;  lSize;  lSize-- ) {
		fData.push_back( SharedString( ptr, data[fData.size()] ) );
	}
}

#endif /* SHMEMCONTAINERS_HPP_ */
