#include "allelesDatabase.hpp"
#include <boost/test/minimal.hpp>
#include <memory>
#include <mutex>
#include <list>
#include <set>
#include <boost/lexical_cast.hpp>


std::string seq = "CTCTTGAGACTGACTGAAAAGACTCTCGACTTGCAGATCGCCCTGGGGATATAGCTCCCTGAGCTAGATCAGTCACTGAAAAGACTCTCGACTTGCAGATCGCCCTGGGGATATAGCTCCCT";


std::vector<std::vector<NormalizedGenomicVariant>> generateVariantDefinitions(unsigned totalVariantsCount, unsigned datasetsCount)
{
    std::vector<NormalizedGenomicVariant> defs(totalVariantsCount);
    unsigned i = 0;
    for (auto & def: defs) {
        ++i;
        def.refId.value = i % 22 + 1;
        if (i % 97 == 3) {
            def.modifications.resize(i % 5 + 1);
        } else {
            def.modifications.resize(1);
        }
        unsigned j = 0;
        for (auto & m: def.modifications) {
            m.category = static_cast<variantCategory>(i%4);
            j += 100;
            unsigned left = 100 + i % 10003 + j;  // TODO - larger position values
            unsigned length = 2 + i%43;
            m.region.setLeftAndLength(left,length);
            switch (m.category) {
                case variantCategory::nonShiftable:
                case variantCategory::shiftableInsertion:
                    m.insertedSequence = seq.substr(i % 29, 1 + i % 37);
                    break;
                case variantCategory::shiftableDeletion:
                    m.lengthChange = 1 + i % (length-1);
                    break;
                case variantCategory::duplication:
                    m.lengthChange = 1 + (i % 47) % length;
                    break;
            }
        }
    }
    std::vector<std::vector<NormalizedGenomicVariant>> out(datasetsCount);
    for (auto & v: out) v.reserve(totalVariantsCount / datasetsCount + 1);
    for (unsigned i = 0; i < totalVariantsCount; ++i) {
        out[i % out.size()].push_back(defs[i]);
    }
    return out;
}

std::vector<std::vector<NormalizedProteinVariant>> generateProteinVariantDefinitions(ReferencesDatabase const * refDb, unsigned totalVariantsCount, unsigned datasetsCount)
{
    ReferenceId const minProteinRefId = refDb->getReferenceId("NP_001299849.1");
    ASSERT(! minProteinRefId.isNull());
    std::vector<NormalizedProteinVariant> defs(totalVariantsCount);
    unsigned i = 0;
    for (auto & def: defs) {
        unsigned maxLength;
        do {
            ++i;
            def.refId = minProteinRefId;
            def.refId.value += i % 100000;
            maxLength = refDb->getMetadata(def.refId).length;
        } while (maxLength < 40);
        if (! refDb->isProteinReference(def.refId)) throw std::runtime_error("Not a protein for i=" + boost::lexical_cast<std::string>(i));
        if (i % 97 == 3 && maxLength > 100) {
            def.modifications.resize(i % 5 + 1);
        } else {
            def.modifications.resize(1);
        }
        unsigned j = 0;
        for (auto & m: def.modifications) {
            m.category = static_cast<variantCategory>(i%4);
            j += 10;
            unsigned left = 1 + (i%5) * (maxLength/10) + j;
            unsigned length = 2 + (i+j)%6;
            m.region.setLeftAndLength(left,length);
            switch (m.category) {
                case variantCategory::nonShiftable:
                case variantCategory::shiftableInsertion:
                    m.insertedSequence = seq.substr(i % 29, 1 + i % 7);
                    break;
                case variantCategory::shiftableDeletion:
                    m.lengthChange = 1 + i % (length-1);
                    break;
                case variantCategory::duplication:
                    m.lengthChange = 1 + (i % 47) % length;
                    break;
            }
        }
    }
    std::vector<std::vector<NormalizedProteinVariant>> out(datasetsCount);
    for (auto & v: out) v.reserve(totalVariantsCount / datasetsCount + 1);
    for (unsigned i = 0; i < totalVariantsCount; ++i) {
        out[i % out.size()].push_back(defs[i]);
    }
    return out;
}


std::vector<std::vector<Identifiers>> generateVariantIdentifiers(unsigned totalIdentifiersCount, unsigned datasetsCount)
{
    std::vector<Identifiers> defs(totalIdentifiersCount);
    unsigned i = 0;
    for (auto & ids: defs) {
        unsigned count = (i%17 + i%3) % 6;
        for ( ;  count;  --count ) {
            ++i;
            switch (i%3) {
            case 0: ids.add( Identifier_dbSNP((i/7) * 11) ); break;
            case 1: ids.add( Identifier_ClinVarAllele((i/7) * 11, "alaMaKota") ); break;
            case 2: ids.add( Identifier_ClinVarVariant((i/7) * 11) ); break;
            }
        }
    }
    std::vector<std::vector<Identifiers>> out(datasetsCount);
    for (auto & v: out) v.reserve(totalIdentifiersCount / datasetsCount + 1);
    for (unsigned i = 0; i < totalIdentifiersCount; ++i) {
        out[i % out.size()].push_back(defs[i]);
    }
    return out;
}


std::vector<Document> convertToDocuments(std::vector<NormalizedGenomicVariant> const & defs, std::vector<NormalizedProteinVariant> const & defs2)
{
    std::vector<Document> docs;
    for (auto & v: defs) {
        DocumentActiveGenomicVariant doc;
        doc.mainDefinition = v;
        docs.push_back(doc);
    }
    for (auto & v: defs2) {
        DocumentActiveProteinVariant doc;
        doc.mainDefinition = v;
        docs.push_back(doc);
    }
    return docs;
}

std::vector<Document> convertToDocuments(std::vector<NormalizedGenomicVariant> const & defs, std::vector<NormalizedProteinVariant> const & defs2, std::vector<Identifiers> const & ids )
{
    std::vector<Document> docs = convertToDocuments(defs,defs2);
    for (unsigned i = 0; i < defs.size(); ++i) {
        if (docs[i].isActiveGenomicVariant()) docs[i].asActiveGenomicVariant().identifiers = ids[i%(ids.size())];
        if (docs[i].isActiveProteinVariant()) docs[i].asActiveProteinVariant().identifiers = ids[i%(ids.size())];
    }
    return docs;
}

struct AllelesDatabaseTester
{
    ReferencesDatabase * refDb;
    AllelesDatabase * db;
    std::map<NormalizedGenomicVariant,std::pair<CanonicalId,Identifiers>> data;
    std::map<NormalizedProteinVariant,std::pair<CanonicalId,Identifiers>> data2;
    std::map<CanonicalId,NormalizedGenomicVariant> ca2def;
    std::map<CanonicalId,NormalizedProteinVariant> pa2def;
    // =============== fetch... methods - operate on a vector of documents (the size of the vector is not changed)
    // fetch variants definitions basing on CA Id (no identifiers/revisions are filled)
    void fetchVariantsByCaIds(std::vector<Document> &);
    // fetch full variants info (with identifiers and revision) by definitions with optional changes in identifiers
    void fetchVariantsByDefinition(std::vector<Document> &);
    void fetchVariantsByDefinitionAndAddIdentifiers(std::vector<Document> &);
    void fetchVariantsByDefinitionAndDeleteIdentifiers(std::vector<Document> &);
    // =============== query... methods - returns vector of matching documents basing on given criteria
    // query variants (with identifiers and revision) with given identifier
    void queryVariants(AllelesDatabase::callbackSendChunk, std::vector<identifierType> const & idType);
    // query variants (with identifiers and revision) by identifiers (the vector of identifiers must be small)
    void queryVariants(AllelesDatabase::callbackSendChunk, std::vector<std::pair<identifierType,uint32_t>> const & ids);
    // query variants from given regions (with identifiers and revision)
    void queryVariants(AllelesDatabase::callbackSendChunk, ReferenceId refId, uint32_t from = 0, uint32_t to = std::numeric_limits<uint32_t>::max());
    // query all variants (with identifiers and revision)
    void queryVariants(AllelesDatabase::callbackSendChunk);
    // -------------- constructor & destructor
    AllelesDatabaseTester(Configuration conf)
    {
        refDb = new ReferencesDatabase(conf.referencesDatabase_path);
        db = new AllelesDatabase(refDb,conf);
    }
    ~AllelesDatabaseTester()
    {
        delete db;
        delete refDb;
    }
};

template<typename Element>
inline void appendReverseSortAndUnique(std::vector<Element> & v1, std::vector<Element> const & v2)
{
    v1.insert( v1.end(), v2.begin(), v2.end() );
    std::sort( v1.begin(), v1.end() );
    auto newEnd = std::unique( v1.begin(), v1.end() );
    v1.erase( newEnd, v1.end() );
    std::reverse( v1.begin(), v1.end() );
}

void checkIfTheSame(std::vector<Document> const & docs1, std::vector<Document> const & docs2)
{
    if (docs1.size() != docs2.size()) throw std::logic_error("compare error: vectors have different sizes");
    for (unsigned i = 0; i < docs1.size(); ++i) {
        if (docs1[i] != docs2[i]) {
            std::cout << docs1[i].toString() << "\t" << docs2[i].toString() << std::endl;
            throw std::logic_error("compare error: different document at position " + boost::lexical_cast<std::string>(i));
        }
    }
}

void AllelesDatabaseTester::fetchVariantsByCaIds(std::vector<Document> & docs)
{
    std::cout << "fetchVariantsByCaIds" << std::endl;
    std::vector<Document> response(docs.size());
    for (unsigned i = 0; i < docs.size(); ++i) {
        response[i] = docs[i];
        if ( ! docs[i].isActiveGenomicVariant() ) continue;
        CanonicalId caId = docs[i].asActiveGenomicVariant().caId;
        auto iCa = ca2def.find(caId);
        if (iCa == ca2def.end()) {
            response[i] = DocumentError(errorType::NotFound);
            continue;
        }
        auto it = data.find(iCa->second);
        if (it == data.end()) {
            throw std::logic_error("No definition for CA ID: " + boost::lexical_cast<std::string>(iCa->first.value) );
        }
        response[i].asActiveGenomicVariant().mainDefinition = it->first;
        response[i].asActiveGenomicVariant().identifiers = it->second.second;
    }

    db->fetchVariantsByCaPaIds(docs);
    checkIfTheSame(response, docs);
}


void AllelesDatabaseTester::fetchVariantsByDefinition(std::vector<Document> & docs)
{
    std::cout << "fetchVariantsByDefinition" << std::endl;
    std::vector<Document> response(docs.size());
    for (unsigned i = 0; i < docs.size(); ++i) {
        response[i] = docs[i];
        if ( docs[i].isActiveGenomicVariant() ) {
            auto it = data.find(docs[i].asActiveGenomicVariant().mainDefinition);
            if (it == data.end()) {
                //response[i] = DocumentError(errorType::NotFound);
                response[i].asActiveGenomicVariant().caId = CanonicalId::null;
                response[i].asActiveGenomicVariant().identifiers.clear();
                continue;
            }
            response[i].asActiveGenomicVariant().caId        = it->second.first;
            response[i].asActiveGenomicVariant().identifiers = it->second.second;
        } else if ( docs[i].isActiveProteinVariant() ) {
            auto it = data2.find(docs[i].asActiveProteinVariant().mainDefinition);
            if (it == data2.end()) {
                //response[i] = DocumentError(errorType::NotFound);
                response[i].asActiveProteinVariant().caId = CanonicalId::null;
                response[i].asActiveProteinVariant().identifiers.clear();
                continue;
            }
            response[i].asActiveProteinVariant().caId        = it->second.first;
            response[i].asActiveProteinVariant().identifiers = it->second.second;
        }
    }

    db->fetchVariantsByDefinition(docs);
    checkIfTheSame(response, docs);
}


void AllelesDatabaseTester::fetchVariantsByDefinitionAndAddIdentifiers(std::vector<Document> & docs)
{
    std::cout << "fetchVariantsByDefinitionAndAddIdentifiers" << std::endl;
    // update data
    for (unsigned i = 0; i < docs.size(); ++i) {
        if ( docs[i].isActiveGenomicVariant() ) {
            auto it = data.find(docs[i].asActiveGenomicVariant().mainDefinition);
            if (it == data.end()) {
                data[docs[i].asActiveGenomicVariant().mainDefinition] = std::make_pair(docs[i].asActiveGenomicVariant().caId,docs[i].asActiveGenomicVariant().identifiers);
                continue;
            }
            // calculate new list of identifiers
            it->second.second.add( docs[i].asActiveGenomicVariant().identifiers );
        } else if ( docs[i].isActiveProteinVariant() ) {
            auto it = data2.find(docs[i].asActiveProteinVariant().mainDefinition);
            if (it == data2.end()) {
                data2[docs[i].asActiveProteinVariant().mainDefinition] = std::make_pair(docs[i].asActiveProteinVariant().caId,docs[i].asActiveProteinVariant().identifiers);
                continue;
            }
            // calculate new list of identifiers
            it->second.second.add( docs[i].asActiveProteinVariant().identifiers );
        }
    }

    // calculate response
    std::vector<Document> response(docs.size());
    for (unsigned i = 0; i < docs.size(); ++i) {
        response[i] = docs[i];
        if ( docs[i].isActiveGenomicVariant() ) {
            auto it = data.find(docs[i].asActiveGenomicVariant().mainDefinition);
            response[i].asActiveGenomicVariant().identifiers = it->second.second;
        } else if ( docs[i].isActiveProteinVariant() ) {
            auto it = data2.find(docs[i].asActiveProteinVariant().mainDefinition);
            response[i].asActiveProteinVariant().identifiers = it->second.second;
        }
    }


    db->fetchVariantsByDefinitionAndAddIdentifiers(docs);
    // save new CA ID
    for (unsigned i = 0; i < docs.size(); ++i) {
        if ( response[i].isActiveGenomicVariant() ) {
            if ( response[i].asActiveGenomicVariant().caId.isNull() ) {
                CanonicalId const caId = docs[i].asActiveGenomicVariant().caId;
                if ( caId.isNull() ) throw std::logic_error("Returned document does not have CA ID");
                response[i].asActiveGenomicVariant().caId = caId;
                data[docs[i].asActiveGenomicVariant().mainDefinition].first = caId;
                ca2def[caId] = docs[i].asActiveGenomicVariant().mainDefinition;
            }
        } else if ( response[i].isActiveProteinVariant() ) {
            if ( response[i].asActiveProteinVariant().caId.isNull() ) {
                CanonicalId const caId = docs[i].asActiveProteinVariant().caId;
                if ( caId.isNull() ) throw std::logic_error("Returned document does not have PA ID");
                response[i].asActiveProteinVariant().caId = caId;
                data2[docs[i].asActiveProteinVariant().mainDefinition].first = caId;
                pa2def[caId] = docs[i].asActiveProteinVariant().mainDefinition;
            }
        }
    }
    checkIfTheSame(response, docs);
}


void AllelesDatabaseTester::fetchVariantsByDefinitionAndDeleteIdentifiers(std::vector<Document> & docs)
{
    std::cout << "fetchVariantsByDefinitionAndDeleteIdentifiers" << std::endl;
    // update data
    for (unsigned i = 0; i < docs.size(); ++i) {
        if ( docs[i].isActiveGenomicVariant() ) {
            auto it = data.find(docs[i].asActiveGenomicVariant().mainDefinition);
            if (it == data.end()) continue;
            // calculate new list of identifiers
            it->second.second.remove( docs[i].asActiveGenomicVariant().identifiers );
        } else if ( docs[i].isActiveProteinVariant() ) {
            auto it = data2.find(docs[i].asActiveProteinVariant().mainDefinition);
            if (it == data2.end()) continue;
            // calculate new list of identifiers
            it->second.second.remove( docs[i].asActiveProteinVariant().identifiers );
        }
    }

    // calculate response
    std::vector<Document> response(docs.size());
    for (unsigned i = 0; i < response.size(); ++i) {
        response[i] = docs[i];
        if ( response[i].isActiveGenomicVariant() ) {
            auto it = data.find(response[i].asActiveGenomicVariant().mainDefinition);
            if (it == data.end()) {
                //response[i] = DocumentError(errorType::NotFound);
                response[i].asActiveGenomicVariant().caId = CanonicalId::null;
                response[i].asActiveGenomicVariant().identifiers.clear();
                continue;
            }
            response[i].asActiveGenomicVariant().caId        = it->second.first;
            response[i].asActiveGenomicVariant().identifiers = it->second.second;
        } else if ( response[i].isActiveProteinVariant() ) {
            auto it = data2.find(response[i].asActiveProteinVariant().mainDefinition);
            if (it == data2.end()) {
                //response[i] = DocumentError(errorType::NotFound);
                response[i].asActiveProteinVariant().caId = CanonicalId::null;
                response[i].asActiveProteinVariant().identifiers.clear();
                continue;
            }
            response[i].asActiveProteinVariant().caId        = it->second.first;
            response[i].asActiveProteinVariant().identifiers = it->second.second;
        }
    }

    db->fetchVariantsByDefinitionAndDeleteIdentifiers(docs);
    checkIfTheSame(response, docs);
}


AllelesDatabase::callbackSendChunk snifferForQueryCallback(std::list<Document> & respond, AllelesDatabase::callbackSendChunk callback, bool & lastCallReached)
{
    auto sniffer = [&respond,callback,&lastCallReached](std::vector<Document> & docs, bool & lastCall)
    {
        if (lastCallReached) throw std::logic_error("callback after last call");
        if (respond.size() < docs.size()) {
            auto itx = respond.begin();
            for (unsigned i = 0; i < docs.size(); ++i) {
                std::cout << ((itx != respond.end()) ? ((itx++)->toString()) : ("NONE")) << "\t";
                std::cout << ((i < docs   .size()) ? (docs   [i].toString()) : ("NONE")) << std::endl;
            }
            throw std::logic_error("query returned to many documents!");
        }
        std::vector<Document> r;
        r.reserve(docs.size());
        while (r.size() < docs.size()) {
            r.push_back(respond.front());
            respond.pop_front();
        }
        checkIfTheSame(r, docs);
        callback(docs, lastCall);
        if (lastCall) {
            lastCallReached = true;
            if (respond.size() > 10) {
                std::cout << "Missing: " << respond.size() << " documents" << std::endl;
            } else {
                for (auto d : respond) std::cout << "Missing\t" << d.toString() << std::endl;
            }
            if (! respond.empty()) throw std::logic_error("Not all documents were returned");
        }
    };
    return sniffer;
}

void AllelesDatabaseTester::queryVariants(AllelesDatabase::callbackSendChunk callback, std::vector<identifierType> const & idType)
{
    std::cout << "queryVariants(callback,idTypes)" << std::endl;
    std::list<Document> respond;
    for (auto const & kv: data) {
        bool notFound = true;
        for (identifierType t: idType) if (! kv.second.second.getShortIds(t).empty()) { notFound = false; break; }
        if (notFound) continue;
        DocumentActiveGenomicVariant doc;
        doc.mainDefinition = kv.first;
        doc.caId = kv.second.first;
        doc.identifiers = kv.second.second;
        respond.push_back(doc);
    }
    for (auto const & kv: data2) {
        bool notFound = true;
        for (identifierType t: idType) if (! kv.second.second.getShortIds(t).empty()) { notFound = false; break; }
        if (notFound) continue;
        DocumentActiveProteinVariant doc;
        doc.mainDefinition = kv.first;
        doc.caId = kv.second.first;
        doc.identifiers = kv.second.second;
        respond.push_back(doc);
    }

    bool lastCallReached = false;
    unsigned skip = 0;
    db->queryVariants( snifferForQueryCallback(respond,callback,lastCallReached), skip, idType );
    if (! lastCallReached) throw std::logic_error("Missing last call");
}


void AllelesDatabaseTester::queryVariants(AllelesDatabase::callbackSendChunk callback, std::vector<std::pair<identifierType,uint32_t>> const & ids)
{
    std::cout << "queryVariants(callback,ids)" << std::endl;
    if (ids.empty()) {
        queryVariants(callback);
        return;
    }
    std::set<CanonicalId> caIds, paIds;
    for (auto const & vi: ids) if (vi.first == identifierType::CA) caIds.insert(CanonicalId(vi.second));
    for (auto const & vi: ids) if (vi.first == identifierType::PA) paIds.insert(CanonicalId(vi.second));
    std::list<Document> respond;
    for (auto const & kv: data) {
        bool notFound = true;
        for (auto const & vi: ids) if (kv.second.second.has(vi.first,vi.second) || caIds.count(kv.second.first) ) {
            notFound = false;
            break;
        }
        if (notFound) continue;
        DocumentActiveGenomicVariant doc;
        doc.mainDefinition = kv.first;
        doc.caId = kv.second.first;
        doc.identifiers = kv.second.second;
        respond.push_back(doc);
    }
    for (auto const & kv: data2) {
        bool notFound = true;
        for (auto const & vi: ids) if (kv.second.second.has(vi.first,vi.second) || paIds.count(kv.second.first) ) {
            notFound = false;
            break;
        }
        if (notFound) continue;
        DocumentActiveProteinVariant doc;
        doc.mainDefinition = kv.first;
        doc.caId = kv.second.first;
        doc.identifiers = kv.second.second;
        respond.push_back(doc);
    }

    bool lastCallReached = false;
    db->queryVariants( snifferForQueryCallback(respond,callback,lastCallReached), ids );
    if (! lastCallReached) throw std::logic_error("Missing last call");
}


void AllelesDatabaseTester::queryVariants(AllelesDatabase::callbackSendChunk callback, ReferenceId refId, uint32_t from, uint32_t to)
{
    std::cout << "queryVariants(callback," << refId.value << "," << from << "," << to << ")" << std::endl;
    unsigned from2 = 0;
    if (from > 10000) from2 = from - 10000;
    std::list<Document> respond;
    NormalizedGenomicVariant varFrom;
    varFrom.refId = refId;
    varFrom.modifications.resize(1);
    varFrom.modifications[0].region.set(from2,from+1);
    NormalizedGenomicVariant varTo = varFrom;
    varTo.modifications[0].region.set(to+1, to+1);
    auto it = data.lower_bound(varFrom);
    for ( ;  it != data.end() && it->first < varTo;  ++it ) {
        if (it->first.modifications.back ().region.right() <= from) continue;
        if (it->first.modifications.front().region.left () >= to  ) continue;
        DocumentActiveGenomicVariant doc;
        doc.mainDefinition = it->first;
        doc.caId = it->second.first;
        doc.identifiers = it->second.second;
        respond.push_back(doc);
    }

    bool lastCallReached = false;
    unsigned skip = 0;
    db->queryVariants( snifferForQueryCallback(respond,callback,lastCallReached), skip, refId, from, to );
    if (! lastCallReached) throw std::logic_error("Missing last call");
}


void AllelesDatabaseTester::queryVariants(AllelesDatabase::callbackSendChunk callback)
{
std::map<unsigned,unsigned> stat;
    std::cout << "queryVariants(callback)" << std::endl;
    std::list<Document> respond;
    for (auto const & kv: data) {
        DocumentActiveGenomicVariant doc;
        doc.mainDefinition = kv.first;
        doc.caId = kv.second.first;
        doc.identifiers = kv.second.second;
        respond.push_back(doc);
    }
    for (auto const & kv: data2) {
        DocumentActiveProteinVariant doc;
        doc.mainDefinition = kv.first;
        doc.caId = kv.second.first;
        doc.identifiers = kv.second.second;
        respond.push_back(doc);
    }
    bool lastCallReached = false;
    unsigned skip = 0;
    db->queryVariants( snifferForQueryCallback(respond,callback,lastCallReached), skip );
    if (! lastCallReached) throw std::logic_error("Missing last call");
}


int test_main(int argc, char ** argv)
{
    Configuration configuration;
    configuration.allelesDatabase_path = "dbTest";
        configuration.allelesDatabase_threads = 6;
    configuration.referencesDatabase_path = "testRef";

    AllelesDatabaseTester db(configuration);

    std::vector<std::vector<NormalizedGenomicVariant>> defsGenomicDatasets = generateVariantDefinitions(4000000, 3);
    std::vector<std::vector<NormalizedProteinVariant>> defsProteinDatasets = generateProteinVariantDefinitions(db.refDb, 4000000, 3);
    std::vector<std::vector<Identifiers>> idsDatasets  = generateVariantIdentifiers(4000000, 3);

    for (unsigned i = 0; i < defsGenomicDatasets.size(); ++i) {
        std::cout << "Test for i=" << i << std::endl;
        // =================================== FETCHING variants by CA id
        std::vector<Document> docsCa(4000000);
        unsigned caId = 0;
        for (auto & d: docsCa) {
            d = DocumentActiveGenomicVariant();
            d.asActiveGenomicVariant().caId = CanonicalId(++caId);
        }
        db.fetchVariantsByCaIds(docsCa);
        // =================================== FETCHING VARIANTS BY DEFINITIONS (& add/remove identifiers)
        for (unsigned j = 0; j <= i; ++j) {
            std::cout << "j=" << j << " ";
            std::vector<Document> d1 = convertToDocuments(defsGenomicDatasets[j], defsProteinDatasets[j]);
            db.fetchVariantsByDefinition(d1);
            d1 = convertToDocuments(defsGenomicDatasets[j], defsProteinDatasets[j], idsDatasets[i]);
            db.fetchVariantsByDefinitionAndAddIdentifiers(d1);
            d1 = convertToDocuments(defsGenomicDatasets[j], defsProteinDatasets[j], idsDatasets[i]);
            db.fetchVariantsByDefinitionAndDeleteIdentifiers(d1);
        }
        // ===================================== QUERIES
        auto callback = [](std::vector<Document> &, bool lastCall) {};
        // test query returning all alleles
        db.queryVariants(callback);
        // test query by region
//TODO      db.queryVariants(callback, ReferenceId((i*4567)%22+1), (i*4567)%5000, (i*4567)%5000 + (i*4567)%3456);
        // test query by identifier types
        std::vector<identifierType> identTypes(1, identifierType::ClinVarAllele);
        db.queryVariants(callback, identTypes );
        identTypes[0] = identifierType::ClinVarVariant;
        identTypes.push_back( identifierType::dbSNP );
        db.queryVariants(callback, identTypes );
        // test query by identifiers
        std::vector<std::pair<identifierType,uint32_t>> vids;
        vids.push_back( std::make_pair(identifierType::CA            , (i*4567)%123456) );
        vids.push_back( std::make_pair(identifierType::ClinVarAllele , (i*4567)%123456) );
        vids.push_back( std::make_pair(identifierType::ClinVarVariant, (i*4567)%123456) );
        vids.push_back( std::make_pair(identifierType::dbSNP         , (i*4567)%123456) );
        db.queryVariants(callback, vids );
        // =====================================
        std::cout << std::endl;
    }

    return 0;
}
