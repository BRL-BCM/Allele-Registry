#include "requests.hpp"
#include "hgvs.hpp"
#include "canonicalization.hpp"
#include "InputTools.hpp"
#include "identifiersTools.hpp"
#include "../commonTools/Stopwatch.hpp"
#include <memory>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include "parsers.hpp"
#include "../externalSources/externalSources.hpp"


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


struct RequestFetchAllelesByDefinition::Pim {
	std::shared_ptr<std::vector<char>> body;
	bool vcfFile;
	bool registerIfNotFound;
	std::vector<FileColumn> columns;
};


std::vector<FileColumn> parseFileFormat(std::string const & fileDescription)
{
	std::vector<std::string> cols;
	boost::split(cols, fileDescription, boost::is_any_of("+") );
	std::vector<FileColumn> columnsDefs;
	std::set<std::string> allNotIgnoredColumns;
	for (auto const & c: cols) {
		if      (c == ""    ) columnsDefs.push_back(FileColumn(FileColumn::ignore));
		else if (c == "hgvs") columnsDefs.push_back(FileColumn(FileColumn::hgvs));
		else if (c == "id"                   ) columnsDefs.push_back(FileColumn(identifierType::CA));
		else if (c == "dbSNP.rs"             ) columnsDefs.push_back(FileColumn(identifierType::dbSNP));
		else if (c == "ClinVar.alleleId"     ) columnsDefs.push_back(FileColumn(identifierType::ClinVarAllele));
		else if (c == "ClinVar.variationId"  ) columnsDefs.push_back(FileColumn(identifierType::ClinVarVariant));
		else if (c == "MyVariantInfo_hg19.id") columnsDefs.push_back(FileColumn(identifierType::MyVariantInfo_hg19));
		else if (c == "MyVariantInfo_hg38.id") columnsDefs.push_back(FileColumn(identifierType::MyVariantInfo_hg38));
		else if (c == "ExAC.id"              ) columnsDefs.push_back(FileColumn(identifierType::ExAC));
		else if (c == "gnomAD.id"            ) columnsDefs.push_back(FileColumn(identifierType::gnomAD));
		else if (c == "ClinVar.preferredName") columnsDefs.push_back(FileColumn(FileColumn::clinvarPreferredName));
		else if (c == "ClinVar.RCV"          ) columnsDefs.push_back(FileColumn(FileColumn::clinvarRCVs));
		else if (c == "COSMIC.id"            ) columnsDefs.push_back(FileColumn(identifierType::COSMIC));
		else if (c.substr(0,15) == "externalSource.") {
			columnsDefs.push_back(FileColumn(identifierType::externalSource));
			columnsDefs.back().sourceName = c.substr(15);
		} else {
			throw ExceptionIncorrectRequest("Unknown column name: '" + c + "'.");
		}
		if (c != "") {
			if ( ! allNotIgnoredColumns.insert(c).second ) {
				throw ExceptionIncorrectRequest("The 'file' parameter cannot have duplicate columns. The following column name was used more than once: '" + c + "'.");
			}
		}
	}
	return columnsDefs;
}


RequestFetchAllelesByDefinition::RequestFetchAllelesByDefinition
	( std::shared_ptr<std::vector<char>> pBody
	, std::string const & columnsDefinitions
	, bool registerNewAlleles
	) : Request(documentType::activeGenomicVariant), pim(new Pim)
{
	std::unique_ptr<Pim> pimGuard(pim);
	pim->body = pBody;
	if (columnsDefinitions == "vcf") {
		pim->vcfFile = true;
		pim->columns.push_back(FileColumn(FileColumn::definitionWithReference));
	} else {
		pim->vcfFile = false;
		pim->columns = parseFileFormat(columnsDefinitions);
	}
	pim->registerIfNotFound = registerNewAlleles;
	ASSERT( pBody != nullptr );
	pimGuard.release();
}


RequestFetchAllelesByDefinition::~RequestFetchAllelesByDefinition()
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
template<class tDef>
inline void setDefinition(tDef & currentDef, tDef const & newDef)
{
	if (currentDef.refId.isNull()) currentDef = newDef;
	else if (currentDef != newDef) throw ExceptionLineParsingError("There are key columns with definition of two different variants");
}


void RequestFetchAllelesByDefinition::process()
{
	ASSERT(! pim->columns.empty());

	// POST requests works only for single columns
	if ( pim->columns.size() > 1 && ! pim->registerIfNotFound ) {
		throw ExceptionIncorrectRequest("For POST requests the 'file' parameter must contain exactly one column");
	}

	// analyze key columns definition
	unsigned keyColumnId = std::numeric_limits<unsigned>::max();
	unsigned ignoredColumns = 0;
	for ( unsigned i = 0;  i < pim->columns.size();  ++i ) {
		auto const & c = pim->columns[i];
		if ( c.colType == FileColumn::hgvs
			|| c.colType == FileColumn::definitionWithReference
			|| ( c.colType == FileColumn::identifier
				&& ( c.idType == identifierType::ExAC
					|| c.idType == identifierType::gnomAD
					|| c.idType == identifierType::MyVariantInfo_hg19
					|| c.idType == identifierType::MyVariantInfo_hg38
			)	)	)
		{
			if (keyColumnId < pim->columns.size()) {
				if ( pim->columns[keyColumnId].colType == FileColumn::identifier && pim->columns[keyColumnId].idType == identifierType::CA ) {
					// exception for importing CA/PA
				} else {
					throw ExceptionIncorrectRequest("The 'file' parameter cannot contain more than one of the key columns: hgvs, id, MyVariantInfo.hg19id, MyVariantInfo.hg38id, ExAC.id, gnomAD.id");
				}
			}
			keyColumnId = i;
		} else if ( c.colType == FileColumn::identifier && c.idType == identifierType::CA ) {
			if (keyColumnId < pim->columns.size()) {
				// exception for importing CA/PA
			} else {
				keyColumnId = i;
			}
		} else if ( c.colType == FileColumn::ignore ) {
			++ignoredColumns;
		}
	}
	if (keyColumnId >= pim->columns.size()) {
		throw ExceptionIncorrectRequest("The 'file' parameter must contain one of the key columns: hgvs, id, MyVariantInfo.hg19id, MyVariantInfo.hg38id, ExAC.id, gnomAD.id");
	}

	// analyze external sources columns
	std::map<std::string,unsigned> extSrcName2inputOrder;
	for (auto const & c: pim->columns) {
		if (c.colType == FileColumn::identifier && c.idType == identifierType::externalSource) {
			if ( ! externalSources::authorization( logLogin, c.sourceName ) ) {
				throw ExceptionAuthorizationError("You do not have privileges to modify links in the external source '" + c.sourceName + "'");
			}
			extSrcName2inputOrder[c.sourceName] = extSrcName2inputOrder.size();
		}
	}

	// check privileges
	if ( pim->columns.size() > 1 + extSrcName2inputOrder.size() + ignoredColumns ) {  // key + extSrcColumns + ignoredColumns
		if (logLogin != "admin") {
			throw ExceptionAuthorizationError("You need to be admin to perform this operation");
		}
	}

	// adjust order in which column are processed
	std::vector<unsigned> columnsIds;
	columnsIds.reserve(pim->columns.size() - ignoredColumns);
	columnsIds.push_back(keyColumnId);
	for (unsigned i = 0; i < pim->columns.size(); ++i) {
		if ( i != keyColumnId && pim->columns[i].colType != FileColumn::ignore ) {
			columnsIds.push_back(i);
		}
	}

	std::unique_ptr<Parser> parser;
	if (pim->vcfFile) {
		parser.reset( new ParserVcf(pim->body) );
	} else {
		parser.reset( new ParserTabSeparated(pim->body) );
	}

	unsigned const chunkSize = 256*1024;
	std::vector<unsigned> inputLines;
	std::vector<std::vector<std::string>> parsedLines;
	std::vector<Document> documents;
	std::vector<std::vector<std::vector<std::string>>> externalSourcesLinksParams;
	inputLines.reserve(chunkSize);
	parsedLines.reserve(chunkSize);
	documents.reserve(chunkSize);
	externalSourcesLinksParams.reserve(chunkSize);

		while (true) {
			documents.clear();
			externalSourcesLinksParams.clear();

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
					if (pim->columns.size() != colData.size()) throw ExceptionLineParsingError("Incorrect number of columns");
					Document doc;
					Identifiers ids;
					std::vector<std::vector<std::string>> extSrcParams(extSrcName2inputOrder.size());

					for ( unsigned columnId: columnsIds ) {
						FileColumn const & colDef = pim->columns[columnId];
						std::string const & word = colData[columnId];
						if (word == "") {
							// ignore, error if it is a key column
							if (columnId == keyColumnId) {
								throw ExceptionLineParsingError("Key column cannot be empty");
							}
						} else if (colDef.colType == FileColumn::definitionWithReference) {
							// def with reference
							std::vector<std::string> vars;
							boost::split(vars, word, boost::is_any_of(",") );
							if ( vars.size() < 5 || (vars.size()-1) % 4 != 0 ) {
								throw ExceptionLineParsingError("Incorrect format of variant definition: " + word);
							}
							PlainVariant pv;
							pv.refId = referencesDb->getReferenceId(vars.front());
							for (unsigned i = 1; i < vars.size(); ++i) {
								PlainSequenceModification sm;
								unsigned const pos    = parseUInt32(vars[i],"Cannot parse position: " + vars[i]);
								++i;
								unsigned const length = parseUInt32(vars[i],"Cannot parse length: " + vars[i]);
								++i;
								sm.region.setLeftAndLength( pos, length );
								sm.newSequence = vars[i];   // TODO - check sequence
								++i;
								sm.originalSequence = vars[i];  // TODO - check sequence
								pv.modifications.push_back(sm);
							}
							std::sort(pv.modifications.begin(),pv.modifications.end());
							if (referencesDb->isProteinReference(pv.refId)) {
								setDefinition( asProteinVariant(doc).mainDefinition, canonicalizeProtein(referencesDb,pv) );
							} else {
								setDefinition( asGenomicVariant(doc).mainDefinition, canonicalizeGenomic(referencesDb,pv) );
							}
						} else if (colDef.colType == FileColumn::hgvs) {
							// hgvs
							HgvsVariant hgvsVar(true);
							decodeHgvs(referencesDb, word, hgvsVar);
							if (hgvsVar.isGenomic()) {
								setDefinition( asGenomicVariant(doc).mainDefinition, canonicalize(referencesDb, hgvsVar.genomic) );
							} else {
								setDefinition( asProteinVariant(doc).mainDefinition, canonicalize(referencesDb, hgvsVar.protein) );
							}
						} else if (colDef.colType == FileColumn::clinvarPreferredName) {
							// clinvar preferred name
							std::vector<IdentifierShort> v = ids.getShortIds(identifierType::ClinVarAllele);
							if (v.size() != 1) throw ExceptionLineParsingError("There is no ClinVarAllele identifier to bind given preferredName.");
							v[0].as_ClinVarAllele().preferredName = word;
							ids.add(v[0]);
						} else if (colDef.colType == FileColumn::clinvarRCVs) {
							// clinvar RCVs
							std::vector<IdentifierShort> v = ids.getShortIds(identifierType::ClinVarVariant);
							if (v.size() != 1) throw ExceptionLineParsingError("There is no ClinVarVariant identifier to bind given list of RCVs.");
							v[0].as_ClinVarVariant().RCVs = parseVectorOfUints(word);
							ids.add(v[0]);
						} else if (colDef.idType == identifierType::CA) {
							// identifiers - id
							if (columnId == keyColumnId) {
								// it is key column - it is used to determine allele type
								if (word.substr(0,2) == "CA") {
									asGenomicVariant(doc).caId = parseCA(word);
								} else if (word.substr(0,2) == "PA") {
									asProteinVariant(doc).caId = parsePA(word);
								} else {
									std::runtime_error("Incorrect Canonical/Protein Allele Id: '" + word + "'.");
								}
							} else {
								// it is not key column, must match current allele type
								if (doc.isActiveProteinVariant()) {
									doc.asActiveProteinVariant().caId = parsePA(word);
								} else if (doc.isActiveGenomicVariant()) {
									doc.asActiveGenomicVariant().caId = parseCA(word);
								}
							}
						} else if ( colDef.idType == identifierType::ClinVarAllele) {
							try {
								ids.add( Identifier_ClinVarAllele(parseShortId(colDef.idType,word)) );
							} catch (std::runtime_error const & e) {
								throw ExceptionLineParsingError(e.what());
							}
						} else if (colDef.idType == identifierType::ClinVarVariant) {
							try {
								ids.add( Identifier_ClinVarVariant(parseShortId(colDef.idType,word)) );
							} catch (std::runtime_error const & e) {
								throw ExceptionLineParsingError(e.what());
							}
						} else if (colDef.idType == identifierType::dbSNP) {
							try {
								if (! word.empty()) {
									std::vector<std::string> vs;
									boost::split(vs, word, boost::is_any_of(",") );
									for (std::string const & s: vs) ids.add( Identifier_dbSNP(parseShortId(colDef.idType,s)) );
								}
							} catch (std::runtime_error const & e) {
								throw ExceptionLineParsingError(e.what());
							}
						} else if (colDef.idType == identifierType::COSMIC) {
							try {
								if (word.size() < 7 || word.substr(0,3) != "COS" || word[word.size()-2] != '/') {
									throw ExceptionUnknownFormatOfCOSMICIdentifier(word);
								}
								bool active = false;
								bool coding = false;
								if (word[3] == 'M') {
									coding = true;
								} else if (word[3] != 'N') {
									throw ExceptionUnknownFormatOfCOSMICIdentifier(word);
								}
								if (word.back() == '1') {
									active = true;
								} else if (word.back() != '0') {
									throw ExceptionUnknownFormatOfCOSMICIdentifier(word);
								}
								uint32_t id = parseUInt32(word.substr(4,word.size()-6),"COSMIC id is incorrect");
								ids.add( Identifier_COSMIC(coding,id,active) );
							} catch (std::runtime_error const & e) {
								throw ExceptionLineParsingError(e.what());
							}
						} else if (colDef.idType == identifierType::MyVariantInfo_hg19 || colDef.idType == identifierType::MyVariantInfo_hg38) {
							ReferenceGenome const genome = (colDef.idType == identifierType::MyVariantInfo_hg19) ? (ReferenceGenome::rgGRCh37) : (ReferenceGenome::rgGRCh38);
							IdentifierWellDefined ident;
							NormalizedGenomicVariant varDef;
							parseIdentifierMyVariantInfo(referencesDb, genome, word, ident, varDef);
							if (logLogin == "admin") ids.add(ident);  // only admin can add identifiers; this check is required because this column may be used as a key by other uers
							setDefinition( asGenomicVariant(doc).mainDefinition, varDef );
						} else if (colDef.idType == identifierType::ExAC || colDef.idType == identifierType::gnomAD) {
							IdentifierWellDefined ident;
							NormalizedGenomicVariant varDef;
							parseIdentifierExACgnomAD(referencesDb, (colDef.idType == identifierType::ExAC), word, ident, varDef);
							if (logLogin == "admin") ids.add(ident);  // only admin can add identifiers; this check is required because this column may be used as a key by other uers
							setDefinition( asGenomicVariant(doc).mainDefinition, varDef );
						} else if (colDef.idType == identifierType::externalSource) {
							std::vector<std::string> vars;
							boost::split(vars, word, boost::is_any_of(" ") );
							extSrcParams[extSrcName2inputOrder[colDef.sourceName]] = vars;
						} else {
							throw ExceptionLineParsingError(toString(colDef.idType) + " identifiers are not implemented"); //TODO
						}
					}
					if (doc.isActiveGenomicVariant()) doc.asActiveGenomicVariant().identifiers = ids;
					if (doc.isActiveProteinVariant()) doc.asActiveProteinVariant().identifiers = ids;
					documents.push_back( doc );
					externalSourcesLinksParams.push_back( extSrcParams );
				} catch (...) {
					documents.push_back( DocumentError::createFromCurrentException() );
					externalSourcesLinksParams.resize( externalSourcesLinksParams.size() + 1 );
				}
			}

			// ====================== read variant definition from CA index if needed
			if ( pim->columns[keyColumnId].colType == FileColumn::identifier && pim->columns[keyColumnId].idType == identifierType::CA ) {
				std::cout << stopwatch.save_and_restart_sec() << " sec\n" << logPrefix << "fetch variants definitions by CA ID ... " << std::flush;
				allelesDb->fetchVariantsByCaPaIds(documents);
			}

			// ====================== map to main reference
			std::cout << stopwatch.save_and_restart_sec() << " sec\n" << logPrefix << "map variants to main genome ... " << std::flush;
			mapVariantsToMainGenome(documents);

			// ====================== fetch data
			if (killThread.load()) throw ExceptionRequestTerminated();
			std::cout << stopwatch.save_and_restart_sec() << " sec\n" << logPrefix << "fetch data ... " << std::flush;
			if (pim->registerIfNotFound) {
				allelesDb->fetchVariantsByDefinitionAndAddIdentifiers(documents);
			} else {
				allelesDb->fetchVariantsByDefinition(documents);
			}

			// ====================== load parameters to external sources
			std::cout << stopwatch.save_and_restart_sec() << " sec\n" << logPrefix << "update externalSources ... " << std::flush;
			for (auto const & kv: extSrcName2inputOrder) {
				std::string const & sourceName = kv.first;
				unsigned const extSrcColId = kv.second;
				// prepare parameters
				std::vector<uint32_t> ids;
				std::vector<std::vector<std::string>> params;
				ids.reserve(documents.size());
				params.reserve(documents.size());
				for ( unsigned i = 0;  i < documents.size();  ++i ) {
					CanonicalId caId;
					if (documents[i].isActiveGenomicVariant()) {
						caId = documents[i].asActiveGenomicVariant().caId;
					} else {
						caId = documents[i].asActiveProteinVariant().caId;
					}
					if ( ! caId.isNull() ) {
						ids.push_back(caId.value);
						params.push_back(externalSourcesLinksParams[i][extSrcColId]);
					}
				}
				// call procedure
				externalSources::registerLinks(ids, sourceName, params);
			}

			// ====================== calculate missing data
			std::cout << stopwatch.save_and_restart_sec() << " sec\n" << logPrefix << "fill variants details ... " << std::flush;
			fillVariantsDetails(documents);

			// ====================== fill inputLine for error objects
			for (unsigned i = 0; i < documents.size(); ++i) {
				if (documents[i].isError()) documents[i].error().fields[label::inputLine] = parser->lineByOffset(inputLines[i]);
			}

			// ====================== build response
			std::cout << stopwatch.save_and_restart_sec() << " sec\n" << logPrefix << "build & send response ... " << std::flush;
			addChunkOfResponse( documents );
			std::cout << stopwatch.save_and_restart_sec() << " sec" << std::endl;
	}
}
