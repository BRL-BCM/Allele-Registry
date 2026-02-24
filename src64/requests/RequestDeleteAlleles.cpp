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
#include <fstream>
#include <boost/thread/thread.hpp>
#include <chrono>

using namespace std::chrono;

struct RequestDeleteAlleles::Pim {
	std::ofstream payload;
	std::shared_ptr<std::vector<char>> body;
};


RequestDeleteAlleles::RequestDeleteAlleles
	( std::shared_ptr<std::vector<char>> pBody
	, std::string const & columnsDefinitions
	) : Request(documentType::activeGenomicVariant), pim(new Pim)
{
	std::unique_ptr<Pim> pimGuard(pim);
	pid_t pid = getpid();
	milliseconds ms = duration_cast< milliseconds >( system_clock::now().time_since_epoch() );
	std::stringstream ss;
	ss << configuration.payloads_path << "/payload" << "-" << pid << "-" << boost::this_thread::get_id() << "-" << ms.count();
	pim->payload.open(ss.str());
	std::cout << "PayloadFileName: " << ss.str() << std::endl;
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
	pim->payload.close();
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

	pim->payload << "#\t" << logTime << "\t" << logLogin << "\t" << logMethod << "\t" << logRequest << "\n";

	std::unique_ptr<Parser> parser;
	parser.reset( new ParserTabSeparated(pim->body) );

	parser->logPayload(pim->payload);

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
			std::vector<uint64_t> ids;
			for ( unsigned i = 0;  i < documents.size();  ++i ) {
				CanonicalId caId;
				if (documents[i].isActiveGenomicVariant()) {
					caId = documents[i].asActiveGenomicVariant().caId;
				} if(documents[i].isActiveProteinVariant()) {
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
			if (documents[i].isError()) {
				if(parsedLines[i].size() != 1) {
					documents[i].error().fields[label::inputLine] = "Incorrect number of columns";
					continue;
				}
				std::string const & word = parsedLines[i][0];
				documents[i].error().fields[label::inputLine] = word;
			}
		}

		// ====================== build response
		std::cout << stopwatch.save_and_restart_sec() << " sec\n" << logPrefix << "build & send response ... " << std::flush;
		addChunkOfResponse( documents );
		std::cout << stopwatch.save_and_restart_sec() << " sec" << std::endl;
	}
}
