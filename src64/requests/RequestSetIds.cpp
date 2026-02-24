#include "requests.hpp"
#include "parsers.hpp"
#include "canonicalization.hpp"
#include "../commonTools/Stopwatch.hpp"
#include <sstream>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/thread/thread.hpp>
#include <chrono>

using namespace std::chrono;

struct FileColumn
{
	enum columnType {
		  ignore = 0
		, definitionWithReference   // from VCF parsing
		, hgvs
		, identifier
		, clinvarPreferredName
		, clinvarRCVs
	};
	columnType colType;
	identifierType idType = identifierType::CA;
	std::string sourceName = "";
	FileColumn(columnType pType) : colType(pType) { ASSERT(pType != columnType::identifier); }
	FileColumn(identifierType pType) : colType(columnType::identifier), idType(pType) {}
};

struct RequestSetIds::Pim {
	std::ofstream payload;
	std::shared_ptr<std::vector<char>> body;
	std::vector<FileColumn> columns;
};

RequestSetIds::RequestSetIds(std::shared_ptr<std::vector<char>> pBody)
: Request(documentType::activeGenomicVariant), pim(new Pim)
{
	std::unique_ptr<Pim> pimGuard(pim);
	pid_t pid = getpid();
	milliseconds ms = duration_cast< milliseconds >( system_clock::now().time_since_epoch() );
	std::stringstream ss;
	ss << configuration.payloads_path << "/payload" << "-" << pid << "-" << boost::this_thread::get_id() << "-" << ms.count();
	pim->payload.open(ss.str());
	std::cout << "PayloadFileName: " << ss.str() << std::endl;
	pim->body = pBody;
	ASSERT( pBody != nullptr );
	pimGuard.release();
}

RequestSetIds::~RequestSetIds()
{
	stopProcessingThread();
	pim->payload.close();
	delete pim;
}

void RequestSetIds::process()
{
	pim->payload << "#\t" << logTime << "\t" << logLogin << "\t" << logMethod << "\t" << logRequest << "\n";
	if (logLogin != "admin") {
		throw ExceptionAuthorizationError("You need to be admin to perform this operation");
	}
	std::unique_ptr<Parser> parser;
	parser.reset( new ParserTabSeparated(pim->body) );

	unsigned const chunkSize = 64*1024;
	std::vector<unsigned> inputLines;
	std::vector<std::vector<std::string>> parsedLines;
	std::vector<Document> documents;

	inputLines.reserve(chunkSize);
	parsedLines.reserve(chunkSize);
	documents.reserve(chunkSize);

	parser->logPayload(pim->payload);
	pim->payload.flush();

	std::vector<unsigned> columnsIds;
	columnsIds.push_back(0);
	pim->columns.push_back(FileColumn(FileColumn::hgvs));
	pim->columns.push_back(FileColumn(identifierType::CA));

	while (true) {
		documents.clear();
		if ( ! parser->parseRecords(parsedLines,inputLines,chunkSize) ) {
			break; // no more data
		}

		std::vector<std::string> ids;

		for (auto const & colData: parsedLines) {
			FileColumn const & colDefHgvs = pim->columns[0];
			std::string const & wordHgvs = colData[0];
			std::cout << "hgvs:\t" << wordHgvs << std::endl;
			FileColumn const & colDefId = pim->columns[1];
			std::string const & wordId = colData[1];
			ids.push_back(wordId);

			HgvsVariant hgvsVar(true);
			decodeHgvs(referencesDb, wordHgvs, hgvsVar);
			Document doc;
			if (hgvsVar.isGenomic()) {
				doc = DocumentActiveGenomicVariant(canonicalize(referencesDb, hgvsVar.genomic));
			} else {
				doc = DocumentActiveProteinVariant(canonicalize(referencesDb, hgvsVar.protein));
			}
			documents.push_back(doc);
		}

		mapVariantsToMainGenome(documents);
		allelesDb->fetchVariantsByDefinition(documents);
		std::cout << "fetch" << std::endl;
		for ( unsigned i = 0;  i < documents.size();  ++i ) {
			CanonicalId caId;
			if (documents[i].isActiveGenomicVariant()) {
				caId = documents[i].asActiveGenomicVariant().caId;
				if (!caId.isNull()) {
					std::cout << "CA" << caId.value << std::endl;
				} else {
					std::cout << "CA:_" << std::endl;
				}
			}
		}
		allelesDb->deleteVariantsByDefinition(documents);
		for ( unsigned i = 0;  i < documents.size();  ++i ) {
			std::stringstream ss;
			ss << ids[i].substr(2);
			uint64_t id;
			ss >> id;
			documents[i].asActiveGenomicVariant().caId = CanonicalId(id);
		}
		std::cout << "delete" << std::endl;
		for ( unsigned i = 0;  i < documents.size();  ++i ) {
			CanonicalId caId;
			if (documents[i].isActiveGenomicVariant()) {
				caId = documents[i].asActiveGenomicVariant().caId;
				if (!caId.isNull()) {
					std::cout << "CA" << caId.value << std::endl;
				} else {
					std::cout << "CA:_" << std::endl;
				}
			}
		}
		allelesDb->fetchVariantsByDefinitionAndAddIdentifiers(documents);
		std::cout << "new" << std::endl;
		for ( unsigned i = 0;  i < documents.size();  ++i ) {
			CanonicalId caId;
			if (documents[i].isActiveGenomicVariant()) {
				caId = documents[i].asActiveGenomicVariant().caId;
				if (!caId.isNull()) {
					std::cout << "CA" << caId.value << std::endl;
				} else {
					std::cout << "CA:_" << std::endl;
				}
			}
		}

		addChunkOfResponse( documents );

	}

}

