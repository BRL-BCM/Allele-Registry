#include "requests.hpp"
#include "hgvs.hpp"
#include "canonicalization.hpp"
#include <boost/thread/thread.hpp>
#include <chrono>

using namespace std::chrono;

RequestFetchAlleleByHgvs::RequestFetchAlleleByHgvs( std::string const & pId, bool registerNewAllele, bool named )
: Request(documentType::activeGenomicVariant), fHgvs(pId), fRegisterNewAllele(registerNewAllele), fNamed(named)
{
	if(fRegisterNewAllele) {
		pid_t pid = getpid();
		milliseconds ms = duration_cast< milliseconds >( system_clock::now().time_since_epoch() );
		std::stringstream ss;
		ss << configuration.payloads_path << "/payload" << "-" << pid << "-" << boost::this_thread::get_id() << "-" << ms.count();
		payload.open(ss.str());
	std::cout << "PayloadFileName: " << ss.str() << std::endl;
	}
}


RequestFetchAlleleByHgvs::~RequestFetchAlleleByHgvs()
{
	stopProcessingThread();
	if(fRegisterNewAllele) {
		payload.close();
	}
}


void RequestFetchAlleleByHgvs::process()
{
	if(fRegisterNewAllele) {
		payload << "#\t" << logTime << "\t" << logLogin << "\t" << logMethod << "\t" << logRequest << "\n";
		payload << fHgvs << "\n";
	}
	HgvsVariant hgvsVar(true);
	decodeHgvs(referencesDb, fHgvs, hgvsVar);
	std::vector<Document> documents(1);
	if (hgvsVar.isGenomic()) {
		documents[0] = DocumentActiveGenomicVariant(canonicalize(referencesDb, hgvsVar.genomic));
		mapVariantsToMainGenome(documents);
	} else {
		documents[0] = DocumentActiveProteinVariant(canonicalize(referencesDb, hgvsVar.protein));
	}
	std::vector<bool> regNew;
	regNew.resize(documents.size());
	{
		allelesDb->fetchVariantsByDefinition(documents);
		for ( unsigned i = 0; i < documents.size() ; ++i ) {
			CanonicalId caId;
			if (documents[i].isActiveGenomicVariant()) {
				caId = documents[i].asActiveGenomicVariant().caId;
				regNew[i] = caId.isNull();
			} else if(documents[i].isActiveProteinVariant()) {
				caId = documents[i].asActiveProteinVariant().caId;
				regNew[i] = caId.isNull();
			} else {
				regNew[i] = false;
			}
		}
	}
	if (fRegisterNewAllele) {
		allelesDb->fetchVariantsByDefinitionAndAddIdentifiers(documents, fNamed);
		std::vector<std::pair<bool,uint64_t> > newIds;
		for ( unsigned i = 0; i < documents.size() ; ++i ) {
			CanonicalId caId;
			if (documents[i].isActiveGenomicVariant()) {
				caId = documents[i].asActiveGenomicVariant().caId;
				if(regNew[i] && !caId.isNull()) {
					newIds.push_back(std::make_pair(true, caId.value));
				}
			} else if(documents[i].isActiveProteinVariant()) {
				caId = documents[i].asActiveProteinVariant().caId;
				if(regNew[i] && !caId.isNull()) {
					newIds.push_back(std::make_pair(false, caId.value));
				}
			}
		}
		smConsumer->send(newIds);
	} else {
		allelesDb->fetchVariantsByDefinition(documents);
		documents = allelesDb->filterNamed(documents);
	}
	fillVariantsDetails(documents);

	if(fRegisterNewAllele) {
		// ====================== save CA/PA id to payload file, in same order it occurs in body
		for ( unsigned i = 0;  i < documents.size();  ++i ) {
			CanonicalId caId;
			if (documents[i].isActiveGenomicVariant()) {
				caId = documents[i].asActiveGenomicVariant().caId;
				if (!caId.isNull()) {
					payload << "# CA" << caId.value << "\n";
				} else {
					payload << "#\n";
				}
			} else if (documents[i].isActiveProteinVariant()) {
				caId = documents[i].asActiveProteinVariant().caId;
				if (!caId.isNull()) {
					payload << "# PA" << caId.value << "\n";
				} else {
					payload << "#\n";
				}
			} else {
				payload << "#\n";
			}
		}
	}

	setResponse(documents[0]);
}


std::string RequestFetchAlleleByHgvs::getRequestToGUI() const
{
	return ("/redmine/projects/registry/genboree_registry/allele?hgvs=" + fHgvs);
}

