#ifndef STRINGPOINTER_HPP_
#define STRINGPOINTER_HPP_

#include <string>

class StringPointer
{
private:
	char const * fBegin;
	char const * fEnd;
public:
	typedef std::string::size_type size_type;
	StringPointer(char const * buffer, size_type length) : fBegin(buffer), fEnd(buffer+length) {}
	StringPointer(char const * begin, char const * end) : fBegin(begin), fEnd(end) {}
	operator std::string() const { return std::string(fBegin,fEnd); }
	size_type size() const { return (fEnd-fBegin); }
	StringPointer substr(size_type pos, size_type length) const
	{
		char const * p = fBegin + pos;
		if (p > fEnd) throw std::out_of_range("StringPointer");
		char const * p2 = (length > static_cast<size_type>(fEnd - p)) ? (fEnd) : (p + length);
		return StringPointer(p, p2);
	}
	char const * data() const { return fBegin; }
};



#endif /* STRINGPOINTER_HPP_ */
