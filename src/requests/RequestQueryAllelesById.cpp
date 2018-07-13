#include "requests.hpp"
#include "identifiersTools.hpp"


RequestQueryAllelesById::RequestQueryAllelesById( ResultSubset const & rs, std::vector<std::string> const & ids )
: Request(documentType::activeGenomicVariant), fRange(rs)
{
	for (auto const & id: ids) {
		// check if it is prefix + number
		unsigned i = 0;
		for ( ;  i < id.size();  ++i ) {
			if (id[i] >= 'A' && id[i] <= 'Z') continue;
			if (id[i] >= 'a' && id[i] <= 'z') continue;
			break;
		}
		if (i == id.size() || id[i] < '0' || id[i] > '9') continue;
		std::string prefix = id.substr(0,i);
		unsigned j = i+1;
		for ( ;  j < id.size();  ++j ) if (id[j] < '0' || id[j] > '9') break;
		if (j < id.size()) continue;
		// unsigned number = boost::lexical_cast<unsigned>(id.substr(i));
		if (prefix == "") {
			fIds.push_back( std::make_pair(identifierType::CA,id) );
			fIds.push_back( std::make_pair(identifierType::ClinVarAllele,id) );
			fIds.push_back( std::make_pair(identifierType::ClinVarVariant,id) );
			fIds.push_back( std::make_pair(identifierType::ClinVarRCV,id) );
			fIds.push_back( std::make_pair(identifierType::dbSNP,id) );
		} else if (prefix == "CA") {
			fIds.push_back( std::make_pair(identifierType::CA,id.substr(2)) );
		} else if (prefix == "rs") {
			fIds.push_back( std::make_pair(identifierType::dbSNP,id.substr(2)) );
		} else if (prefix == "RCV") {
			fIds.push_back( std::make_pair(identifierType::ClinVarRCV,id.substr(3)) );
		}
	}
}


RequestQueryAllelesById::~RequestQueryAllelesById()
{
	stopProcessingThread();
}


inline bool isUInt(std::string const & s)
{
	for (char c: s) if (c < '0' || c > '9') return false;
	return true;
}


void RequestQueryAllelesById::process()
{
	std::vector<std::pair<identifierType,uint32_t>> vIdentVals;
	std::map<NormalizedGenomicVariant,std::vector<identifierType>> vGenomicDefs;

	for (auto const & p: fIds) {
		std::string s = p.second;
		switch (p.first) {
			case identifierType::CA:
				if (s.substr(0,2) == "CA") s = s.substr(2);
				if (isUInt(s)) {
					vIdentVals.push_back( std::make_pair(identifierType::CA,boost::lexical_cast<unsigned>(s)) );
				}
				break;
			case identifierType::PA:
				if (s.substr(0,2) == "PA") s = s.substr(2);
				if (isUInt(s)) {
					vIdentVals.push_back( std::make_pair(identifierType::PA,boost::lexical_cast<unsigned>(s)) );
				}
				break;
			case identifierType::ClinVarAllele:
				if (isUInt(s)) {
					vIdentVals.push_back( std::make_pair(identifierType::ClinVarAllele,boost::lexical_cast<unsigned>(s)) );
				}
				break;
			case identifierType::ClinVarVariant:
				if (isUInt(s)) {
					vIdentVals.push_back( std::make_pair(identifierType::ClinVarVariant,boost::lexical_cast<unsigned>(s)) );
				}
				break;
			case identifierType::ClinVarRCV:
				if (s.substr(0,3) == "RCV") s = s.substr(3);
				if (isUInt(s)) {
					vIdentVals.push_back( std::make_pair(identifierType::ClinVarRCV,boost::lexical_cast<unsigned>(s)) );
				}
				break;
			case identifierType::dbSNP:
				if (s.substr(0,2) == "rs") s = s.substr(2);
				if (isUInt(s)) {
					vIdentVals.push_back( std::make_pair(identifierType::dbSNP,boost::lexical_cast<unsigned>(s)) );
				}
				break;
			case identifierType::MyVariantInfo_hg19:
			case identifierType::MyVariantInfo_hg38:
				{
					ReferenceGenome const genome = (p.first == identifierType::MyVariantInfo_hg19) ? (ReferenceGenome::rgGRCh37) : (ReferenceGenome::rgGRCh38);
					IdentifierWellDefined ident;
					NormalizedGenomicVariant varDef;
					parseIdentifierMyVariantInfo(referencesDb, genome, p.second, ident, varDef);
					vGenomicDefs[varDef].push_back(p.first);
				}
				break;
			case identifierType::ExAC:
			case identifierType::gnomAD:
				{
					IdentifierWellDefined ident;
					NormalizedGenomicVariant varDef;
					parseIdentifierExACgnomAD(referencesDb, (p.first == identifierType::ExAC), p.second, ident, varDef);
					vGenomicDefs[varDef].push_back(p.first);
				}
				break;
		}
	}

	if (! vGenomicDefs.empty()) {
		// for definition-like identifiers
		std::vector<Document> docsOrg;
		for (auto gd: vGenomicDefs) docsOrg.push_back(DocumentActiveGenomicVariant(gd.first));
		std::vector<Document> docs = docsOrg;
		mapVariantsToMainGenome(docs);
		allelesDb->fetchVariantsByDefinition(docs);
		// -- filter variants without required identifiers
		std::vector<Document> docs2;
		for (unsigned i = 0; i < docs.size(); ++i) {
			Document d = docs[i];
			if (! d.isActiveGenomicVariant()) continue;
			bool hasIdent = false;
			for (auto idType: vGenomicDefs[docsOrg[i].asActiveGenomicVariant().mainDefinition]) {
				if ( ! d.asActiveGenomicVariant().identifiers.getHgvsIds(idType).empty() ) {
					hasIdent = true;
					break;
				}
			}
			if (hasIdent) docs2.push_back(d);
		}
		// -- apply skip/limit parameters
		if (fRange.skip < docs2.size()) {
			docs2.erase(docs2.begin(), docs2.begin() + fRange.skip);
			if (docs2.size() > fRange.limit) docs2.resize(fRange.limit);
			fillVariantsDetails(docs2);
			addChunkOfResponse(docs2);
		}
		return;
	}

	// for short identifiers
	auto callback = [this](std::vector<Document> & docs, bool & lastCall)
	{
		if (fRange.skip >= docs.size()) {
			fRange.skip -= docs.size();
			return;
		}
		if (fRange.skip > 0) {
			docs.erase( docs.begin(), docs.begin() + fRange.skip );
			fRange.skip = 0;
		}
		if (fRange.limit <= docs.size()) {
			docs.erase( docs.begin() + fRange.limit, docs.end() );
			fRange.limit = 0;
			lastCall = true;
		} else {
			fRange.limit -= docs.size();
		}
		fillVariantsDetails(docs);
		addChunkOfResponse(docs);
	};

	allelesDb->queryVariants(callback, vIdentVals);
}
