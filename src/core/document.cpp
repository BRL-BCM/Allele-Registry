#include "document.hpp"
#include <boost/lexical_cast.hpp>


DocumentError DocumentError::createFromCurrentException()
{
	DocumentError d;
	try {
		std::rethrow_exception( std::current_exception() );
		d.type = errorType::InternalServerError;
		d.message = "Unknown error occurred";
		d.fields[label::debug] = "Empty exception container!";
	} catch (ExceptionBase const & e) {
		d.type = e.type();
		d.fields = e.fields();
		d.message = e.what();
	} catch (std::exception const & e) {
		d.type = errorType::InternalServerError;
		d.message = e.what();
	} catch (...) {
		d.type = errorType::InternalServerError;
		d.message = "Unknown error occurred";
	}
	return d;
}


std::string DocumentError::toString() const
{
	return "[" + boost::lexical_cast<std::string>(type) + "," + message + "]";
}

std::string DocumentActiveGenomicVariant::toString() const
{
	return "[" + mainDefinition.toString() + ",CA" + boost::lexical_cast<std::string>(caId.value) + ","+ identifiers.toString() + "]";
}

std::string DocumentActiveProteinVariant::toString() const
{
	return "[" + mainDefinition.toString() + ",PA" + boost::lexical_cast<std::string>(caId.value) + ","+ identifiers.toString() + "]";
}

std::string DocumentInactiveVariant::toString() const
{
	std::string s = "[";
	bool first = true;
	for (auto const & nc: newCaIds) {
		if (first) {
			first = false;
		} else {
			s += ",";
		}
		s += "(" + boost::lexical_cast<std::string>(nc.first.value) + "," + boost::lexical_cast<std::string>(nc.second) + ")";
	}
	return (s + "]");
}

std::string DocumentReference::toString() const
{
	return "[RS" + boost::lexical_cast<std::string>(rsId) + "]";
}

std::string DocumentGene::toString() const
{
	return "[GN" + boost::lexical_cast<std::string>(gnId) + "]";
}

Document const Document::null;

std::string Document::toString() const
{
	switch (fType) {
		case documentType::null           : return "{}";
		case documentType::error          : return "{error:"           + error          ().toString() + "}";
		case documentType::activeGenomicVariant  : return "{activeGenomicVariant:"   + asActiveGenomicVariant().toString() + "}";
		case documentType::activeProteinVariant  : return "{activeProteinVariant:"   + asActiveProteinVariant().toString() + "}";
		case documentType::inactiveVariant: return "{inactiveVariant:" + inactiveVariant().toString() + "}";
		case documentType::reference      : return "{reference:"       + reference      ().toString() + "}";
		case documentType::gene           : return "{gene:"            + gene           ().toString() + "}";
	}
	throw std::logic_error("Unknown document type");
}
