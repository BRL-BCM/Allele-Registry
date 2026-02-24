/*
Request handler for endpoint to query by digest and produce refget compatible response:
*/

#include "requests.hpp"
#include "hgvs.hpp"
#include "canonicalization.hpp"

// param foramt - seqName:coord1
//                seqName:coord1-coord2
// ex:            NC_000010.9:1000
// ex:            NC_000010.9:1000-2000
// note: the response will contain all genes that are either
// completely contained within the range [coord1,coord2]
// or have one locus point contained within that range
// or have locus points on opposite sides of that range
// extra note: the gene 'intersection' only considers locus points of genes
// i.e. the minimal and maximal coordinates, it does not check transcript
// intersections at this time
RequestGeneIntersection::RequestGeneIntersection(std::string const & qParam)
:Request(documentType::activeGenomicVariant)
{
	size_t findC = qParam.find(":");
	if(findC == std::string::npos) {
		throw std::runtime_error("Please make sure to have a ':' in query string, ex: NC_000010.9:1000 or NC_000010.9:1000-2000");
	}
	std::string seqName = qParam.substr(0,findC);
	std::cout << seqName << std::endl;
	name = seqName;
	refId = referencesDb->getReferenceId(seqName);
	size_t findL = qParam.find("-");
	if(findL == std::string::npos) {
		std::string startStr = qParam.substr(findC+1);
		std::cout << startStr << std::endl;
		coordStart = coordEnd = boost::lexical_cast<size_t>(startStr);
	} else {
		std::string startStr = qParam.substr(findC+1,(findL-findC)-1);
		std::cout << startStr << std::endl;
		std::string endStr = qParam.substr(findL+1);
		std::cout << endStr << std::endl;
		coordStart = boost::lexical_cast<size_t>(startStr);
		coordEnd   = boost::lexical_cast<size_t>(endStr);
		if(coordStart > coordEnd) {
			throw std::runtime_error("Please make sure startCoord <= endCoord");
		}
	}
}

RequestGeneIntersection::~RequestGeneIntersection(){
	stopProcessingThread();
}

void RequestGeneIntersection::process(){
	std::vector<std::string> genes;
	auto geneList = referencesDb->getGenes();
	for(auto g : geneList) {
		try {
			std::string symbol = g.hgncSymbol;
			//std::cout << symbol << std::endl;
			auto regions = referencesDb->getGeneRegions(symbol);
			for(auto r : regions) {
				auto names = referencesDb->getNames(r.first);
				for(auto n : names) {
					//std::cout << "\t" << n << std::endl;
					if(n == name) {
						for(auto seq : r.second) {
							if(seq.left() < coordStart && seq.right() > coordEnd) {
								genes.push_back(symbol);
							} else
							if(seq.left() >= coordStart && seq.right() <= coordEnd) {
								genes.push_back(symbol);
							} else
							if(seq.left() >= coordStart && seq.left() <= coordEnd) {
								genes.push_back(symbol);
							} else
							if(seq.right() >= coordStart && seq.right() <= coordEnd) {
								genes.push_back(symbol);
							}
							break;
						}
					}
				}
			}
		} catch (...) {
			
		}
	}
	std::stringstream ss;
	ss << "[";
	for(int i=0; i<genes.size(); i++) {
		if(i==0) {
			ss << "\"" << genes[i] << "\"";
		} else {
			ss << ", " << "\"" << genes[i] << "\"";
		}
	}
	ss << "]";
	setResponse(ss.str());
}
