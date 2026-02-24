#include "dispatcher.hpp"
#include "requests/requests.hpp"
#include "mysql/mysqlConnection.hpp"
#include "authorization.hpp"
#include <map>
#include <functional>
#include <chrono>
#include <boost/lexical_cast.hpp>

//std::string toString(std::map<std::string,std::string> const & params)
//{
//	std::string s = "";
//	for (auto const & kv: params) {
//		s += " ";
//		s += kv.first + "=" + kv.second;
//	}
//	return "{" + s + "}";
//}

HTTPP::HTTP::HttpCode errorType2httpStatus( errorType errType )
{
	switch (errType) {
		case NoErrors:
			return HTTPP::HTTP::HttpCode::Ok;
		case NotFound:
			return HTTPP::HTTP::HttpCode::NotFound;
		case AuthorizationError:
		case DisabledError:
			return HTTPP::HTTP::HttpCode::Forbidden;
		case HgvsParsingError:
		case IncorrectHgvsPosition:
		case IncorrectReferenceAllele:
		case NoConsistentAlignment:
		case UnknownCDS:
		case UnknownGene:
		case UnknownReferenceSequence:
		case VcfParsingError:
		case RequestTooLarge:
		case VariationTooLong:
		case CoordinateOutsideReference:
		case ComplexAlleleWithOverlappingSimpleAlleles:
		case IncorrectRequest:
			return HTTPP::HTTP::HttpCode::BadRequest;
		case InternalServerError:
		default:
			break;
	}
	return HTTPP::HTTP::HttpCode::InternalServerError;
}

enum requestProperty {
	  none
	, hasBody
	, supportPagination
};

class Params {
public:
	class Param {
	public:
		std::string fName;
		std::string fValue;
		Param(std::string const & name, std::string const & value = "") : fName(name), fValue(value) {}
		std::string asStr() { return fValue; }
		unsigned asUInt(std::string const & optionalPrefix = "")
		{
			std::string t = fValue;
			if (fValue.substr(0,optionalPrefix.size()) == optionalPrefix) t = fValue.substr(optionalPrefix.size());
			if (t.size() == 0 || t.size() > 10)  throw std::runtime_error("Parameter's value must be unsigned integer, containing not more than 10 digits.");
			for (char c: t) if (c < '0' || c > '10') throw std::runtime_error("Parameter's value must be unsigned integer, containing not more than 10 digits.");
			return boost::lexical_cast<unsigned>(t);
		}
		bool asBool(std::string const & valTrue, std::string const & valFalse)
		{
			if (fValue == valTrue ) return true;
			if (fValue == valFalse) return false;
			throw std::runtime_error("Parameter must be equal to one of the following values: " + valTrue + ", " + valFalse);
		}
		std::shared_ptr<std::vector<char>> asBody()
		{
			std::shared_ptr<std::vector<char>> data(new std::vector<char>());
			data->insert( data->end(), fValue.begin(), fValue.end() );
			return data;
		}
		bool operator<(Param const & p) const
		{
			return (fName < p.fName);
		}
	};
	std::vector<Param> params;
	std::shared_ptr<std::vector<char>> fPayload;
	ResultSubset fRange;
	Param & operator[](unsigned i) { return params.at(i); }
	void setPayload(std::shared_ptr<std::vector<char>> b) { fPayload = b; }
	std::shared_ptr<std::vector<char>> body() { return fPayload; }
	ResultSubset const & range() const { return fRange; }
	bool operator<(Params const & p) const
	{
		return (params < p.params);
	}
};

class ReqType
{
private:
	std::vector<std::string> fPath;
	Params fParams;
	bool fPagination;
	bool fBody;
public:
	ReqType(std::string const & path1, requestProperty prop = requestProperty::none)
	{
		ASSERT(! path1.empty());
		fPath.push_back(path1);
		fPagination = (prop == requestProperty::supportPagination);
		fBody = (prop == requestProperty::hasBody);
		if (path1.front() == '{' && path1.back() == '}') {
			std::string const name = path1.substr(1,path1.size()-2);
			fParams.params.push_back(Params::Param(name));
		}
	}
	ReqType(std::string const & path1, std::string const & path2, requestProperty prop = requestProperty::none)
	{
		ASSERT(! path1.empty());
		ASSERT(! path2.empty());
		fPath.push_back(path1);
		fPath.push_back(path2);
		fPagination = (prop == requestProperty::supportPagination);
		fBody = (prop == requestProperty::hasBody);
		if (path1.front() == '{' && path1.back() == '}') {
			std::string const name = path1.substr(1,path1.size()-2);
			fParams.params.push_back(Params::Param(name));
		}
		if (path2.front() == '{' && path2.back() == '}') {
			std::string const name = path2.substr(1,path2.size()-2);
			fParams.params.push_back(Params::Param(name));
		}
	}
	ReqType(std::string const & path1, std::string const & path2, std::string const & path3, requestProperty prop = requestProperty::none)
	{
		ASSERT(! path1.empty());
		ASSERT(! path2.empty());
		ASSERT(! path3.empty());
		fPath.push_back(path1);
		fPath.push_back(path2);
		fPath.push_back(path3);
		fPagination = (prop == requestProperty::supportPagination);
		fBody = (prop == requestProperty::hasBody);
		if (path1.front() == '{' && path1.back() == '}') {
			std::string const name = path1.substr(1,path1.size()-2);
			fParams.params.push_back(Params::Param(name));
		}
		if (path2.front() == '{' && path2.back() == '}') {
			std::string const name = path2.substr(1,path2.size()-2);
			fParams.params.push_back(Params::Param(name));
		}
		if (path3.front() == '{' && path3.back() == '}') {
			std::string const name = path3.substr(1,path3.size()-2);
			fParams.params.push_back(Params::Param(name));
		}
	}
	ReqType(std::string const & path1, std::string const & path2, std::string const & path3, std::string const & path4, requestProperty prop = requestProperty::none)
	{
		ASSERT(! path1.empty());
		ASSERT(! path2.empty());
		ASSERT(! path3.empty());
		ASSERT(! path4.empty());
		fPath.push_back(path1);
		fPath.push_back(path2);
		fPath.push_back(path3);
		fPath.push_back(path4);
		fPagination = (prop == requestProperty::supportPagination);
		fBody = (prop == requestProperty::hasBody);
		if (path1.front() == '{' && path1.back() == '}') {
			std::string const name = path1.substr(1,path1.size()-2);
			fParams.params.push_back(Params::Param(name));
		}
		if (path2.front() == '{' && path2.back() == '}') {
			std::string const name = path2.substr(1,path2.size()-2);
			fParams.params.push_back(Params::Param(name));
		}
		if (path3.front() == '{' && path3.back() == '}') {
			std::string const name = path3.substr(1,path3.size()-2);
			fParams.params.push_back(Params::Param(name));
		}
		if (path4.front() == '{' && path4.back() == '}') {
			std::string const name = path4.substr(1,path4.size()-2);
			fParams.params.push_back(Params::Param(name));
		}
	}
	ReqType & param(std::string const & name)
	{
		fParams.params.push_back(Params::Param(name));
		return *this;
	}
	bool matchPath(std::vector<std::string> const & path) const
	{
		if (path.size() != fPath.size()) return false;
		for (unsigned i = 0; i < path.size(); ++i) {
			if (fPath[i].front() == '{' && fPath[i].back() == '}') {
				// parameter
			} else {
				if (fPath[i] != path[i]) return false;
			}
		}
		return true;
	}
	bool requiresPayload() const { return fBody; }
	bool is_non_negative_number(std::string s) const {
		return !s.empty() && std::all_of(s.begin(), s.end(), ::isdigit);
	}
	bool matchParams(std::vector<std::string> const & path, std::map<std::string,std::string> const & inputParams, Params & matchedParams) const
	{
		matchedParams = fParams;

		// parameters from path
		unsigned paramsCount = 0;
		for (unsigned i = 0; i < path.size(); ++i) {
			if (fPath[i].front() == '{' && fPath[i].back() == '}') {
				matchedParams[paramsCount].fValue = path[i];
				++paramsCount;
			}
		}

		// standard parameters
		for (auto const & kv: inputParams) {
			if (kv.first == "skip") {
				if (fPagination) {
					if(is_non_negative_number(kv.second) == false) {
						throw ExceptionIncorrectRequest("Please provide non-negative skip parameter.");
					}
					matchedParams.fRange.skip = boost::lexical_cast<unsigned>(kv.second);
					continue;
				} else {
					return false;
				}
			}
			if (kv.first == "limit") {
				if (fPagination) {
					if(is_non_negative_number(kv.second) == false) {
						throw ExceptionIncorrectRequest("Please provide non-negative limit parameter.");
					}
					matchedParams.fRange.limit = boost::lexical_cast<unsigned>(kv.second);
					continue;
				} else {
					return false;
				}
			}
			bool found = false;
			for (auto & p: matchedParams.params) {
				if (p.fName == kv.first) {
					p.fValue = kv.second;
					found = true;
					break;
				}
			}
			if (found) {
				++paramsCount;
			} else {
				return false;
			}
		}
		if (paramsCount < matchedParams.params.size()) return false;
		return true;
	}
	bool operator<(ReqType const & r) const
	{
		if (fPath != r.fPath) return (fPath < r.fPath);
		return (fParams < r.fParams);
	}
};
RELATIONAL_OPERATORS(ReqType);




struct Dispatcher::Pim
{
	std::mutex synch;
	std::atomic<int> memCount;
	std::map<uint64_t, Request*> timingMap;
	std::map<ReqType,std::function<Request*(Params &)>> get, post, put, delete_;
	Configuration const & conf;
	Pim(Configuration const & pConf) : conf(pConf) {memCount = 0;}
};

Dispatcher::Dispatcher(Configuration const & conf) : pim(new Pim(conf))
{
	Request::initGlobalVariables(conf);

	// bump id
	pim->put[ReqType("bumpId").param("hgvs")] = [](Params & p)->Request*{ return new RequestBumpId(p[0].asStr()); };
	pim->put[ReqType("bumpIds",hasBody)] = [](Params & p)->Request*{ return new RequestBumpIds(p.body()); };
	pim->put[ReqType("setIds",hasBody)] = [](Params & p)->Request*{ return new RequestSetIds(p.body()); };

	// gene intersection
	pim->get[ReqType("geneIntersection").param("coord")] = [](Params & p)->Request*{ return new RequestGeneIntersection(p[0].asStr()); };

	// single objects
	pim->get[ReqType("allele", "{CAid}")]     = [](Params & p)->Request*{ return new RequestFetchAlleleById  (p[0].asStr()); };
	pim->get[ReqType("allele").param("hgvs")] = [](Params & p)->Request*{ return new RequestFetchAlleleByHgvs(p[0].asStr(),false); };
	pim->put[ReqType("allele").param("hgvs")] = [](Params & p)->Request*{ return new RequestFetchAlleleByHgvs(p[0].asStr(),true ); };
	pim->put[ReqType("reserve").param("hgvs")] = [](Params & p)->Request*{ return new RequestFetchAlleleByHgvs(p[0].asStr(),true,true ); };
	pim->get[ReqType("refseq", "{RSid}")]     = [](Params & p)->Request*{ return new RequestFetchReference   (p[0].asStr()); };
	pim->get[ReqType("refseq").param("id")]   = [](Params & p)->Request*{ return new RequestFetchReference   (p[0].asStr()); };
	pim->get[ReqType("refseq").param("id").param("refGenome")]   = [](Params & p)->Request*{ return new RequestFetchReference   (p[0].asStr(), p[1].asStr()); };
	pim->get[ReqType("gene"  , "{GNid}")]            = [](Params & p)->Request*{ return new RequestFetchGene(p[0].asStr()); };
	pim->get[ReqType("gene"  ).param("HGNC.symbol")] = [](Params & p)->Request*{ return new RequestFetchGene(p[0].asStr(), true); };
	// Dispatcher for coordinate transformation
	pim->get[ReqType("coordinateTransform").param("assembly").param("chr").param("start").param("end")] = [](Params & p)->Request*{ return new RequestCoordinateTransformation(p.range(),p[0].asStr(),p[1].asStr(),p[2].asUInt(),p[3].asUInt()); };	
	pim->get[ReqType("coordinateTransform").param("glDesc")] = [](Params & p)->Request*{ return new RequestCoordinateTransformation(p.range(),p[0].asStr()); };	

	// bulk operations
	pim->post[ReqType("alleles",hasBody).param("file")] = [](Params & p){ return new RequestFetchAllelesByDefinition(p.body(),p[0].asStr(),false); };
	pim->put [ReqType("alleles",hasBody).param("file")] = [](Params & p){ return new RequestFetchAllelesByDefinition(p.body(),p[0].asStr(),true); };
	pim->put [ReqType("reserve",hasBody).param("file")] = [](Params & p){ return new RequestFetchAllelesByDefinition(p.body(),p[0].asStr(),true,true); };

	// bulk coordinate transformation
	pim->post[ReqType("coordinateTransform",hasBody).param("file")] = [](Params & p){ return new RequestCoordinateTransformations(p.body(),p[0].asStr()); };
	pim->post[ReqType("coordinateTransforms",hasBody).param("file")] = [](Params & p){ return new RequestCoordinateTransformations(p.body(),p[0].asStr()); };

	// queries - alleles
	pim->get[ReqType("alleles",supportPagination).param("name")] = [](Params & p){ return new RequestQueryAllelesById(p.range(),{p[0].asStr()}); };
	pim->get[ReqType("alleles",supportPagination).param("gene")] = [](Params & p){ return new RequestQueryAllelesByGene(p.range(),p[0].asStr(),1024); };
	pim->get[ReqType("alleles",supportPagination).param("gene").param("batch")] = [](Params & p){ return new RequestQueryAllelesByGene(p.range(),p[0].asStr(),p[1].asUInt()); };
	pim->get[ReqType("alleles",supportPagination).param("ClinVar.variationId"  )] = [](Params & p){ return new RequestQueryAllelesById(p.range(),{std::make_pair(identifierType::ClinVarVariant    ,p[0].asStr())}); };
	pim->get[ReqType("alleles",supportPagination).param("ClinVar.alleleId"     )] = [](Params & p){ return new RequestQueryAllelesById(p.range(),{std::make_pair(identifierType::ClinVarAllele     ,p[0].asStr())}); };
	pim->get[ReqType("alleles",supportPagination).param("ClinVar.RCV"          )] = [](Params & p){ return new RequestQueryAllelesById(p.range(),{std::make_pair(identifierType::ClinVarRCV        ,p[0].asStr())}); };
	pim->get[ReqType("alleles",supportPagination).param("dbSNP.rs"             )] = [](Params & p){ return new RequestQueryAllelesById(p.range(),{std::make_pair(identifierType::dbSNP             ,p[0].asStr())}); };
	pim->get[ReqType("alleles",supportPagination).param("MyVariantInfo_hg19.id")] = [](Params & p){ return new RequestQueryAllelesById(p.range(),{std::make_pair(identifierType::MyVariantInfo_hg19,p[0].asStr())}); };
	pim->get[ReqType("alleles",supportPagination).param("MyVariantInfo_hg38.id")] = [](Params & p){ return new RequestQueryAllelesById(p.range(),{std::make_pair(identifierType::MyVariantInfo_hg38,p[0].asStr())}); };
	pim->get[ReqType("alleles",supportPagination).param("ExAC.id"              )] = [](Params & p){ return new RequestQueryAllelesById(p.range(),{std::make_pair(identifierType::ExAC              ,p[0].asStr())}); };
	pim->get[ReqType("alleles",supportPagination).param("gnomAD_2.id"            )] = [](Params & p){ return new RequestQueryAllelesById(p.range(),{std::make_pair(identifierType::gnomAD_2            ,p[0].asStr())}); };
	pim->get[ReqType("alleles",supportPagination).param("gnomAD_3.id"            )] = [](Params & p){ return new RequestQueryAllelesById(p.range(),{std::make_pair(identifierType::gnomAD_3            ,p[0].asStr())}); };
	pim->get[ReqType("alleles",supportPagination).param("gnomAD_4.id"            )] = [](Params & p){ return new RequestQueryAllelesById(p.range(),{std::make_pair(identifierType::gnomAD_4            ,p[0].asStr())}); };
	pim->get[ReqType("allelesIntersection",supportPagination).param("hgvs")] = [](Params & p){ return new RequestQueryAlleleIntersection(p.range(),p[0].asStr()); };
	pim->get[ReqType("allelesIntersection",supportPagination).param("hgvs").param("alignment")] = [](Params & p){ return new RequestQueryAlleleIntersection(p.range(),p[0].asStr(),p[1].asStr()); };
	pim->get[ReqType("alleles",supportPagination).param("refseq")] = [](Params & p){ return new RequestQueryAllelesByRegion(p.range(),p[0].asStr()); };
	pim->get[ReqType("alleles",supportPagination).param("refseq").param("refGenome").param("begin").param("end").param("alignment")] = [](Params & p){ return new RequestQueryAllelesByRegion(p.range(),p[0].asStr(),p[1].asStr(),p[2].asUInt(),p[3].asUInt(),p[4].asStr()); };
	pim->get[ReqType("alleles",supportPagination).param("refseq").param("begin").param("end").param("alignment")] = [](Params & p){ return new RequestQueryAllelesByRegion(p.range(),p[0].asStr(),"GRCh38",p[1].asUInt(),p[2].asUInt(),p[3].asStr()); };
	pim->get[ReqType("alleles",supportPagination).param("refseq").param("refGenome").param("begin").param("end")] = [](Params & p){ return new RequestQueryAllelesByRegion(p.range(),p[0].asStr(),p[1].asStr(),p[2].asUInt(),p[3].asUInt()); };
	pim->get[ReqType("alleles",supportPagination).param("refseq").param("begin").param("end")] = [](Params & p){ return new RequestQueryAllelesByRegion(p.range(),p[0].asStr(),"GRCh38",p[1].asUInt(),p[2].asUInt()); };

	// queries - genes
	pim->get[ReqType("genes",supportPagination).param("name")] = [](Params & p)->Request*{ return new RequestQueryGenes(p.range(),p[0].asStr()); };
	pim->get[ReqType("genes",supportPagination)              ] = [](Params & p)->Request*{ return new RequestQueryGenes(p.range()); };

	// queries - reference sequences
	pim->get[ReqType("refseqs",supportPagination).param("name")] = [](Params & p)->Request*{ return new RequestQueryReferences(p.range(),p[0].asStr()); };
	pim->get[ReqType("refseqs",supportPagination).param("gene")] = [](Params & p)->Request*{ return new RequestQueryReferences(p.range(),p[0].asStr(),true); };
	pim->get[ReqType("refseqs",supportPagination).param("referenceGenome")]; // TODO

	// annotate VCF files
	pim->post[ReqType("annotateVcf",hasBody).param("assembly").param("ids")] = [](Params & p){ return new RequestAnnotateVcf(p.body(),p[0].asStr(),p[1].asStr(),false); };
	pim->put [ReqType("annotateVcf",hasBody).param("assembly").param("ids")] = [](Params & p){ return new RequestAnnotateVcf(p.body(),p[0].asStr(),p[1].asStr(),true ); };

	// admin tools
	pim->delete_[ReqType("alleles",hasBody).param("file")] = [](Params & p){ return new RequestDeleteAlleles(p.body(),p[0].asStr()); };
	pim->delete_[ReqType("externalRecords", "{name}")] = [](Params & p)->Request*{ return new RequestDeleteIdentifiers(p[0].asStr()); };
	pim->delete_[ReqType("externalRecords", "{name}").param("from").param("to")] = [](Params & p)->Request*{ return new RequestDeleteIdentifiers(p[0].asStr(), p[1].asUInt(), p[2].asUInt()); };

	// not documented
	pim->get[ReqType("genomicAlleles",supportPagination)] = [](Params & p){ return new RequestQueryAlleles(p.range(),false); };
	pim->get[ReqType("proteinAlleles",supportPagination)] = [](Params & p){ return new RequestQueryAlleles(p.range(),true ); };
	pim->get[ReqType("refseq", "{RSid}", "sequence")] = [](Params & p)->Request*{ return new RequestFetchReference(p[0].asStr(),"",true); };
	pim->get[ReqType("matchAlleles").param("gene").param("variant")] = [](Params & p){ return new RequestQueryAllelesByGeneAndMutation(p[0].asStr(),p[1].asStr()); };

	// external sources
	pim->get    [ReqType("externalSource", "{name}")] = [](Params & p)->Request*{ return new RequestQueryExternalSources(p[0].asStr()); };
	pim->get    [ReqType("externalSources")] = [](Params & p)->Request*{ return new RequestQueryExternalSources(); };
	pim->put    [ReqType("externalSource", "{name}").param("url").param("params").param("guiName").param("guiLabel").param("guiUrl")] = [](Params & p)->Request*{ return new RequestCreateExternalSource(p[0].asStr(),p[1].asStr(),p[2].asStr(),p[3].asStr(),p[4].asStr(),p[5].asStr()); };
	pim->put    [ReqType("externalSource", "{name}").param("url")     ] = [](Params & p)->Request*{ return new RequestModifyExternalSource(p[0].asStr(),1,p[1].asStr()); };
	pim->put    [ReqType("externalSource", "{name}").param("guiName") ] = [](Params & p)->Request*{ return new RequestModifyExternalSource(p[0].asStr(),2,p[1].asStr()); };
	pim->put    [ReqType("externalSource", "{name}").param("guiLabel")] = [](Params & p)->Request*{ return new RequestModifyExternalSource(p[0].asStr(),3,p[1].asStr()); };
	pim->put    [ReqType("externalSource", "{name}").param("guiUrl")  ] = [](Params & p)->Request*{ return new RequestModifyExternalSource(p[0].asStr(),4,p[1].asStr()); };
	pim->delete_[ReqType("externalSource", "{name}")] = [](Params & p)->Request*{ return new RequestDeleteExternalSources(p[0].asStr()); };
	pim->put    [ReqType("externalSource", "{name}", "user", "{login}")] = [](Params & p)->Request*{ return new RequestModifyExternalSourceUser(p[0].asStr(),p[1].asStr(),false); };
	pim->delete_[ReqType("externalSource", "{name}", "user", "{login}")] = [](Params & p)->Request*{ return new RequestModifyExternalSourceUser(p[0].asStr(),p[1].asStr(),true); };

	// links in external sources
	pim->put    [ReqType("allele", "{CAid}", "externalSource", "{name}")]                                     = [](Params & p)->Request*{ return new RequestModifyLink(false,p[0].asStr(),p[1].asStr()); };
	pim->put    [ReqType("allele", "{CAid}", "externalSource", "{name}").param("p1")]                         = [](Params & p)->Request*{ return new RequestModifyLink(false,p[0].asStr(),p[1].asStr(),{p[2].asStr()}); };
	pim->put    [ReqType("allele", "{CAid}", "externalSource", "{name}").param("p1").param("p2")]             = [](Params & p)->Request*{ return new RequestModifyLink(false,p[0].asStr(),p[1].asStr(),{p[2].asStr(),p[3].asStr()}); };
	pim->put    [ReqType("allele", "{CAid}", "externalSource", "{name}").param("p1").param("p2").param("p3")] = [](Params & p)->Request*{ return new RequestModifyLink(false,p[0].asStr(),p[1].asStr(),{p[2].asStr(),p[3].asStr(),p[4].asStr()}); };
	pim->delete_[ReqType("allele", "{CAid}", "externalSource", "{name}")]                                     = [](Params & p)->Request*{ return new RequestModifyLink(true ,p[0].asStr(),p[1].asStr()); };
	pim->delete_[ReqType("allele", "{CAid}", "externalSource", "{name}").param("p1")]                         = [](Params & p)->Request*{ return new RequestModifyLink(true ,p[0].asStr(),p[1].asStr(),{p[2].asStr()}); };
	pim->delete_[ReqType("allele", "{CAid}", "externalSource", "{name}").param("p1").param("p2")]             = [](Params & p)->Request*{ return new RequestModifyLink(true ,p[0].asStr(),p[1].asStr(),{p[2].asStr(),p[3].asStr()}); };
	pim->delete_[ReqType("allele", "{CAid}", "externalSource", "{name}").param("p1").param("p2").param("p3")] = [](Params & p)->Request*{ return new RequestModifyLink(true ,p[0].asStr(),p[1].asStr(),{p[2].asStr(),p[3].asStr(),p[4].asStr()}); };
	pim->get    [ReqType("alleles",supportPagination).param("externalSource")]                                     = [](Params & p)->Request*{ return new RequestQueryAllelesByExternalSource(p.range(),p[0].asStr()); };
	pim->get    [ReqType("alleles",supportPagination).param("externalSource").param("p1")]                         = [](Params & p)->Request*{ return new RequestQueryAllelesByExternalSource(p.range(),p[0].asStr(),{p[1].asStr()}); };
	pim->get    [ReqType("alleles",supportPagination).param("externalSource").param("p1").param("p2")]             = [](Params & p)->Request*{ return new RequestQueryAllelesByExternalSource(p.range(),p[0].asStr(),{p[1].asStr(),p[2].asStr()}); };
	pim->get    [ReqType("alleles",supportPagination).param("externalSource").param("p1").param("p2").param("p3")] = [](Params & p)->Request*{ return new RequestQueryAllelesByExternalSource(p.range(),p[0].asStr(),{p[1].asStr(),p[2].asStr(),p[3].asStr()}); };
	pim->delete_[ReqType("externalSource", "{name}", "links")]                                   = [](Params & p)->Request*{ return new RequestDeleteExternalSourceLinks(p[0].asStr()); };

	// RefGet requests
	pim->get[ReqType("sequence", "service-info")]     = [](Params & p)->Request*{ return new RequestSequenceServiceInfo( ); };
	pim->get[ReqType("sequence", "{id}").param("start").param("end")]     = [](Params & p)->Request*{ return new RequestSequenceByDigest(p[0].asStr(),p[1].asStr(),p[2].asStr()); };
	pim->get[ReqType("sequence", "{id}","metadata")]     = [](Params & p)->Request*{ return new RequestMetadataForSequenceByDigest(p[0].asStr()); };

	// vr Allele
	pim->get[ReqType("vrAllele").param("hgvs")] = [](Params & p)->Request*{ return new RequestVrAlleleForHgvs(p[0].asStr()); };

}

Dispatcher::~Dispatcher()
{
	delete pim;
}

#include <unistd.h>
#include <ios>
#include <iostream>
#include <fstream>
#include <string>
void process_mem_usage(unsigned long& vm_usage, unsigned long& resident_set)
{
   using std::ios_base;
   using std::ifstream;
   using std::string;

   vm_usage     = 0.0;
   resident_set = 0.0;

   // 'file' stat seems to give the most reliable results
   //
   ifstream stat_stream("/proc/self/stat",ios_base::in);

   // dummy vars for leading entries in stat that we don't care about
   //
   string pid, comm, state, ppid, pgrp, session, tty_nr;
   string tpgid, flags, minflt, cminflt, majflt, cmajflt;
   string utime, stime, cutime, cstime, priority, nice;
   string O, itrealvalue, starttime;

   // the two fields we want
   //
   unsigned long vsize;
   long rss;

   stat_stream >> pid >> comm >> state >> ppid >> pgrp >> session >> tty_nr
               >> tpgid >> flags >> minflt >> cminflt >> majflt >> cmajflt
               >> utime >> stime >> cutime >> cstime >> priority >> nice
               >> O >> itrealvalue >> starttime >> vsize >> rss; // don't care about the rest

   stat_stream.close();

   long page_size_kb = sysconf(_SC_PAGE_SIZE) / 1024; // in case x86-64 is configured to use 2MB pages
   vm_usage     = vsize / 1024;
   resident_set = rss * page_size_kb;
}

void Dispatcher::processRequest
	( std::string const & fullUrl                          // fullUrl (original)
	, HTTPP::HTTP::Method const & httpMethod               // GET, POST, PUT etc.
	, std::string const & httpPath                         // path from URL (like /xxx/yyy.html, no host)
	, std::vector<HTTPP::HTTP::KV> const & parameters      // parameters from URL
	, std::shared_ptr<std::vector<char>> body              // request body, nullptr if none
	, HTTPP::HTTP::HttpCode & httpStatus                   // output: response status
	, std::string & contentType                            // output: content-type
	, std::function<std::string()> & callbackNextBodyChunk // output: callback producing response body
	, std::function<void()> & callbackCleanup
	, std::string & redirected                             // output: set if request is redirected
	, void * & request_
	)
{
	contentType = "application/json";
	redirected = "";
	std::map<std::string,std::string> params;
	responseFormat format = responseFormat::prettyJson;
	std::string fields = "";
	uint64_t solutionSizeLimit = 1000000;
	bool gbLogin = false;
	bool gbToken = false;
	bool gbTime  = false;
	std::string gbLoginValue = "";

	// std::cerr << "This is path of the request from dispatcher => " << httpPath << std::endl;
	// std::cerr << std::regex_match(httpPath, std::regex r("sequence/[a-zA-Z0-9-\\]+$")) << std::endl;



	for (auto const & kv: parameters) {
		if ( kv.first == "gbLogin" ) {
			gbLogin = true;
			gbLoginValue = kv.second;
		} else if ( kv.first == "gbToken" ) gbToken = true;
		else if ( kv.first == "gbTime"  ) gbTime = true;
		else if ( kv.first == "maxReportedTranscripts" ) solutionSizeLimit = atoi(kv.second.c_str());
		else if ( kv.first == "useHost" ) continue;
		else if ( kv.first == "" ) continue;
		else if ( kv.first == "fields"  ) {
			fields = kv.second;
		} else {
			params[kv.first] = kv.second;
		}
	}

	try {

		std::string FQDN = pim->conf.alleleRegistryFQDN;
		std::vector<std::string> fqdnPath;
		{	// ---------------- parsing FQDN path
			std::string::size_type i = 1;
			while ( true ) {
				std::string::size_type j = FQDN.find('/', i);
				if (j == std::string::npos) {
					fqdnPath.push_back( FQDN.substr(i) );
					break;
				} else {
					fqdnPath.push_back( FQDN.substr(i, j-i) );
				}
				i = j+1;
			}
		}	// ------------------------------

		std::string protocol = "";
		std::vector<std::string> path;
		{	// ---------------- parsing http path
			if (httpPath.size() < 2 || httpPath[0] != '/') throw std::logic_error("Bad request: httpPath=" + httpPath); // TODO
			std::string::size_type i = 1;
			while ( true ) {
				std::string::size_type j = httpPath.find('/', i);
				if (j == std::string::npos) {
					path.push_back( httpPath.substr(i) );
					break;
				} else {
					path.push_back( httpPath.substr(i, j-i) );
				}
				i = j+1;
			}
			i = path.back().find('.');
			if (i != std::string::npos) {
				protocol = path.back().substr(i+1);
				path.back().resize(i);
			}
		}	// ------------------------------

		// This is a hack when the path is like sequence/SzCTylwtUXBoMOhBpXuHhYS80iHSwUTQ
		// The response format is set to txt rather than JSON
		// If you are extending /sequence/ path please make sure that this still holds true 
		if(path.size() == 2 && path[0] == "sequence"){
			if(path[1] != "service-info"){
				protocol = "txt";
			}
		}
		// Hack ends here.

		int k=0;
		for(int i=0,j=0;i<fqdnPath.size();i++) {
			for(int j=0;j<path.size();j++) {
				if(fqdnPath[i] == path[j]) {
					k=j+1;
					break;
				}
			}
		}

		std::vector<std::string> strippedPath;
		for(int i=k;i<path.size();i++) {
			strippedPath.push_back(path[i]);
		}
		path = strippedPath;


		if (protocol != "") {
			if (protocol == "json" || protocol == "jsonld") format = responseFormat::compressedJson;
			else if (protocol == "html") format = responseFormat::html;
			else if (protocol == "txt") format = responseFormat::txt;
			else throw std::logic_error("Bad request: protocol=" + protocol); // TODO
		}

		bool authenticated = false;
		if (pim->conf.noAuthentication) {
			authenticated = true;
			if ( ! gbLogin || pim->conf.superusers.count(gbLoginValue) ) {
				gbLoginValue = "admin";
			}
		} else if (gbLogin && gbToken && gbTime) {
			authenticated = authorization(pim->conf.genboree_db,fullUrl,pim->conf.genboree_allowedHostnames);
			if (pim->conf.superusers.count(gbLoginValue)) {
				gbLoginValue = "admin";
				authenticated = true;
			}
		}

		std::map<ReqType,std::function<Request*(Params&)>> const * requests;
		switch (httpMethod) {
			case HTTPP::HTTP::Method::GET :
				requests = &(pim->get );
				break;
			case HTTPP::HTTP::Method::PUT :
				if (! authenticated) throw ExceptionAuthorizationError("You have no privileges to send HTTP PUT requests.");
				if (pim->conf.readOnly) throw ExceptionSystemInReadOnlyMode();
				requests = &(pim->put );
				break;
			case HTTPP::HTTP::Method::POST:
				requests = &(pim->post);
				break;
			case HTTPP::HTTP::Method::DELETE_:
				if (! authenticated) throw ExceptionAuthorizationError("You have no privileges to send HTTP DELETE requests.");
				if (pim->conf.readOnly) throw ExceptionSystemInReadOnlyMode();
				requests = &(pim->delete_);
				break;
			default:
				throw std::logic_error("Incorrect HTTP request type: " + HTTPP::HTTP::to_string(httpMethod));
		}

		bool matchPath = false;
		Params matchedParams;
		auto it = requests->begin();
		for ( ;  it != requests->end();  ++it ) {
			if (it->first.matchPath(path)) {
				matchPath = true;
				if (it->first.matchParams(path,params,matchedParams)) break;
			}
		}
		if (it == requests->end()) {
			if (matchPath) throw ExceptionIncorrectRequest("Set of parameters does not match for path: " + httpPath);
			else throw ExceptionIncorrectRequest("Incorrect path: " + httpPath);
		}
		if (it->first.requiresPayload() && (body == nullptr)) throw ExceptionIncorrectRequest("This request requires payload");
		if ((! it->first.requiresPayload()) && (body != nullptr)) throw ExceptionIncorrectRequest("This request is not supposed to have payload");
		matchedParams.setPayload(body);

		auto lock = &pim->synch;
		Request * request = ( (it->second)(matchedParams) );
		request_ = request;
		if(request != NULL) {
#ifdef DEBUG_FLAG
			std::lock_guard<std::mutex> synchScope(*lock);
			pim->memCount++;
			uint64_t id = (uint64_t)request;
			pim->timingMap[id] = request;
#endif
		}
		auto memCount = &pim->memCount;
		auto aTimingMap = &pim->timingMap;

		callbackCleanup = [request, memCount, aTimingMap, lock]()->void
		{
			if(request != NULL) {
				std::lock_guard<std::mutex> synchScope(*lock);
#ifdef DEBUG_FLAG
				auto timer = std::chrono::system_clock::now();
				auto logTime = std::chrono::duration_cast<std::chrono::milliseconds>(timer - request->timer).count();
				unsigned long vm, rss;
				process_mem_usage(vm, rss);
				std::cout << request->logRequest << "\telapsed time [ms]:\t" << logTime << "\t" << "VM:" << vm << "\t" << "RSS:" << rss << std::endl;
				std::stringstream ss;
				ss << request << " " << request->logRequest;
				uint64_t id = (uint64_t)request;
				(*aTimingMap).erase(id);
				int counter = 1;
				for(auto x : (*aTimingMap)) {
					std::cout << counter << " " << x.second->logRequest << ": " << "running" << std::endl;
					counter++;
				}
#endif
				delete request;
#ifdef DEBUG_FLAG
				(*memCount)--;
				std::cout << "Active request count:" << (*memCount) << std::endl;
#endif
			}
		};

		if (format == responseFormat::html) {
			httpStatus = HTTPP::HTTP::HttpCode::Redirect;
			redirected = request->getRequestToGUI();
			return;
		} else if ( format == responseFormat::txt || dynamic_cast<RequestAnnotateVcf*>(request) != nullptr ) {
			// if returned format is txt or the request returns VCF
			contentType = "text/plain";
		}

		request->logTime = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
		request->timer = std::chrono::system_clock::now();
		request->logLogin = gbLoginValue;
		request->logMethod = to_string(httpMethod);
		request->logRequest = fullUrl;
		request->solutionSizeLimit = solutionSizeLimit;
		request->startProcessingThread(format, fields);

		if (path.front().back() == 's') {
			callbackNextBodyChunk = [request]()->std::string
			{
				std::string chunk = "";
				while (chunk == "") if ( ! request->nextChunkOfResponse(chunk) ) return "";
				return chunk;
			};
		} else {
			std::shared_ptr<std::string> response(new std::string(""));
			for ( std::string chunk = ""; request->nextChunkOfResponse(chunk); ) (*response) += chunk;
			callbackNextBodyChunk = [response]()->std::string
			{
				std::string chunk = "";
				std::swap( *response, chunk );
				return chunk;
			};
		}
		httpStatus = errorType2httpStatus( request->lastErrorType() );

	} catch (...) {
		Document doc = DocumentError::createFromCurrentException();
		OutputFormatter formatter(documentType::null);
		if (format != responseFormat::compressedJson) format = responseFormat::prettyJson; // format for error response
		formatter.setFormat(format, "all");
		httpStatus = errorType2httpStatus( doc.error().type );
		std::shared_ptr<std::string> response(new std::string(formatter.createOutput(doc)));
		callbackNextBodyChunk = [response]()->std::string {
			std::string toReturn = *response;
			*response = "";
			return toReturn;
		};
	}
}
