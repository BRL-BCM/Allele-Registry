#ifndef ASSERT_HPP_
#define ASSERT_HPP_

#include <stdexcept>
#include <string>
#include <boost/lexical_cast.hpp>

class ExceptionAssertionFailed : public std::logic_error
{
private:
	std::string fAssertion = "";
	std::string fFile = "";
	unsigned    fLine = 0;
	std::string fFunction = "";
public:
	ExceptionAssertionFailed(std::string const & msg)
	: std::logic_error("Assertion failed: " + msg), fAssertion(msg)
	{}
	ExceptionAssertionFailed(std::string const & msg, std::string const & file, unsigned line, std::string const & function)
	: std::logic_error("Assertion failed: " + msg + "  ( " + file + " , " + boost::lexical_cast<std::string>(line) + " , " + function + " )")
	, fAssertion(msg), fFile(file), fLine(line), fFunction(function)
	{}
};

#define ASSERT(expr) if (!(expr)) throw ExceptionAssertionFailed(#expr, __FILE__, __LINE__, __func__);



#endif /* ASSERT_HPP_ */
