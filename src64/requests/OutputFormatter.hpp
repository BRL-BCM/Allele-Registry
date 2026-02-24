#ifndef OUTPUTFORMATTER_HPP_
#define OUTPUTFORMATTER_HPP_

#include "../core/document.hpp"
#include "../referencesDatabase/referencesDatabase.hpp"
#include "../commonTools/DocumentSettings.hpp"

enum responseFormat {
	  prettyJson
	, compressedJson
	, html
	, txt   // for alleles only, dumps "hgvs <tab> id"
};


class OutputFormatter
{
private:
	struct Pim;
	Pim * pim;
public:
	static std::string carURI;
	static ReferencesDatabase const * refDb;
	OutputFormatter(documentType docType);
	~OutputFormatter();
	// set format of the response
	void setFormat(responseFormat, std::string const & fields);
	// create chunk of response from single document
	std::string createOutput(Document const &) const;
	// method to check what data is needed
	bool returnsGenomeBuildsVariants() const;
	bool returnsGenesRegionsVariants() const;
	bool returnsTranscriptsVariants () const;
	bool returnsProteinsVariants    () const;
	jsonBuilder::DocumentSettings externalSourcesConfiguration() const;
	bool prettyJson() const;
	bool prettyOrCompressedJson() const;
};


#endif /* OUTPUTFORMATTER_HPP_ */
