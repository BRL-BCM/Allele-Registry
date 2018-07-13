#include "parsers.hpp"
#include "InputTools.hpp"
#include "../core/globals.hpp"
#include "../core/exceptions.hpp"
#include <boost/algorithm/string.hpp>
#include <map>
#include <set>

// NCBI36
inline std::string chr2refseq_NCBI36(Chromosome chr)
{
	switch (chr) {
		case Chromosome::chr1: return "NC_000001.9";
		case Chromosome::chr2: return "NC_000002.10";
		case Chromosome::chr3: return "NC_000003.10";
		case Chromosome::chr4: return "NC_000004.10";
		case Chromosome::chr5: return "NC_000005.8";
		case Chromosome::chr6: return "NC_000006.10";
		case Chromosome::chr7: return "NC_000007.12";
		case Chromosome::chr8: return "NC_000008.9";
		case Chromosome::chr9: return "NC_000009.10";
		case Chromosome::chr10: return "NC_000010.9";
		case Chromosome::chr11: return "NC_000011.8";
		case Chromosome::chr12: return "NC_000012.10";
		case Chromosome::chr13: return "NC_000013.9";
		case Chromosome::chr14: return "NC_000014.7";
		case Chromosome::chr15: return "NC_000015.8";
		case Chromosome::chr16: return "NC_000016.8";
		case Chromosome::chr17: return "NC_000017.9";
		case Chromosome::chr18: return "NC_000018.8";
		case Chromosome::chr19: return "NC_000019.8";
		case Chromosome::chr20: return "NC_000020.9";
		case Chromosome::chr21: return "NC_000021.7";
		case Chromosome::chr22: return "NC_000022.9";
		case Chromosome::chrX: return "NC_000023.9";
		case Chromosome::chrY: return "NC_000024.8";
		case Chromosome::chrM: return "NC_001807.4";
	}
	ASSERT(false);
}

// GRCh37
inline std::string chr2refseq_GRCh37(Chromosome chr)
{
	switch (chr) {
		case Chromosome::chr1: return "NC_000001.10";
		case Chromosome::chr2: return "NC_000002.11";
		case Chromosome::chr3: return "NC_000003.11";
		case Chromosome::chr4: return "NC_000004.11";
		case Chromosome::chr5: return "NC_000005.9";
		case Chromosome::chr6: return "NC_000006.11";
		case Chromosome::chr7: return "NC_000007.13";
		case Chromosome::chr8: return "NC_000008.10";
		case Chromosome::chr9: return "NC_000009.11";
		case Chromosome::chr10: return "NC_000010.10";
		case Chromosome::chr11: return "NC_000011.9";
		case Chromosome::chr12: return "NC_000012.11";
		case Chromosome::chr13: return "NC_000013.10";
		case Chromosome::chr14: return "NC_000014.8";
		case Chromosome::chr15: return "NC_000015.9";
		case Chromosome::chr16: return "NC_000016.9";
		case Chromosome::chr17: return "NC_000017.10";
		case Chromosome::chr18: return "NC_000018.9";
		case Chromosome::chr19: return "NC_000019.9";
		case Chromosome::chr20: return "NC_000020.10";
		case Chromosome::chr21: return "NC_000021.8";
		case Chromosome::chr22: return "NC_000022.10";
		case Chromosome::chrX: return "NC_000023.10";
		case Chromosome::chrY: return "NC_000024.9";
		case Chromosome::chrM: return "NC_012920.1";
	}
	ASSERT(false);
}

// GRCh38
inline std::string chr2refseq_GRCh38(Chromosome chr)
{
	switch (chr) {
		case Chromosome::chr1: return "NC_000001.11";
		case Chromosome::chr2: return "NC_000002.12";
		case Chromosome::chr3: return "NC_000003.12";
		case Chromosome::chr4: return "NC_000004.12";
		case Chromosome::chr5: return "NC_000005.10";
		case Chromosome::chr6: return "NC_000006.12";
		case Chromosome::chr7: return "NC_000007.14";
		case Chromosome::chr8: return "NC_000008.11";
		case Chromosome::chr9: return "NC_000009.12";
		case Chromosome::chr10: return "NC_000010.11";
		case Chromosome::chr11: return "NC_000011.10";
		case Chromosome::chr12: return "NC_000012.12";
		case Chromosome::chr13: return "NC_000013.11";
		case Chromosome::chr14: return "NC_000014.9";
		case Chromosome::chr15: return "NC_000015.10";
		case Chromosome::chr16: return "NC_000016.10";
		case Chromosome::chr17: return "NC_000017.11";
		case Chromosome::chr18: return "NC_000018.10";
		case Chromosome::chr19: return "NC_000019.10";
		case Chromosome::chr20: return "NC_000020.11";
		case Chromosome::chr21: return "NC_000021.9";
		case Chromosome::chr22: return "NC_000022.11";
		case Chromosome::chrX: return "NC_000023.11";
		case Chromosome::chrY: return "NC_000024.10";
		case Chromosome::chrM: return "NC_012920.1";
	}
	ASSERT(false);
}

// hg19
inline std::string chr2refseq_hg19(Chromosome chr)
{
	if (chr == Chromosome::chrM) return "NC_001807.4";
	return chr2refseq_GRCh37(chr);
}


inline std::string chr2refseq(std::string const & assembly, Chromosome chr, unsigned lineNumber)
{
	if (assembly == "NCBI36" || assembly == "hg18") return chr2refseq_NCBI36(chr);
	else if (assembly == "GRCh37") return chr2refseq_GRCh37(chr);
	else if (assembly == "GRCh38") return chr2refseq_GRCh38(chr);
	else if (assembly == "hg19"  ) return chr2refseq_hg19  (chr);
	else throw ExceptionVcfParsingError(lineNumber, "The only supported assemblies are 'NCBI36', 'GRCh37', 'GRCh38', 'hg18', 'hg19' (got '" + assembly + "')");
}


// parse: list key1=value1,key2=value2,key3=value3,...
inline std::map<std::string,std::string> parseListOfFields(std::string const & s)
{
	std::vector<std::string> vs;
	boost::split(vs, s, boost::is_any_of(","));
	std::map<std::string,std::string> results;
	for (auto const & s: vs) {
		std::string::size_type tp = s.find_first_of('=');
		std::string key, value = "";
		if (tp == std::string::npos) {
			key = s;
		} else {
			key = s.substr(0,tp);
			if (tp+1 < s.size()) value = s.substr(tp+1);
		}
		if (results.count(key)) throw std::runtime_error("duplicated key '" + key + "'");
		results[key] = value;
	}
	return results;
}

// ===================================== ParserVcf ================================

struct ParserVcf::Pim
{
	std::shared_ptr<std::vector<char>> fBody;
	std::vector<char>::const_iterator fPosition;
	unsigned fLineNumber = 0;
	std::map<std::string,std::string> refSequences;
};


ParserVcf::ParserVcf(std::shared_ptr<std::vector<char>> body) : pim(new Pim)
{
	pim->fBody = body;
	pim->fPosition = body->begin();
}


bool ParserVcf::parseRecords(std::vector<std::vector<std::string>> & out, std::vector<unsigned> & outLines, unsigned maxRecordsCount)
{
	out.clear();
	out.reserve(maxRecordsCount);
	outLines.clear();
	outLines.reserve(maxRecordsCount);

	std::vector<char>::const_iterator & iBody = pim->fPosition;
	std::vector<char>::const_iterator const iEnd = pim->fBody->end();

	// ========================================== header =============================================
	if (iBody == pim->fBody->begin()) {  // first call - parse the header
		std::string assembly = "";
		while ( iBody < iEnd ) {
			++(pim->fLineNumber);
			// ---- parse a line
			auto iLine = iBody;
			while ( iBody < iEnd && *iBody != '\n' && *iBody != '\r' ) {
				++iBody;
			}
			while (iLine < iBody && (*iLine == ' ' || *iLine == '\t')) ++iLine; // ommit white spaces
			if (iLine == iBody) continue; // ignore empty lines (for now)
			std::string line(iLine,iBody);
			// ---- eat EOL character(s) - LF, CR or CR+LF
			if ( iBody < iEnd && *iBody == '\r' ) ++iBody;
			if ( iBody < iEnd && *iBody == '\n' ) ++iBody;
			// ---- parse header line
			if ( line.front() != '#') {
				throw ExceptionVcfParsingError(pim->fLineNumber, "Unexpected data line (header line was expected)");
			}
			std::string const headerLine = "#CHROM\tPOS\tID\tREF\tALT\tQUAL\tFILTER\tINFO";
			if ( line.substr(0,headerLine.size()) == headerLine ) break; // end of header
			if ( line.substr(0,11) == "##assembly=") {
				assembly = line.substr(11);
				chr2refseq(assembly, Chromosome::chr1, pim->fLineNumber); // check assembly format
				continue; // go to next line
			}
			if ( line.substr(0,10) != "##contig=<" ) continue; // go to next line
			// header line with contig field
			std::string::size_type tempPos = line.find_first_of('>',10);
			if (tempPos == std::string::npos) {
				throw ExceptionVcfParsingError(pim->fLineNumber, "Incorrect format of ##contig header line - missing '>' character");
			}
			std::map<std::string,std::string> fields;
			try {
				fields = parseListOfFields(line.substr( 10, tempPos - 10 ));
			} catch (std::exception const & e) {
				throw ExceptionVcfParsingError(pim->fLineNumber, "Incorrect format of ##contig header line - " + std::string(e.what()));
			}
			if ( fields["ID"] == "" ) {
				throw ExceptionVcfParsingError(pim->fLineNumber, "Every ##contig header line must contain field 'ID'");
			}
			if ( fields["assembly"] == "" && assembly == "" ) {
				throw ExceptionVcfParsingError(pim->fLineNumber, "Every ##contig header line must contain fields 'assembly', if it is not set globally");
			}
			if ( fields["assembly"] != "" && assembly != "" && fields["assembly"] != assembly ) {
				throw ExceptionVcfParsingError(pim->fLineNumber, "Value of 'assembly' in ##contig header line does not match global 'assembly' field");
			}
			if ( fields["assembly"] == "" ) fields["assembly"] = assembly;
			std::string id = fields["ID"];
			if (id.substr(0,3) == "chr") id = id.substr(3);
			unsigned chr = 0;
			if (id == "X") chr = Chromosome::chrX;
			else if (id == "Y") chr = Chromosome::chrY;
			else if (id == "M" || id == "MT") chr = Chromosome::chrM;
			else {
				for (unsigned i = 0; i < id.size(); ++i) {
					if (id[i] < '0' || id[i] > '9' || chr > 2) {
						chr = 0;
						break;
					}
					chr *= 10;
					chr += (id[i] - '0');
				}
				if ( chr == 0 || chr > 22 ) {
					throw ExceptionVcfParsingError(pim->fLineNumber, "In ##contig header line, field ID must contain one of the following: '1'-'22', 'X', 'Y', 'M', 'MT' with optional 'chr' prefix");
				}
			}
			id = fields["ID"];
			if (pim->refSequences.count(id)) {
				throw ExceptionVcfParsingError(pim->fLineNumber, "##contig header line with duplicated ID field: " + id);
			}
			pim->refSequences[id] = chr2refseq(fields["assembly"], static_cast<Chromosome>(chr), pim->fLineNumber);
		}
		// set default value for unset config
		if (assembly != "") {
			for (unsigned i = static_cast<unsigned>(Chromosome::chr1); i <= static_cast<unsigned>(Chromosome::chrM); ++i) {
				std::string id = "";
				if (i <= 22) id = boost::lexical_cast<std::string>(i);
				else if (i == 23) id = "X";
				else if (i == 24) id = "Y";
				else if (i == 25) id = "M";
				if (pim->refSequences.count(id) == 0) pim->refSequences[id] = chr2refseq(assembly, static_cast<Chromosome>(i), pim->fLineNumber);
			}
			if (pim->refSequences.count("MT") == 0) pim->refSequences["MT"] = chr2refseq(assembly, Chromosome::chrM, pim->fLineNumber);
		}
	}

	// ========================================== data =============================================
	std::vector<std::string> colData;
	while ( out.size() < maxRecordsCount && iBody < iEnd ) {
		colData.clear();
		++(pim->fLineNumber);
		// ---- parse a line
		auto iWord = iBody;
		auto iLine = iBody;
		while ( iBody < iEnd && *iBody != '\n' && *iBody != '\r' ) {
			if (*iBody == '\t') {
				colData.push_back( std::string(iWord,iBody) );
				iWord = ++iBody;
			} else {
				++iBody;
			}
		}
		if (iWord < iBody) colData.push_back( std::string(iWord,iBody) ); // last word/column
		std::string const inputLine = std::string(iLine,iBody);
		// ---- eat EOL character(s) - LF, CR or CR+LF
		if ( iBody < iEnd && *iBody == '\r' ) ++iBody;
		if ( iBody < iEnd && *iBody == '\n' ) ++iBody;
		// ---- parse data line
		if ( (! colData.empty()) && (colData.front().front() == '#') ) {
			throw ExceptionVcfParsingError(pim->fLineNumber, "Unexpected header line (data line was expected)");
		}
		if ( colData.size() < 8 ) {
			throw ExceptionVcfParsingError(pim->fLineNumber, "Incorrect format of data line (number of columns is too small)");
		}
		std::string chr = colData[0];
		if (chr.substr(0,3) == "chr") chr = chr.substr(3);
		std::string pos = colData[1];
		std::string ref = colData[3];
		if (ref == ".") ref = "";
		std::string alt = colData[4];
		if (alt == ".") alt = ref;
		if (pim->refSequences.count(chr) == 0) {
			throw ExceptionVcfParsingError(pim->fLineNumber, "Unknown chromosome in data line. Probably ##contig header line is missing");
		}
		try {
			unsigned pos2 = parseUInt32(pos);
			if (pos2 == 0) throw ExceptionVcfParsingError(pim->fLineNumber, "Zero is not allowed as a position.");
			pos = boost::lexical_cast<std::string>(--pos2);
		} catch (std::exception const & e) {
			throw ExceptionVcfParsingError(pim->fLineNumber, "Incorrect position in data line: " + std::string(e.what()));
		}
		if (ref == "*") ref = "";
		if (alt == "*") alt = "";
		if (ref == "") throw ExceptionVcfParsingError(pim->fLineNumber, "Incorrect reference sequence");
		if (alt == "") throw ExceptionVcfParsingError(pim->fLineNumber, "Incorrect alternate sequence");
		// ---- build record of definitions with reference
		std::string::size_type altPos = 0;
		while (true) {
			std::string::size_type const altNext = alt.find_first_of(',',altPos);
			colData.resize(1);
			colData[0] = pim->refSequences[chr];
			colData[0] += "," + pos;
			colData[0] += "," + boost::lexical_cast<std::string>(ref.size());
			colData[0] += "," + alt.substr(altPos,altNext-altPos);
			colData[0] += "," + ref;
			out.push_back(colData);
			outLines.push_back(iLine - pim->fBody->begin());
			if (altNext == std::string::npos) break;
			altPos = altNext + 1;
		}
	}

	return (! out.empty());
}


uint64_t ParserVcf::numberOfParsedBytes() const
{
	return (pim->fPosition - pim->fBody->begin());
}


std::string ParserVcf::lineByOffset(unsigned lineOffset) const
{
	std::vector<char>::const_iterator itBegin = pim->fBody->begin() + lineOffset;
	auto itEnd = itBegin;
	while ( itEnd < pim->fBody->end() && *itEnd != '\r' && *itEnd != '\n' ) ++itEnd;
	return std::string(itBegin,itEnd);
}


ParserVcf::~ParserVcf()
{
	delete pim;
}


// ===================================== ParserTabSeparated ================================

struct ParserTabSeparated::Pim
{
	std::shared_ptr<std::vector<char>> fBody;
	std::vector<char>::const_iterator fPosition;
};


ParserTabSeparated::ParserTabSeparated(std::shared_ptr<std::vector<char>> body) : pim(new Pim)
{
	pim->fBody = body;
	pim->fPosition = body->begin();
}


bool ParserTabSeparated::parseRecords(std::vector<std::vector<std::string>> & out, std::vector<unsigned> & outLines, unsigned maxRecordsCount)
{
	out.clear();
	out.reserve(maxRecordsCount);
	outLines.clear();
	outLines.reserve(maxRecordsCount);

	std::vector<char>::const_iterator & iBody = pim->fPosition;
	std::vector<char>::const_iterator const iEnd = pim->fBody->end();
	std::vector<std::string> colData;

	while ( out.size() < maxRecordsCount && iBody < iEnd ) {
		colData.clear();
		// ---- parse a line
		auto iWord = iBody;
		auto const iLine = iBody;
		while ( iBody < iEnd && *iBody != '\n' && *iBody != '\r' ) {
			if (*iBody == '\t') {
				colData.push_back( std::string(iWord,iBody) );
				iWord = ++iBody;
			} else {
				++iBody;
			}
		}
		colData.push_back( std::string(iWord,iBody) ); // last word/column
		out.push_back(colData);
		outLines.push_back(iLine - pim->fBody->begin());
		// ---- eat EOL character(s) - LF, CR or CR+LF
		if ( iBody < iEnd && *iBody == '\r' ) ++iBody;
		if ( iBody < iEnd && *iBody == '\n' ) ++iBody;
	}

	return (! out.empty());
}


uint64_t ParserTabSeparated::numberOfParsedBytes() const
{
	return (pim->fPosition - pim->fBody->begin());
}


std::string ParserTabSeparated::lineByOffset(unsigned lineOffset) const
{
	std::vector<char>::const_iterator itBegin = pim->fBody->begin() + lineOffset;
	auto itEnd = itBegin;
	while ( itEnd < pim->fBody->end() && *itEnd != '\r' && *itEnd != '\n' ) ++itEnd;
	return std::string(itBegin,itEnd);
}


ParserTabSeparated::~ParserTabSeparated()
{
	delete pim;
}

// =========================================== ParserVcf2 ==============================


struct ParserVcf2::Pim
{
	std::vector<ReferenceId> fRefIdByChromosome;
	std::shared_ptr<std::vector<char>> fBody;
	std::vector<char>::const_iterator fPositionParsing;
	std::vector<char>::const_iterator fPositionPrinting;
	unsigned fLineNumber = 0;
	std::vector<unsigned> fIdsPositionsInLinesToReturn;
};


ParserVcf2::ParserVcf2(std::shared_ptr<std::vector<char>> body, std::vector<ReferenceId> const & refIdByChromosome) : pim(new Pim)
{
	ASSERT( refIdByChromosome.size() > Chromosome::chrM );
	pim->fRefIdByChromosome = refIdByChromosome;
	pim->fBody = body;
	pim->fPositionParsing = pim->fPositionPrinting = body->begin();
}


ParserVcf2::~ParserVcf2()
{
	delete pim;
}


// ------
inline std::vector<char>::const_iterator parseWordTillTab(std::vector<char>::const_iterator it, std::vector<char>::const_iterator const & iE)
{
	if (it >= iE || *it == '\t') throw std::runtime_error("Missing value (a column with empty string was spotted)");
	for ( ++it;  it < iE;  ++it ) {
		if (*it == '\t') return it;
	}
	throw std::runtime_error("Missing columns");
}
// ------ iB < iE on the input, iB == iE at the end
inline Chromosome parseChromosome(std::vector<char>::const_iterator & iB, std::vector<char>::const_iterator const & iE)
{
	std::runtime_error wrongChromosome("Incorrect chromosome number");
	if (*iB == 'c') {
		if (++iB >= iE) throw wrongChromosome;
		if (*iB != 'h') throw wrongChromosome;
		if (++iB >= iE) throw wrongChromosome;
		if (*iB != 'r') throw wrongChromosome;
		if (++iB >= iE) throw wrongChromosome;
	}
	Chromosome r;
	// parse number / X,Y,M,MT
	if (*iB >= '0' && *iB <= '9') {
		unsigned no = *iB - '0';
		if (++iB < iE) {
			if (*iB < '0' || *iB > '9') throw wrongChromosome;
			no *= 10;
			no += *iB - '0';
			if (++iB < iE) throw wrongChromosome;
		}
		if (no < 1 || no > 22) throw wrongChromosome;
		r = static_cast<Chromosome>(no);
	} else if (*iB == 'X') {
		r = Chromosome::chrX;
		if (++iB < iE) throw wrongChromosome;
	} else if (*iB == 'Y') {
		r = Chromosome::chrY;
		if (++iB < iE) throw wrongChromosome;
	} else if (*iB == 'M') {
		r = Chromosome::chrM;
		if (++iB < iE) {
			if (*iB == 'T') ++iB;
			if (iB < iE) throw wrongChromosome;
		}
	} else {
		throw wrongChromosome;
	}
	return r;
}
// ---
inline unsigned parsePosition(std::vector<char>::const_iterator & iB, std::vector<char>::const_iterator const & iE)
{
	unsigned no = 0;
	for (; iB < iE; ++iB) {
		if (*iB < '0' || *iB > '9') throw std::runtime_error("Incorrect position");
		no *= 10;
		no += *iB - '0';
	}
	return no;
}
// ---
inline std::string parseSequence(std::vector<char>::const_iterator & iB, std::vector<char>::const_iterator const & iE, std::string const reference = "")
{
	if (*iB == '.') {
		if (++iB < iE) throw std::runtime_error("Unexpected character anfter '.'");
		if (reference == "") throw std::runtime_error("Reference sequence cannot be set to missing value ('.')");
		return reference;
	}
	std::string s(iB,iE);
	for ( ;  iB < iE;  ++iB ) {
		if (*iB == ',') throw std::runtime_error("Multiallelic records are not supported");
		if ( ! (*iB == 'A' || *iB == 'C' || *iB == 'G' || *iB == 'T') ) throw std::runtime_error("Incorrect allele");
	}
	return s;
}


bool ParserVcf2::parseRecords(std::vector<PlainVariant> & out, unsigned maxRecordsCount)
{
	ASSERT(pim->fPositionParsing == pim->fPositionPrinting);
	out.clear();
	out.reserve(maxRecordsCount);
	pim->fIdsPositionsInLinesToReturn.clear();
	pim->fIdsPositionsInLinesToReturn.reserve(maxRecordsCount);

	std::vector<char>::const_iterator & iBody = pim->fPositionParsing;
	std::vector<char>::const_iterator const iEnd = pim->fBody->end();

	// ========================================== header =============================================
	if (iBody == pim->fBody->begin()) {  // first call - parse the header
		while ( iBody < iEnd ) {
			++(pim->fLineNumber);
			// ---- parse a line
			auto iLine = iBody;
			while ( iBody < iEnd && *iBody != '\n' && *iBody != '\r' ) {
				++iBody;
			}
			std::string line(iLine,iBody);
			// ---- eat EOL character(s) - LF, CR or CR+LF
			if ( iBody < iEnd && *iBody == '\r' ) ++iBody;
			if ( iBody < iEnd && *iBody == '\n' ) ++iBody;
			// ---- parse header line
			if ( line.front() != '#') {
				throw ExceptionVcfParsingError(pim->fLineNumber, "Unexpected data line (header line was expected)");
			}
			std::string const headerLine = "#CHROM\tPOS\tID\tREF\tALT\tQUAL\tFILTER\tINFO";
			if ( line.substr(0,headerLine.size()) == headerLine ) break; // end of header
		}
	}

	// ========================================== data =============================================
	while ( out.size() < maxRecordsCount && iBody < iEnd ) {
		++(pim->fLineNumber);
		pim->fIdsPositionsInLinesToReturn.push_back( iBody - pim->fBody->begin() );
		try {
			// ---- parse a line
			auto iWordEnd = parseWordTillTab(iBody, iEnd);
			Chromosome const chromosome = parseChromosome(iBody, iWordEnd);
			iWordEnd = parseWordTillTab(++iBody, iEnd);
			unsigned const pos = parsePosition(iBody, iWordEnd);
			iWordEnd = parseWordTillTab(++iBody, iEnd);
			// parse IDs
			pim->fIdsPositionsInLinesToReturn.back() = (iBody - pim->fBody->begin());
			iBody = iWordEnd;
			iWordEnd = parseWordTillTab(++iBody, iEnd);
			std::string const ref = parseSequence(iBody, iWordEnd);
			iWordEnd = parseWordTillTab(++iBody, iEnd);
			std::string const alt = parseSequence(iBody, iWordEnd, ref);
			// build document
			PlainVariant pv;
			pv.refId = pim->fRefIdByChromosome[chromosome];
			pv.modifications.resize(1);
			pv.modifications.front().region.setLeftAndLength(pos-1,ref.size());
			pv.modifications.front().originalSequence = ref;
			pv.modifications.front().newSequence = alt;
			// save line
			out.push_back(pv);
		} catch (std::exception const & e) {
			out.push_back(PlainVariant());
		}
		// go to the end of line
		while ( iBody < iEnd && *iBody != '\n' && *iBody != '\r' ) ++iBody;
		// ---- eat EOL character(s) - LF, CR or CR+LF
		if ( iBody < iEnd && *iBody == '\r' ) ++iBody;
		if ( iBody < iEnd && *iBody == '\n' ) ++iBody;
	}

	return (! out.empty());
}


void ParserVcf2::buildVcfResponse(std::ostream & out, std::vector<std::vector<std::string>> const & recordsIds)
{
	ASSERT( pim->fPositionParsing > pim->fPositionPrinting );
	ASSERT( pim->fIdsPositionsInLinesToReturn.size() == recordsIds.size() );

	// ---- process identifiers
	for ( unsigned iR = 0;  iR < recordsIds.size();  ++iR ) {

		std::vector<std::string> const & newIds = recordsIds[iR];
		if (newIds.empty()) continue;  // the line is unchanged and will be printed later

		{ // print first part of the line
			unsigned const toPrint = pim->fBody->begin() + pim->fIdsPositionsInLinesToReturn[iR] - pim->fPositionPrinting;
			out.write( &*(pim->fPositionPrinting), toPrint );
			pim->fPositionPrinting += toPrint;
		}

		// parse current ids
		std::set<std::string> currentIds;
		if ( *(pim->fPositionPrinting) == '.') {
			++(pim->fPositionPrinting);
			if ( *(pim->fPositionPrinting) != '\t' ) --(pim->fPositionPrinting);
		}
		std::vector<char>::const_iterator itId = pim->fPositionPrinting;
		while ( *(itId) != '\t' ) {
			std::vector<char>::const_iterator itNext = itId;
			while ( *itNext != ',' && *itNext != '\t' ) ++itNext;
			currentIds.insert( std::string(itId,itNext) );
			if ( *itNext != '\t' ) ++itNext;
			itId = itNext;
		}
		{ // print parsed IDs
			unsigned const toPrint = itId - pim->fPositionPrinting;
			out.write( &*(pim->fPositionPrinting), toPrint );
			pim->fPositionPrinting += toPrint;
		}

		// print new ids
		for (auto const & id: newIds) {
			if (currentIds.count(id)) continue;
			if (! currentIds.empty()) out << ",";
			currentIds.insert(id);
			out << id;
		}

	}

	{ // ---- print remaining stuff
		unsigned const toPrint = pim->fPositionParsing - pim->fPositionPrinting;
		out.write( &*(pim->fPositionPrinting), toPrint );
		pim->fPositionPrinting += toPrint;
	}
}

uint64_t ParserVcf2::numberOfParsedBytes() const
{
	return (pim->fPositionParsing - pim->fBody->begin());
}



