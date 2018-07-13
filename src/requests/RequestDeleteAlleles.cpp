#include "requests.hpp"
#include "hgvs.hpp"
#include "canonicalization.hpp"
#include "InputTools.hpp"
#include "identifiersTools.hpp"
#include "../commonTools/Stopwatch.hpp"
#include "../externalSources/externalSources.hpp"
#include <memory>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include "parsers.hpp"


struct RequestDeleteAlleles::Pim {
	std::shared_ptr<std::vector<char>> body;
};


RequestDeleteAlleles::RequestDeleteAlleles
	( std::shared_ptr<std::vector<char>> pBody
	, std::string const & columnsDefinitions
	) : Request(documentType::activeGenomicVariant), pim(new Pim)
{
	std::unique_ptr<Pim> pimGuard(pim);
	pim->body = pBody;
	if (columnsDefinitions != "id") {
		throw std::runtime_error("Only file=id is accepted for this request");
	}
	ASSERT( pBody != nullptr );
	pimGuard.release();
}


RequestDeleteAlleles::~RequestDeleteAlleles()
{
	stopProcessingThread();
	delete pim;
}


inline DocumentActiveGenomicVariant & asGenomicVariant(Document & doc)
{
	if (doc.isNull()) doc = DocumentActiveGenomicVariant();
	if (! doc.isActiveGenomicVariant())	throw ExceptionLineParsingError("There are key columns with definition of two different variants");
	return doc.asActiveGenomicVariant();
}
inline DocumentActiveProteinVariant & asProteinVariant(Document & doc)
{
	if (doc.isNull()) doc = DocumentActiveProteinVariant();
	if (! doc.isActiveProteinVariant())	throw ExceptionLineParsingError("There are key columns with definition of two different variants");
	return doc.asActiveProteinVariant();
}


void RequestDeleteAlleles::process()
{
	if (logLogin != "admin") {
		throw ExceptionAuthorizationError("You need to be admin to perform this operation");
	}

	std::unique_ptr<Parser> parser;
	parser.reset( new ParserTabSeparated(pim->body) );

	unsigned const chunkSize = 256*1024;
	std::vector<unsigned> inputLines;
	std::vector<std::vector<std::string>> parsedLines;
	std::vector<Document> documents;
	inputLines.reserve(chunkSize);
	parsedLines.reserve(chunkSize);
	documents.reserve(chunkSize);

	while (true) {
		documents.clear();

		// ---- logs
		std::string const logPrefix = boost::lexical_cast<std::string>(parser->numberOfParsedBytes()/(1024*1024)) + " MB -> ";
		Stopwatch stopwatch;
		std::cout << logPrefix << "parse payload ... " << std::flush;

		if ( ! parser->parseRecords(parsedLines,inputLines,chunkSize) ) {
			break; // no more data
		}

		// ====================== parse & convert to canonical allele

		// ---- convert parsed words to documents
		for (auto const & colData: parsedLines) {
			try {
				if (colData.size() != 1) throw ExceptionLineParsingError("Incorrect number of columns");
				Document doc;
				Identifiers ids;
				std::string const & word = colData[0];
				try {
					if (word.size() > 0 && word[0] == 'P') {
						asProteinVariant(doc).caId = parsePA(word);
					} else {
						asGenomicVariant(doc).caId = parseCA(word);
					}
				} catch (std::runtime_error const & e) {
					throw ExceptionLineParsingError(e.what());
				}
				documents.push_back( doc );
			} catch (...) {
				documents.push_back( DocumentError::createFromCurrentException() );
			}
		}

		// ====================== fetch definitions
		allelesDb->fetchVariantsByCaPaIds(documents);

		// ====================== delete links from external sources
		std::cout << stopwatch.save_and_restart_sec() << " sec\n" << logPrefix << "update externalSources ... " << std::flush;
		{
			// prepare parameters
			std::vector<uint32_t> ids;
			for ( unsigned i = 0;  i < documents.size();  ++i ) {
				CanonicalId caId;
				if (documents[i].isActiveGenomicVariant()) {
					caId = documents[i].asActiveGenomicVariant().caId;
				} else {
					caId = documents[i].asActiveProteinVariant().caId;
				}
				if ( ! caId.isNull() ) {
					ids.push_back(caId.value);
				}
			}
			// call procedure
			externalSources::deleteLinks(ids);
		}

		// ====================== delete record
		if (killThread.load()) throw ExceptionRequestTerminated();
		std::cout << stopwatch.save_and_restart_sec() << " sec\n" << logPrefix << "fetch data ... " << std::flush;
		allelesDb->fetchVariantsByDefinitionAndDelete(documents);

		// ====================== fill inputLine for error objects
		for (unsigned i = 0; i < documents.size(); ++i) {
			if (documents[i].isError()) documents[i].error().fields[label::inputLine] = inputLines[i];
		}

		// ====================== build response
		std::cout << stopwatch.save_and_restart_sec() << " sec\n" << logPrefix << "build & send response ... " << std::flush;
		addChunkOfResponse( documents );
		std::cout << stopwatch.save_and_restart_sec() << " sec" << std::endl;
	}
}
