#include "requests.hpp"
#include "coordinateTransformationHelper.hpp"


RequestCoordinateTransformation::RequestCoordinateTransformation(ResultSubset const & rs, std::string const & assembly,std::string const & chr,unsigned int const & cstart, unsigned int const & cend)
: Request(documentType::coordinateTransform), fRange(rs), assembly(assembly), chr(chr), cstart(cstart), cend(cend)
{
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

	for(auto & coord : gct.transformation){
		if(coord.assembly == ReferenceGenome::rgGRCh38){
			documents[0].grch38Assembly = toString(coord.assembly);
			documents[0].grch38Chr = toString(coord.chr);
			documents[0].grch38Start = coord.region.left();
			documents[0].grch38End = coord.region.right();			
		}
		else if (coord.assembly == ReferenceGenome::rgGRCh37){
			documents[0].grch37Assembly = toString(coord.assembly);
			documents[0].grch37Chr = toString(coord.chr);
			documents[0].grch37Start = coord.region.left();
			documents[0].grch37End = coord.region.right();
		}
		else if(coord.assembly == ReferenceGenome::rgNCBI36) {
			std::cerr << "Assembly" << coord.assembly << std::endl;
			documents[0].ncbi36Assembly = toString(coord.assembly);
			documents[0].ncbi36Chr = toString(coord.chr);
			documents[0].ncbi36Start = coord.region.left();
			documents[0].ncbi36End = coord.region.right();
		}
	};

	setResponse(documents[0]);
}
