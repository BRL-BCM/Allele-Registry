#ifndef COMMONTOOLS_FASTSTRING_HPP_
#define COMMONTOOLS_FASTSTRING_HPP_

#include <cstdint>
#include <vector>
#include <string>
#include <algorithm>
#include "assert.hpp"

#include "../debug.hpp"

template<typename tType>
struct buffer_ptr
{
	uint32_t * fBuffer;  // first two variables (objects counter + size), then data
	buffer_ptr()
	{
		this->fBuffer = new uint32_t[2];
		this->fBuffer[0] = 1; // number of references
		this->fBuffer[1] = 0; // size of the buffer - TODO - is it needed?
	}
	buffer_ptr(tType const * data, uint32_t size)
	{
		this->fBuffer = reinterpret_cast<uint32_t*>(new uint8_t[ sizeof(tType)*size + sizeof(uint32_t)*2 ]);
		this->fBuffer[0] = 1;                  // number of references
		this->fBuffer[1] = sizeof(tType)*size; // size of the buffer - TODO - is it needed?
		tType * ptr =  reinterpret_cast<tType*>(this->fBuffer+2);
		for ( ;  size > 0;  --size ) {
			*ptr = *data;
			++ptr;
			++data;
		}
	}
	buffer_ptr(buffer_ptr const & src) : fBuffer(src.fBuffer)
	{
		++(this->fBuffer[0]);
	}
	~buffer_ptr()
	{
		if ( --(this->fBuffer[0]) == 0 ) delete [] reinterpret_cast<uint8_t*>(this->fBuffer);
	}
	buffer_ptr const & operator=(buffer_ptr const & src)
	{
		++(src.fBuffer[0]);
		if ( --(this->fBuffer[0]) == 0 ) delete [] reinterpret_cast<uint8_t*>(this->fBuffer);
		this->fBuffer = src.fBuffer;
		return *this;
	}
	uint32_t size() const
	{
		return (this->fBuffer[1] / sizeof(tType));
	}
	tType* buffer() const { return reinterpret_cast<tType*>(this->fBuffer + 2); }
	bool operator<(buffer_ptr const & b) const { return (this->fBuffer < b.fBuffer); }
};


class fastString
{
private:
	struct tSubString
	{
		uint32_t endPosition = 0;
		uint32_t internalOffset = 0;
		buffer_ptr<char> data;
		tSubString() {};
		explicit tSubString(buffer_ptr<char> const & buf) : endPosition(buf.size()), data(buf) {}
	};
	std::vector<tSubString> fData;
public:
	fastString() {}
	fastString(std::string const & s)
	{
		buffer_ptr<char> buf(s.data(), s.size());
		fData.push_back(tSubString(buf));
	}
	operator std::string() const
	{
		if (fData.empty()) return "";
		std::string s(fData.back().endPosition, 'X');
		std::string::iterator iS = s.begin();
		uint32_t offset = 0;
		for (auto const & e: fData) {
			uint32_t length = e.endPosition - offset;
			char const * ptr = e.data.buffer() + e.internalOffset;
			for ( ;  length > 0;  --length ) {
				*iS = *ptr;
				++iS;
				++ptr;
			}
			offset = e.endPosition;
		}
		return s;
	}
	uint32_t size() const
	{
		if (fData.empty()) return 0;
		return (fData.back().endPosition);
	}
	fastString & operator+=(fastString const & fs)
	{
		if (fData.empty()) {
			fData = fs.fData;
			return *this;
		}
		unsigned const size1 = fData.size();
		unsigned const size2 = fs.fData.size();
		uint32_t const length1 = fData.back().endPosition;
		fData.reserve(size1 + size2);
		for (uint32_t i = 0; i < size2; ++i) {
			fData.push_back(fs.fData[i]);
			fData.back().endPosition += length1;
		}
		return *this;
	}
	fastString operator+(fastString const & fs) const
	{
		fastString r;
		r.fData.reserve( fData.size() + fs.fData.size() );
		r.fData.insert(r.fData.end(), fData.begin(), fData.end());
		uint32_t const length1 = fData.back().endPosition;
		for (auto const & x: fs.fData) {
			r.fData.push_back(x);
			r.fData.back().endPosition += length1;
		}
		return r;
	}
	char operator[](uint32_t position) const
	{
		if (fData.empty() || position >= fData.back().endPosition) {
			throw std::out_of_range("fastString::operator[](...) const");
		}
		auto it = std::upper_bound( fData.begin(), fData.end(), position, [](uint32_t v, tSubString const & e)->bool{ return (v < e.endPosition); } );
		// read offset of the element
		uint32_t offsetOfTheElement = 0;
		if (it != fData.begin()) {
			--it;
			offsetOfTheElement = it->endPosition;
			++it;
		}
		// return character
		return (it->data.buffer()[position - offsetOfTheElement + it->internalOffset]);
	}
	fastString substr(uint32_t position, uint32_t length = std::numeric_limits<uint32_t>::max()) const
	{
		fastString r;
		if (fData.empty()) {
			if (position == 0) return r;
			throw std::out_of_range("fastString::substr(...)");
		}
		if (position > fData.back().endPosition) {
			throw std::out_of_range("fastString::substr(...)");
		}
		length = std::min( length, fData.back().endPosition - position );
		if (length == 0) return r;
		// left border
		auto itLeft = std::upper_bound( fData.begin(), fData.end(), position, [](uint32_t v, tSubString const & e)->bool{ return (v < e.endPosition); } );
		ASSERT(itLeft != fData.end());
		// right border
		auto itRight = std::lower_bound( itLeft, fData.end(), position+length, [](tSubString const & e, uint32_t v)->bool{ return (e.endPosition < v); } );
		ASSERT(itRight != fData.end());
		++itRight;
		// read offset of the first element to copy
		uint32_t offsetOfTheFirstElement = 0;
		if (itLeft != fData.begin()) {
			--itLeft;
			offsetOfTheFirstElement = itLeft->endPosition;
			++itLeft;
		}
		// copy entries
		r.fData.reserve(itRight-itLeft);
		for ( ;  itLeft != itRight;  ++itLeft ) {
			r.fData.push_back(*itLeft);
			r.fData.back().endPosition -= position;
		}
		// correct the first element
		r.fData.front().internalOffset += (position - offsetOfTheFirstElement);
		// correct the last element
		r.fData.back().endPosition = length;
		// return result
		return r;
	}
};


#endif /* COMMONTOOLS_FASTSTRING_HPP_ */
