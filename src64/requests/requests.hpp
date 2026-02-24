#ifndef REQUESTS_HPP_
#define REQUESTS_HPP_


#include "../allelesDatabase/allelesDatabase.hpp"
#include "OutputFormatter.hpp"
#include <fstream>
#include <memory>
#include <thread>
#include <atomic>
#include <mutex>
#include <list>
#include <set>
#include "../kafka/sharedMemoryConsumer.hpp"

struct ResultSubset {
	static ResultSubset const all;
	unsigned skip  = 0;
	unsigned limit = std::numeric_limits<unsigned>::max();
	ResultSubset() {}
	ResultSubset(unsigned pSkip, unsigned pLimit) : skip(pSkip), limit(pLimit) {}
	inline unsigned right() const { return ((std::numeric_limits<unsigned>::max()-skip > limit) ? (limit+skip) : (std::numeric_limits<unsigned>::max())); }
};


class Request
{
private:
	// response in chunks
	std::mutex chunksToSend_access;
	std::mutex chunksToSend_access2;
	std::list<std::string> chunksToSend;
	bool chunksToSend_started  = false;
	bool chunksToSend_finished = false;
	errorType error = errorType::NoErrors;
	uint64_t bytesCount = 0;
	uint64_t documentsCount = 0;
	// thread object
	std::thread * processingThread = NULL;
	// method run in thread
	void internalThread() noexcept;
	// requests log
	static std::mutex requestLog_access;
	static std::ofstream requestLog;
protected:
	// flag - signalization of request termination
	std::atomic<bool> killThread;
	alignment fAlignment = alignment::rightAligned;
	// global stuff
	static sharedMemoryConsumer * smConsumer;
	static ReferencesDatabase const * referencesDb;
	static AllelesDatabase * allelesDb;
	static Configuration configuration;
	// must be called in destructor of derived class
	void stopProcessingThread() noexcept;
	// method called by internal thread - must be implemented in derived class
	virtual void process() = 0;
	virtual void cleanup() = 0;
	// methods used in process() to build response - tools used in derived class
	// it checks also if the thread was killed, if yes, it finalizes the response
	// and throws exception
	void addChunkOfResponse(std::vector<Document> const & documents);
	void addChunkOfResponse(std::string && chunk);
	void setResponse(Document const & doc);
	void setResponse(std::string const & text);

	OutputFormatter outputBuilder;
	// constructor
	Request(documentType docType) : killThread(false), outputBuilder(docType) {}
public:
	// methods used in derived class
	void fillVariantsDetails(std::vector<Document> & docs, alignment pAlignment = alignment::leftAligned) const;    // TODO - should be protected or static
	void mapVariantsToMainGenome(std::vector<Document> & docs, alignment pAlignment = alignment::leftAligned) const;// TODO - should be protected or static
	// this must be called once at the beginning of application
	static void initGlobalVariables(Configuration const & conf);
	// call this method to start request processing (it starts processing in separate thread and returns)
	void startProcessingThread(responseFormat respFormat, std::string const & respFields = "");
	// call this method to get chunk of response (after calling the previous method)
	bool nextChunkOfResponse(std::string & chunk);  // returns: true - chunk returned, false - no more data
	// get error type
	errorType lastErrorType();
	// get request to corresponding GUI page (if available)
	virtual std::string getRequestToGUI() const;
	// standard destructor, not copyable, not movable
	virtual ~Request() {
		if(this->processingThread != NULL)
		{
			this->stopProcessingThread();
			if(this->processingThread != NULL) {
				std::cout << "destroying request with active processingThread" << std::endl;
				exit(0);
			}
		}
	}
	Request(Request const &) = delete;
	Request(Request &&) = delete;
	Request& operator=(Request const &) = delete;
	Request& operator=(Request &&) = delete;
	// fields with data to log
	unsigned logTime;
	std::chrono::time_point<std::chrono::system_clock> timer;
	std::string logMethod;
	std::string logLogin;
	std::string logRequest;
	uint64_t solutionSizeLimit = 1000000;
public:
	Request* setAlignment(alignment const & pAlignment) {
		this->fAlignment = pAlignment;
		return this;
	}

};


class RequestGeneIntersection : public Request
{
private:
	ReferenceId refId;
	std::string name;
	uint32_t coordStart;
	uint32_t coordEnd;
protected:
	virtual void process();
	virtual void cleanup() {};
public:
	RequestGeneIntersection(std::string const & qParam);
	virtual ~RequestGeneIntersection();
};



class RequestFetchAllelesByDefinition : public Request
{
private:
	struct Pim;
	Pim * pim;
protected:
	virtual void process();
	virtual void cleanup() {};
public:
	RequestFetchAllelesByDefinition(std::shared_ptr<std::vector<char>> pBody, std::string const & columnsDefinitions, bool registerNewAlleles, bool named = false); // payload
	virtual ~RequestFetchAllelesByDefinition();
};


class RequestFetchAlleleById : public Request
{
private:
	std::string fId;
protected:
	virtual void process();
	virtual void cleanup() {};
public:
	RequestFetchAlleleById(std::string const & id);
	virtual std::string getRequestToGUI() const;
	virtual ~RequestFetchAlleleById();
};


class RequestFetchAlleleByHgvs : public Request
{
private:
	std::ofstream payload;
	std::string fHgvs;
	bool fRegisterNewAllele;
	bool fNamed;
protected:
	virtual void process();
	virtual void cleanup() {};
public:
	RequestFetchAlleleByHgvs(std::string const & hgvs, bool registerNewAllele, bool named = false);
	virtual std::string getRequestToGUI() const;
	virtual ~RequestFetchAlleleByHgvs();
};


class RequestQueryAllelesByRegion : public Request
{
private:
	std::thread * worker;
	std::list<std::vector<Document> > docsList;
	bool done;
	std::mutex m;
	ResultSubset fRange;
	ReferenceId fRefId;
	RegionCoordinates fRegion;
	std::string fChr;
	std::string fRefGenome;
	alignment fAlignment;
protected:
	virtual void process();
	virtual void cleanup();
public:
	RequestQueryAllelesByRegion(ResultSubset const & rs, std::string const & ref, std::string const & pRefGenome = "", unsigned from = 0, unsigned to = std::numeric_limits<unsigned>::max(), std::string const fAlignment = "left");
	virtual ~RequestQueryAllelesByRegion();
};


class RequestQueryAlleleIntersection : public Request
{
private:
	ResultSubset fRange;
	ReferenceId fRefId;
	std::vector<RegionCoordinates> fRegionOrig;
	std::vector<RegionCoordinates> fRegion;
	std::string fHgvs;
	std::string fChr;
	std::string fRefGenome;
	alignment fAlignment;
protected:
	virtual void process();
	virtual void cleanup() {};
public:
	RequestQueryAlleleIntersection(ResultSubset const & rs, std::string const pHgvs, std::string const pAlignment = "left");
	virtual ~RequestQueryAlleleIntersection();
};


class RequestQueryAlleles : public Request
{
private:
	ResultSubset fRange;
	bool fProteinAlleles;
protected:
	virtual void process();
	virtual void cleanup() {};
public:
	// proteinAlleles: if true then protein only; else genomic only
	RequestQueryAlleles(ResultSubset const & rs, bool proteinAlleles) : Request(documentType::activeGenomicVariant), fRange(rs), fProteinAlleles(proteinAlleles) {}
	virtual ~RequestQueryAlleles();
};


class RequestQueryAllelesByGene : public Request
{
private:
	ResultSubset fRange;
	std::string fGeneName;
	unsigned fMaxChunkSize;
protected:
	virtual void process();
	virtual void cleanup() {};
public:
	RequestQueryAllelesByGene(ResultSubset const & rs, std::string const & geneName, unsigned const maxChunkSize) : Request(documentType::activeGenomicVariant), fRange(rs), fGeneName(geneName), fMaxChunkSize(maxChunkSize) {}
	virtual ~RequestQueryAllelesByGene();
};


class RequestQueryAllelesById : public Request
{
private:
	ResultSubset fRange;
	std::vector<std::pair<identifierType,std::string>> fIds;
protected:
	virtual void process();
	virtual void cleanup() {};
public:
	RequestQueryAllelesById(ResultSubset const & rs, std::vector<std::string> const & ids);
	RequestQueryAllelesById(ResultSubset const & rs, std::vector<std::pair<identifierType,std::string>> const & ids)
	: Request(documentType::activeGenomicVariant), fRange(rs), fIds(ids) {}
	virtual ~RequestQueryAllelesById();
};


class RequestFetchGene : public Request
{
private:
	unsigned fGnId = 0;
	std::string fHgncSymbol = "";
protected:
	virtual void process();
	virtual void cleanup() {};
public:
	RequestFetchGene(std::string const & idOrSymbol, bool hgncSymbol = false);
	virtual ~RequestFetchGene();
};


class RequestQueryGenes : public Request
{
private:
	ResultSubset fRange;
	std::string fName;
protected:
	virtual void process();
	virtual void cleanup() {};
public:
	// filter by name if set
	RequestQueryGenes(ResultSubset rs = ResultSubset::all, std::string const & name = "")
	: Request(documentType::gene), fRange(rs), fName(name) { }
	virtual ~RequestQueryGenes();
};


class RequestFetchReference : public Request
{
private:
	ReferenceId fRefId;
	std::string fRefBuild;
	bool fDetails = false;
protected:
	virtual void process();
	virtual void cleanup() {};
public:
	RequestFetchReference(std::string const & ref, std::string const & pRefBuild = "", bool details = false);
	virtual ~RequestFetchReference();
};


class RequestQueryReferences : public Request
{
private:
	ResultSubset fRange;
	std::string fName;
	bool fGene;
protected:
	virtual void process();
	virtual void cleanup() {};
public:
	RequestQueryReferences(ResultSubset rs, std::string const & geneOrName, bool geneName = false);
	//RequestQueryReferences(ResultSubset rs, ReferenceGenome refGenome);
	virtual ~RequestQueryReferences();
};

class RequestQueryAllelesByGeneAndMutation : public Request
{
private:
	struct Pim;
	Pim * pim;
protected:
	virtual void process();
	virtual void cleanup() {};
public:
	RequestQueryAllelesByGeneAndMutation(std::string const & gene, std::string const & mutation);
	virtual ~RequestQueryAllelesByGeneAndMutation();
};


class RequestDeleteAlleles : public Request
{
private:
	struct Pim;
	Pim * pim;
protected:
	virtual void process();
	virtual void cleanup() {};
public:
	RequestDeleteAlleles(std::shared_ptr<std::vector<char>> pBody, std::string const & columnsDefinitions); // payload
	virtual ~RequestDeleteAlleles();
};


class RequestBumpId : public Request
{
private:
	std::string mHgvs;
protected:
	virtual void process();
	virtual void cleanup() {};
public:
	RequestBumpId(std::string hgvs); 
	virtual ~RequestBumpId();
};


class RequestBumpIds : public Request
{
private:
	struct Pim;
	Pim * pim;
protected:
	virtual void process();
	virtual void cleanup() {};
public:
	RequestBumpIds(std::shared_ptr<std::vector<char>> pBody); 
	virtual ~RequestBumpIds();
};


class RequestSetIds : public Request
{
private:
	struct Pim;
	Pim * pim;
protected:
	virtual void process();
	virtual void cleanup() {};
public:
	RequestSetIds(std::shared_ptr<std::vector<char>> pBody); 
	virtual ~RequestSetIds();
};


class RequestAnnotateVcf : public Request
{
private:
	struct Pim;
	Pim * pim;
protected:
	virtual void process();
	virtual void cleanup() {};
public:
	RequestAnnotateVcf(std::shared_ptr<std::vector<char>> pBody, std::string const & assembly, std::string const & ids, bool registerNewVariants);
	virtual ~RequestAnnotateVcf();
};

// ============================= external sources

class RequestQueryExternalSources : public Request
{
private:
	struct Pim;
	Pim * pim;
protected:
	virtual void process();
	virtual void cleanup() {};
public:
	RequestQueryExternalSources(std::string srcName = "");
	virtual ~RequestQueryExternalSources();
};

class RequestCreateExternalSource : public Request
{
private:
	struct Pim;
	Pim * pim;
protected:
	virtual void process();
	virtual void cleanup() {};
public:
	RequestCreateExternalSource(std::string const & name, std::string const & url, std::string const & paramType, std::string const & guiName, std::string const & guiLabel, std::string const & guiUrl);
	virtual ~RequestCreateExternalSource();
};

class RequestModifyExternalSource : public Request
{
private:
	struct Pim;
	Pim * pim;
protected:
	virtual void process();
	virtual void cleanup() {};
public:
	// fieldToModify: 1 - url, 2 - guiSrcName, 3 - guiLabel, 4 - guiUrl
	RequestModifyExternalSource(std::string const & name, unsigned fieldToModify, std::string const & newValue);
	virtual ~RequestModifyExternalSource();
};

class RequestDeleteExternalSources : public Request
{
private:
	struct Pim;
	Pim * pim;
protected:
	virtual void process();
	virtual void cleanup() {};
public:
	RequestDeleteExternalSources(std::string const & name);
	virtual ~RequestDeleteExternalSources();
};

class RequestQueryExternalSourceUsers : public Request
{
private:
	struct Pim;
	Pim * pim;
protected:
	virtual void process();
	virtual void cleanup() {};
public:
	RequestQueryExternalSourceUsers(std::string const & srcName);
	virtual ~RequestQueryExternalSourceUsers();
};

class RequestModifyExternalSourceUser : public Request
{
private:
	struct Pim;
	Pim * pim;
protected:
	virtual void process();
	virtual void cleanup() {};
public:
	RequestModifyExternalSourceUser(std::string const & srcName, std::string const & usrName, bool deleteRecord = false); // false = add
	virtual ~RequestModifyExternalSourceUser();
};

class RequestModifyLink : public Request
{
private:
	struct Pim;
	Pim * pim;
protected:
	virtual void process();
	virtual void cleanup() {};
public:
	RequestModifyLink(bool deleteRecord, std::string const & ca, std::string const & srcName, std::vector<std::string> params = std::vector<std::string>()); // false = add
	virtual ~RequestModifyLink();
};

class RequestQueryAllelesByExternalSource : public Request
{
private:
	struct Pim;
	Pim * pim;
protected:
	virtual void process();
	virtual void cleanup() {};
public:
	RequestQueryAllelesByExternalSource(ResultSubset rs, std::string const & srcName, std::vector<std::string> params = std::vector<std::string>());
	virtual ~RequestQueryAllelesByExternalSource();
};

class RequestDeleteExternalSourceLinks : public Request
{
private:
	struct Pim;
	Pim * pim;
protected:
	virtual void process();
	virtual void cleanup() {};
public:
	RequestDeleteExternalSourceLinks(std::string const & srcName);
	virtual ~RequestDeleteExternalSourceLinks();
};

class RequestDeleteIdentifiers : public Request
{
private:
	struct Pim;
	Pim * pim;
protected:
	virtual void process();
	virtual void cleanup() {};
public:
	RequestDeleteIdentifiers(std::string const & name, uint64_t from = 0, uint64_t to = std::numeric_limits<uint64_t>::max());
	virtual ~RequestDeleteIdentifiers();
};

/* Requests implemented by Ronak. The above request are by Piotr Pawliczek */

class RequestCoordinateTransformation : public Request
{
private:
	bool acse;
	ResultSubset fRange;
	std::string assembly;
	std::string chr;
	unsigned int cstart;
	unsigned int cend; 
	ReferencesDatabase const * seqDb;
protected:
	virtual void process();
	virtual void cleanup() {};
public:
	RequestCoordinateTransformation(ResultSubset const & rs, std::string const & assembly,std::string const & chr,unsigned int const & cstart, unsigned int const & cend);
	RequestCoordinateTransformation(ResultSubset const & rs, std::string const & glDesc);
	virtual ~RequestCoordinateTransformation();
};

class RequestCoordinateTransformations : public Request
{
private:
	struct Pim;
	Pim * pim;
protected:
	virtual void process();
	virtual void cleanup() {};
public:
	RequestCoordinateTransformations(std::shared_ptr<std::vector<char>> pBody, std::string const & columnsDefinitions); // payload
    virtual ~RequestCoordinateTransformations();
};

class RequestSequenceServiceInfo : public Request 
{
private:
	struct Pim;
	Pim * pim;
protected:
	virtual void process();
	virtual void cleanup() {};
public:
	RequestSequenceServiceInfo();
	virtual ~RequestSequenceServiceInfo();
};

class RequestSequenceByDigest : public Request
{
private: 
	std::string sequenceDigestId;
	std::string cstart;
	std::string cend;
protected:
	virtual void process();
	virtual void cleanup() {};
public:
	RequestSequenceByDigest(std::string const & pId, std::string const & cstart, std::string const & cend);	
	virtual ~RequestSequenceByDigest();	
};

class RequestVrAlleleForHgvs : public Request
{
private:
	std::string fHgvs;
protected:
	virtual void process();
	virtual void cleanup() {};
public:
	RequestVrAlleleForHgvs(std::string const & hgvs);
	virtual ~RequestVrAlleleForHgvs();
};

class RequestMetadataForSequenceByDigest : public Request
{
private: 
	std::string digest;
protected:
	virtual void process();
	virtual void cleanup() {};
public:
	RequestMetadataForSequenceByDigest(std::string const & qDigest);
	virtual ~RequestMetadataForSequenceByDigest();
};

#endif /* REQUESTS_HPP_ */
