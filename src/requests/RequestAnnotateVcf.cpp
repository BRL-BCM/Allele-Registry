#include "requests.hpp"
#include "parsers.hpp"
#include "canonicalization.hpp"
#include "../commonTools/Stopwatch.hpp"
#include <sstream>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>


struct RequestAnnotateVcf::Pim
{
	std::shared_ptr<std::vector<char>> body;
	std::vector<ReferenceId> refIdByChromosome;
	std::vector<identifierType> identifiers;
	bool registerUnknownVariants;
};


RequestAnnotateVcf::RequestAnnotateVcf(std::shared_ptr<std::vector<char>> pBody, std::string const & assembly, std::string const & pIds, bool registerNewVariants)
: Request(documentType::activeGenomicVariant), pim(new Pim)
{
	std::unique_ptr<Pim> pimGuard(pim);
	pim->body = pBody;
	pim->registerUnknownVariants = registerNewVariants;

	// ==== set reference genome and referenceIdentifiers for chromosomes
	ReferenceGenome refGenome;
	if (assembly == "NCBI36" || assembly == "hg18") refGenome = ReferenceGenome::rgNCBI36;
	else if (assembly == "GRCh37" || assembly == "hg19") refGenome = ReferenceGenome::rgGRCh37;
	else if (assembly == "GRCh38" || assembly == "hg38") refGenome = ReferenceGenome::rgGRCh38;
	else throw ExceptionIncorrectRequest("Incorrect value of assembly parameter, it must equal to one of the following: 'NCBI36', 'GRCh37', 'GRCh38', 'hg18', 'hg19', 'hg38'");

	pim->refIdByChromosome.resize( static_cast<unsigned>(Chromosome::chrM) + 1u );
	for ( unsigned i = static_cast<unsigned>(Chromosome::chr1);  i < static_cast<unsigned>(Chromosome::chrM);  ++i ) {
		pim->refIdByChromosome[i] = referencesDb->getReferenceId(refGenome, static_cast<Chromosome>(i));
	}
	// special case for mitochondrial DNA - TODO - solve it somehow
	if ( refGenome == ReferenceGenome::rgGRCh38 || assembly == "GRCh37" ) {
		pim->refIdByChromosome[static_cast<unsigned>(Chromosome::chrM)] = referencesDb->getReferenceId(ReferenceGenome::rgGRCh38, Chromosome::chrM);
	}

	// ==== parse identifiers
	std::vector<std::string> ids;
	boost::split(ids, pIds, boost::is_any_of("+") );
	for (auto const & c: ids) {
		if      (c == "CA"                   ) pim->identifiers.push_back(identifierType::CA);
		else if (c == "dbSNP.rs"             ) pim->identifiers.push_back(identifierType::dbSNP);
		else if (c == "ClinVar.alleleId"     ) pim->identifiers.push_back(identifierType::ClinVarAllele);
		else if (c == "ClinVar.variationId"  ) pim->identifiers.push_back(identifierType::ClinVarVariant);
		else if (c == "MyVariantInfo_hg19.id") pim->identifiers.push_back(identifierType::MyVariantInfo_hg19);
		else if (c == "MyVariantInfo_hg38.id") pim->identifiers.push_back(identifierType::MyVariantInfo_hg38);
		else if (c == "ExAC.id"              ) pim->identifiers.push_back(identifierType::ExAC);
		else if (c == "gnomAD.id"            ) pim->identifiers.push_back(identifierType::gnomAD);
		else if (c == "AllelicEpigenome.id"  ) pim->identifiers.push_back(identifierType::AllelicEpigenome);
		else if (c == "ClinVar.RCV"          ) pim->identifiers.push_back(identifierType::ClinVarRCV);
		else if (c == "COSMIC.id"            ) pim->identifiers.push_back(identifierType::COSMIC);
		else throw ExceptionIncorrectRequest("Unknown identifier name: '" + c + "'.");
	}

	// ==== success
	pimGuard.release();
}


RequestAnnotateVcf::~RequestAnnotateVcf()
{
	stopProcessingThread();
	delete pim;
}


void RequestAnnotateVcf::process()
{
	std::unique_ptr<ParserVcf2> parser( new ParserVcf2(pim->body,pim->refIdByChromosome) );

	unsigned const chunkSize = 1024*1024;
	std::vector<PlainVariant> variants;
	std::vector<Document> documents;
	documents.reserve(chunkSize);
	std::vector<std::vector<std::string>> outIds;
	outIds.reserve(chunkSize);

	while (true) {
		variants.clear();
		documents.clear();
		outIds.clear();

		// ---- logs
		std::string const logPrefix = boost::lexical_cast<std::string>(parser->numberOfParsedBytes()/(1024*1024)) + " MB -> ";
		Stopwatch stopwatch;
		std::cout << logPrefix << "parse payload ... " << std::flush;

		if ( ! parser->parseRecords(variants,chunkSize) ) {
			break; // no more data
		}

		// ====================== parse & convert to variants

		// ---- convert parsed variants to documents
		for (auto const & pv: variants) {
			try {
				if (pv.refId.isNull()) {
					documents.push_back(DocumentError(errorType::VcfParsingError));
				} else {
					DocumentActiveGenomicVariant doc;
					doc.mainDefinition = canonicalizeGenomic(referencesDb,pv);
					documents.push_back( doc );
				}
			} catch (...) {
				documents.push_back( DocumentError::createFromCurrentException() );
			}
		}

		// ====================== map to main reference
		std::cout << stopwatch.save_and_restart_sec() << " sec\n" << logPrefix << "map variants to main genome ... " << std::flush;
		mapVariantsToMainGenome(documents);

		// ====================== fetch data
		if (killThread.load()) throw ExceptionRequestTerminated();
		std::cout << stopwatch.save_and_restart_sec() << " sec\n" << logPrefix << "fetch data ... " << std::flush;
		if (pim->registerUnknownVariants) {
			allelesDb->fetchVariantsByDefinitionAndAddIdentifiers(documents);
		} else {
			allelesDb->fetchVariantsByDefinition(documents);
		}
		// ====================== convert identifiers to string
		for (auto const & doc: documents) {
			outIds.push_back(std::vector<std::string>());
			// omit empty
			if ( ! doc.isActiveGenomicVariant() ) {
				continue;
			}
			// check identifiers
			for (identifierType idt: pim->identifiers) {
				if (idt == identifierType::CA) {
					if ( ! doc.asActiveGenomicVariant().caId.isNull() ) {
						outIds.back().push_back( "CA" + toString(doc.asActiveGenomicVariant().caId.value) );
					}
				} else {
					std::vector<IdentifierShort> ids = doc.asActiveGenomicVariant().identifiers.getShortIds(idt);
					for (auto const & id: ids) {
						outIds.back().push_back( id.toString() );
					}
				}
			}
		}

		// ====================== build response
		std::cout << stopwatch.save_and_restart_sec() << " sec\n" << logPrefix << "build & send response ... " << std::flush;
		std::stringstream response;
		parser->buildVcfResponse(response, outIds);
		addChunkOfResponse( response.str() );
		std::cout << stopwatch.save_and_restart_sec() << " sec" << std::endl;
	}
	addChunkOfResponse(""); // to signal EOF - temporary solution
}
