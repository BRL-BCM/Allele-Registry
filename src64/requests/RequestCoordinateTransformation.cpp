#include "requests.hpp"
#include "coordinateTransformationHelper.hpp"


RequestCoordinateTransformation::RequestCoordinateTransformation(ResultSubset const & rs, std::string const & assembly,std::string const & chr,unsigned int const & cstart, unsigned int const & cend)
: Request(documentType::coordinateTransform), fRange(rs), assembly(assembly), chr(chr), cstart(cstart), cend(cend), acse(true)
{
};

RequestCoordinateTransformation::RequestCoordinateTransformation(ResultSubset const & rs, std::string const & glDesc)
: Request(documentType::coordinateTransform), fRange(rs), acse(false)
{
	int i = 0;
	std::string word = glDesc;
	while(i < word.size())
	{
		if(word[i] == '(' || word[i] == ')')
		{
			word.erase(i,1);
		} else {
			i++;
		}
	}
	int j=0;
	bool range = false;
	for(int i=0;i<word.size();i++) {
		if(word[i] == ' ') {
			assembly = word.substr(j,i-j);
			j = i+1;
		} else
		if(word[i] == ':') {
			chr = word.substr(j,i-j);
			j = i+1;
		} else
		if(word[i] == '-') {
			cstart = atoi(word.substr(j,i-j).c_str());
			j = i+1;
			range = true;
		}
	}
	if(range) cend = atoi(word.substr(j).c_str());
	else { cstart = atoi(word.substr(j).c_str()); cend = cstart; }
	if(chr.find("chr")!=std::string::npos) chr = chr.substr(3);
};

RequestCoordinateTransformation::~RequestCoordinateTransformation()
{
	stopProcessingThread();
}

void RequestCoordinateTransformation::process(){

	textInput v;
	v = {assembly, chr, cstart, cend} ;

	genomicCoordinateTransformer gct = {referencesDb, v};

	std::vector<DocumentCoordinates> documents;
	documents.push_back(DocumentCoordinates());

	std::stringstream out;
	bool first = true;
	for(auto & coord : gct.transformation){
		if(coord.assembly == ReferenceGenome::rgGRCh38){
			documents[0].grch38Assembly = toString(coord.assembly);
			documents[0].grch38Chr = toString(coord.chr);
			documents[0].grch38Start = coord.region.left();
			documents[0].grch38End = coord.region.right();
			if(first) first = false; else out << '\t';
			if(cstart != cend) out << documents[0].grch38Assembly << " (chr" << documents[0].grch38Chr << ":" << documents[0].grch38Start << "-" << documents[0].grch38End << ")";
			else out << documents[0].grch38Assembly << " (chr" << documents[0].grch38Chr << ":" << documents[0].grch38Start << ")";
		}
		else if (coord.assembly == ReferenceGenome::rgGRCh37){
			documents[0].grch37Assembly = toString(coord.assembly);
			documents[0].grch37Chr = toString(coord.chr);
			documents[0].grch37Start = coord.region.left();
			documents[0].grch37End = coord.region.right();
			if(first) first = false; else out << '\t';
			if(cstart != cend) out << documents[0].grch37Assembly << " (chr" << documents[0].grch37Chr << ":" << documents[0].grch37Start << "-" << documents[0].grch37End << ")";
			else out << documents[0].grch37Assembly << " (chr" << documents[0].grch37Chr << ":" << documents[0].grch37Start << ")";
		}
		else if(coord.assembly == ReferenceGenome::rgNCBI36) {
			documents[0].ncbi36Assembly = toString(coord.assembly);
			documents[0].ncbi36Chr = toString(coord.chr);
			documents[0].ncbi36Start = coord.region.left();
			documents[0].ncbi36End = coord.region.right();
			if(first) first = false; else out << '\t';
			if(cstart != cend) out << documents[0].ncbi36Assembly << " (chr" << documents[0].ncbi36Chr << ":" << documents[0].ncbi36Start << "-" << documents[0].ncbi36End << ")";
			else out << documents[0].ncbi36Assembly << " (chr" << documents[0].ncbi36Chr << ":" << documents[0].ncbi36Start << ")";
		}
	};
	if(acse) setResponse(documents[0]);
	else setResponse(out.str());
}
