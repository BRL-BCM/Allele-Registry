#ifndef REQUESTS_PARSERS_HPP_
#define REQUESTS_PARSERS_HPP_

#include <string>
#include <vector>
#include <memory>
#include <ostream>
#include <cstdint>
#include "../core/variants.hpp"

class Parser {
public:
	// false - no data were returned, EOF reached
	virtual bool parseRecords(std::vector<std::vector<std::string>> & out, std::vector<unsigned> & outLinesOffsets, unsigned maxRecordsCount) = 0;
	// return number of parsed bytes
	virtual uint64_t numberOfParsedBytes() const = 0;
	// return single line by offset
	virtual std::string lineByOffset(unsigned lineOffset) const = 0;
	// destructor
	virtual ~Parser() = default;
};

class ParserVcf : public Parser {
private:
	struct Pim;
	Pim * pim;
public:
	ParserVcf(std::shared_ptr<std::vector<char>> body);
	bool parseRecords(std::vector<std::vector<std::string>> & out, std::vector<unsigned> & outLinesOffsets, unsigned maxRecordsCount) override final;
	uint64_t numberOfParsedBytes() const override final;
	std::string lineByOffset(unsigned lineOffset) const override final;
	~ParserVcf();
};

class ParserTabSeparated : public Parser {
private:
	struct Pim;
	Pim * pim;
public:
	ParserTabSeparated(std::shared_ptr<std::vector<char>> body);
	bool parseRecords(std::vector<std::vector<std::string>> & out, std::vector<unsigned> & outLinesOffsets, unsigned maxRecordsCount) override final;
	uint64_t numberOfParsedBytes() const override final;
	std::string lineByOffset(unsigned lineOffset) const override final;
	~ParserTabSeparated();
};

class ParserVcf2 {
private:
	struct Pim;
	Pim * pim;
public:
	ParserVcf2(std::shared_ptr<std::vector<char>> body, std::vector<ReferenceId> const & refIdByChromosome);
	bool parseRecords(std::vector<PlainVariant> & out, unsigned maxRecordsCount);
	uint64_t numberOfParsedBytes() const;
	void buildVcfResponse(std::ostream &, std::vector<std::vector<std::string>> const &);
	~ParserVcf2();
};

#endif /* REQUESTS_PARSERS_HPP_ */
