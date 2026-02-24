#include "requests.hpp"
#include "hgvs.hpp"
#include "InputTools.hpp"
#include "identifiersTools.hpp"
#include "../commonTools/Stopwatch.hpp"
#include <memory>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include "parsers.hpp"
#include "coordinateTransformationHelper.hpp"

struct RequestCoordinateTransformations::Pim {
	std::shared_ptr<std::vector<char>> body;
	std::vector<std::string> columns;
	std::string fileDescription;
};

std::vector<std::string> parseFileFormat3(std::string const & fileDescription)
{
	std::vector<std::string> columnsDefs;

	if (fileDescription == "acse") {
		columnsDefs.push_back("assembly");
		columnsDefs.push_back("chromosome");
		columnsDefs.push_back("cstart");
		columnsDefs.push_back("cend");
	}
	else if (fileDescription == "glDesc") {
		columnsDefs.push_back("assembly");
		columnsDefs.push_back("chromosome");
		columnsDefs.push_back("cstart");
		columnsDefs.push_back("cend");
	}
	else {
		throw ExceptionIncorrectRequest("Unknown column name: '" + fileDescription + "'.");
	}

	for(auto & a : columnsDefs){
		std::cerr << "Column => " << a << std::endl;
	}

	return columnsDefs;
}


RequestCoordinateTransformations::RequestCoordinateTransformations
	( std::shared_ptr<std::vector<char>> pBody
	, std::string const & columnsDefinitions) : Request(documentType::coordinateTransform), pim(new Pim) 
	{
		std::unique_ptr<Pim> pimGuard(pim);
		pim->body = pBody;
		// If the file contains information about assembly coordinate start and end
		pim->fileDescription = columnsDefinitions;
		if (columnsDefinitions == "acse" || columnsDefinitions == "glDesc")
			pim->columns = parseFileFormat3(columnsDefinitions);
		else
			throw ExceptionIncorrectRequest("The 'file' parameter must have a value of 'acse' or 'glDesc' i.e. ?file=acse or ?file=glDesc");

		ASSERT( pBody != nullptr );
		pimGuard.release();
	}

RequestCoordinateTransformations::~RequestCoordinateTransformations()
{
	stopProcessingThread();
	delete pim;
}

void RequestCoordinateTransformations::process()
{

	ASSERT(! pim->columns.empty());

	std::unique_ptr<Parser> parser;
	if(pim->fileDescription == "acse") {
		parser.reset( new ParserTabSeparated(pim->body) );
	} else
	if(pim->fileDescription == "glDesc") {
		parser.reset( new ParserGlDescriptor(pim->body) );
	}

	// streaming the file
	unsigned const chunkSize = 256*1024;

	std::vector<unsigned> columnsIds;
	columnsIds.reserve(pim->columns.size());
	// std::cerr << "Size of the column is => '" << pim->columns.size() << "'" << std::endl;

	for (unsigned i = 0; i < pim->columns.size(); ++i) {
		columnsIds.push_back(i);
	}

	std::vector<unsigned> inputLines;
	std::vector<std::vector<std::string>> parsedLines;
	std::vector<Document> documents;
	textInput v;

	inputLines.reserve(chunkSize);
	parsedLines.reserve(chunkSize);
	documents.reserve(chunkSize);

	std::stringstream out;

	while(true){

		documents.clear();

		std::string const logPrefix = boost::lexical_cast<std::string>(parser->numberOfParsedBytes()/(1024*1024)) + " MB -> ";
		Stopwatch stopwatch;

		if ( ! parser->parseRecords(parsedLines,inputLines,chunkSize) ) {
			break; 
		}

		for(auto const & colData: parsedLines){
			std::string assembly = "";
			std::string chromosome = "";
			unsigned int cstart = 0 , cend = 0;
			v = {assembly, chromosome, cstart, cend};
			try{
				if(colData.size() != 4 && colData.size() != 3){
					throw ExceptionLineParsingError("Need four tab delimited columns per line (assembly, chromosome, start, end), or three tab delimited columns (assmebly, chromosome, start)");
				}
				else {
					try {
						assembly = colData[0];
						chromosome = colData[1];
						if(chromosome.find("chr") != std::string::npos) chromosome = chromosome.substr(3);
						cstart = std::stoi(colData[2]);
						cend   = (colData.size() == 4)?std::stoi(colData[3]):std::stoi(colData[2]);
						v = {assembly, chromosome, cstart, cend};
						genomicCoordinateTransformer gct(referencesDb, v);
						DocumentCoordinates doc;
						bool first = true;
						for(auto & coord : gct.transformation){
							if(coord.assembly == ReferenceGenome::rgGRCh38){
								doc.grch38Assembly = toString(coord.assembly);
								doc.grch38Chr = toString(coord.chr);
								doc.grch38Start = coord.region.left();
								doc.grch38End = coord.region.right();
								if(first) first = false; else out << '\t';
								if(colData.size()==4) out << doc.grch38Assembly << " (chr" << doc.grch38Chr << ":" << doc.grch38Start << "-" << doc.grch38End << ")";
								else out << doc.grch38Assembly << " (chr" << doc.grch38Chr << ":" << doc.grch38Start << ")";
							}
							else if (coord.assembly == ReferenceGenome::rgGRCh37){
								doc.grch37Assembly = toString(coord.assembly);
								doc.grch37Chr = toString(coord.chr);
								doc.grch37Start = coord.region.left();
								doc.grch37End = coord.region.right();
								if(first) first = false; else out << '\t';
								if(colData.size()==4) out << doc.grch37Assembly << " (chr" << doc.grch37Chr << ":" << doc.grch37Start << "-" << doc.grch37End << ")";
								else out << doc.grch37Assembly << " (chr" << doc.grch37Chr << ":" << doc.grch37Start << ")";
							}
							else if(coord.assembly == ReferenceGenome::rgNCBI36) {
								doc.ncbi36Assembly = toString(coord.assembly);
								doc.ncbi36Chr = toString(coord.chr);
								doc.ncbi36Start = coord.region.left();
								doc.ncbi36End = coord.region.right();
								if(first) first = false; else out << '\t';
								if(colData.size()==4) out << doc.ncbi36Assembly << " (chr" << doc.ncbi36Chr << ":" << doc.ncbi36Start << "-" << doc.ncbi36End << ")";
								else out << doc.ncbi36Assembly << " (chr" << doc.ncbi36Chr << ":" << doc.ncbi36Start << ")";
							}	
						}
						out << "\n";
						documents.push_back(doc);
					}
					catch(const std::runtime_error& error){
						std::string chromosome = colData[1];
						if(chromosome.find("chr") != std::string::npos) chromosome = chromosome.substr(3);
						if(colData.size()==4) out << (colData[0]) << " (chr" << (chromosome) << ":" << std::stoi(colData[2]) << "-" << std::stoi(colData[3]) << ")" << '\t' << "ERROR:" << error.what() << std::endl;
						else out << (colData[0]) << " (chr" << (chromosome) << ":" << std::stoi(colData[2]) << ")" << '\t' << "ERROR:" << error.what() << std::endl;
						throw std::runtime_error(error.what());
					}
					catch(...){
						throw ExceptionLineParsingError("Exception occured while parsing columns, likely it did not have expected input.");
					}
				}
			}
			catch(...){
				documents.push_back( DocumentError::createFromCurrentException() );
			}
		}

		std::cout << stopwatch.save_and_restart_sec() << " sec\n" << logPrefix << "build & send response ... " << std::flush;
		if(pim->fileDescription == "acse") {
			addChunkOfResponse( documents );
		}
		std::cout << stopwatch.save_and_restart_sec() << " sec\n" << logPrefix << "build & send response ... " << std::flush;
	}
	if(pim->fileDescription == "glDesc") {
		setResponse(out.str());
	}
}
