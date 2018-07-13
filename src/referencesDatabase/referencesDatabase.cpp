#include "tabSeparatedFile.hpp"
#include "shmemContainers.hpp"
#include "referencesDatabase.hpp"
#include "../core/exceptions.hpp"
#include "../commonTools/assert.hpp"
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/filesystem.hpp>
#include <boost/timer/timer.hpp>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/interprocess/sync/named_semaphore.hpp>
#include <map>
#include <fstream>


inline std::string translateToAminoAcid2(std::string const & seq)
{
    std::string res = "";
    for ( unsigned i = 0;  i+3 <= seq.size();  i+=3 ) {
        std::string const c = seq.substr(i,3);
        // -----
        if (c == "TTT" || c == "TTC") res += "F";
        if (c == "TTA" || c == "TTG") res += "L";
        if (c == "CTT" || c == "CTC") res += "L";
        if (c == "CTA" || c == "CTG") res += "L";
        if (c == "ATT" || c == "ATC" || c == "ATA") res += "I";
        if (c == "ATG") res += "M";
        if (c == "GTT" || c == "GTC") res += "V";
        if (c == "GTA" || c == "GTG") res += "V";
        // -----
        if (c == "TCT" || c == "TCC") res += "S";
        if (c == "TCA" || c == "TCG") res += "S";
        if (c == "CCT" || c == "CCC") res += "P";
        if (c == "CCA" || c == "CCG") res += "P";
        if (c == "ACT" || c == "ACC") res += "T";
        if (c == "ACA" || c == "ACG") res += "T";
        if (c == "GCT" || c == "GCC") res += "A";
        if (c == "GCA" || c == "GCG") res += "A";
        // -----
        if (c == "TAT" || c == "TAC") res += "Y";
        if (c == "TAA" || c == "TAG") res += "*"; // stop codon
        if (c == "CAT" || c == "CAC") res += "H";
        if (c == "CAA" || c == "CAG") res += "Q";
        if (c == "AAT" || c == "AAC") res += "N";
        if (c == "AAA" || c == "AAG") res += "K";
        if (c == "GAT" || c == "GAC") res += "D";
        if (c == "GAA" || c == "GAG") res += "E";
        // -----
        if (c == "TGT" || c == "TGC") res += "C";
        if (c == "TGA") res += "*";  // stop codon
        if (c == "TGG") res += "W";
        if (c == "CGT" || c == "CGC") res += "R";
        if (c == "CGA" || c == "CGG") res += "R";
        if (c == "AGT" || c == "AGC") res += "S";
        if (c == "AGA" || c == "AGG") res += "R";
        if (c == "GGT" || c == "GGC") res += "G";
        if (c == "GGA" || c == "GGG") res += "G";
    }
    if (seq.size() % 3) {
    	// special case for ENSEMBL partial transcripts
    	std::string const s = seq.substr( seq.size() - seq.size()%3 );
    	if (s == "CT") res += "L";
    	else if (s == "GT") res += "V";
    	else if (s == "TC") res += "S";
    	else if (s == "CC") res += "P";
    	else if (s == "AC") res += "T";
    	else if (s == "GC") res += "A";
    	else if (s == "CG") res += "R";
    	else if (s == "GG") res += "G";
    	else res += "X";
    }
    return res;
}


static uint64_t parseUnsignedInt(std::string const & s)
{
	uint64_t v = 0;
	for (char c: s) {
		if (c < '0' || c > '9') throw std::runtime_error("Incorrect number (non-digit character): " + s);
		uint64_t v2 = v * 10;
		v2 += (c-'0');
		if ((v2 - (c-'0')) / 10 != v) throw std::runtime_error("Incorrect number (overflow): " + s);
		v = v2;
	}
	return v;
}

static uint64_t calculateProteinAccessionIdentifier(std::string const & pName)
{
	std::string name = pName;
	uint64_t accessionType = 0;
	if (name.substr(0,4) == "ENSP") {
		accessionType = 0;
		name = name.substr(4);
	} else if (name.substr(0,3) == "NP_") {
		accessionType = 1;
		name = name.substr(3);
	} else if (name.substr(0,3) == "XP_") {
		accessionType = 2;
		name = name.substr(3);
	} else if (name.substr(0,3) == "AP_") {
		accessionType = 3;
		name = name.substr(3);
	} else {
		throw std::runtime_error("Unknown protein accession name (unknown prefix): " + pName);
	}
	std::string::size_type const dotPos = name.find('.');
	if (dotPos == std::string::npos) {
		throw std::runtime_error("Incorrect protein accession name (missing dot): " + pName);
	}
	// ---- parse main & version numbers
	uint64_t main, version;
	try {
		main = parseUnsignedInt(name.substr(0,dotPos));
		version = parseUnsignedInt(name.substr(dotPos+1));
	} catch (std::exception const & e) {
		throw std::runtime_error("Incorrect protein accession name (" + std::string(e.what()) + "): " + pName);
	}
	// ---- check main number
	if (accessionType == 0) {
		// Ensembl
		if (dotPos != 11) throw std::runtime_error("Incorrect protein accession name (11 digits expected): " + pName);
	} else if (accessionType < 4) {
		// Refseq
		if (dotPos < 6 || dotPos > 13) {
			throw std::runtime_error("Incorrect protein accession name (6-13 digits expected): " + pName);
		}
		if ( (main >> 35) > 0 ) {
			throw std::runtime_error("Unsupported protein accession name (main number too large): " + pName);
		}
		main |= ( (static_cast<uint64_t>(dotPos)-6) << 35 );
	} else {
		ASSERT(false);
	}
	// ---- check version number
	if ( (dotPos + 1 >= name.size()) || (name[dotPos+1] == '0') ) {
		throw std::runtime_error("Incorrect protein accession name (wrong version number): " + pName);
	}
	if ( (version >> 7) > 0 ) {
		throw std::runtime_error("Unsupported protein accession name (version number too large): " + pName);
	}
	// ---- calculate result
	return ( (accessionType << 45) | (main << 7) | version );
}


struct LogScopeWallTime
{
private:
	static unsigned fScopeDepth;
	boost::timer::cpu_timer fTimer;
public:
	LogScopeWallTime(std::string const & scopeTitle)
	{
		++fScopeDepth;
		for (unsigned i = 0; i < fScopeDepth; ++i) std::clog << "========";
		std::clog << " " << scopeTitle << std::endl;
		fTimer.start();
	}
	~LogScopeWallTime()
	{
		for (unsigned i = 0; i < fScopeDepth; ++i) std::clog << "--------";
		std::clog << " Done in " << fTimer.format(3,"%w") << " seconds" << std::endl;
		--fScopeDepth;
	}
};
unsigned LogScopeWallTime::fScopeDepth = 0;



struct ReferencesDatabase::Pim
{
	unsigned const offsetMainGenome = 0;
	unsigned offsetMappedReferences;
	unsigned offsetTranscripts;
	unsigned offsetProteins;

	boost::interprocess::named_mutex * mutexForSharedMemory = nullptr;
	boost::interprocess::named_semaphore * semaphoreForSharedMemory = nullptr;
	boost::interprocess::shared_memory_object * sharedMemory = nullptr;
	boost::interprocess::mapped_region * mappedMemory = nullptr;
	SharedVectorOfStrings mainGenome;

	std::vector<ReferenceId> proteins2transcripts;  // proteinId = referenceId - offsetProteins

	std::vector<GeneralSeqAlignment> mappedReferences;

	std::vector<GeneralSeqAlignment> transcripts;
	std::vector<std::vector<std::pair<RegionCoordinates,RegionCoordinates>>> transcriptsExons;  // unspliced coordinates, spliced coordinates

	std::vector<std::vector<std::string>> names;
	std::map<std::string,unsigned> name2refSeq;

	std::vector<Gene> genes; // by hgnc id
	std::map<std::string, std::vector<unsigned>> genesBySymbol; // symbol -> hgncId

	std::vector<ReferenceMetadata> metadata;  // by RefId

	std::map<uint64_t,ReferenceId> proteinAccessionIdentifier2referenceId;

	// index
	struct RegionIndexElement {
		Alignment alignment;
		unsigned targetEnd;
		unsigned maxTargetEnd = std::numeric_limits<unsigned>::max();
		RegionIndexElement(Alignment const & a) : alignment(a), targetEnd(a.targetRegion().right()) {}
		bool operator<(RegionIndexElement const & e2) const
		{
			if (alignment.targetLeftPosition != e2.alignment.targetLeftPosition) return (alignment.targetLeftPosition < e2.alignment.targetLeftPosition);
			if (targetEnd != e2.targetEnd) return (targetEnd < e2.targetEnd);
			return (alignment.sourceRefId < e2.alignment.sourceRefId);
		}
	};
	std::vector<std::vector<RegionIndexElement>> alignmentsToMainGenome;  // [ mainGenomeRefId ][ ... ]

	void readFromFile( std::string const & filename, std::vector<std::string> & names, SharedVectorOfStrings & target );
	void readFromFile( std::string const & filename, std::vector<std::string> & names, std::vector<std::string> & target);
	void readFromFile( std::string const & filename, std::vector<std::string> & names, std::vector<GeneralSeqAlignment> & target, bool transcripts );
	void readNames( std::string const & filename );
	void readGenes( std::string const & filename );
	void readTranscriptMetadata( std::string const & filename );
	void unspliceTranscripts();
	void buildAlignmentsToMainGenome();
	GeneralSeqAlignment calculateGeneralAlignment(GeneralSeqAlignment const & reference, RegionCoordinates const & region, bool exactTargetEdges = false) const;
};


inline std::vector<std::string> split_with_single_space(std::string const & s, unsigned const minimal_words_number)
{
	std::vector<std::string> v;
	boost::split(v, s, boost::is_any_of(" \t"));
	if (v.size() < minimal_words_number) {
		throw std::runtime_error("At least " + boost::lexical_cast<std::string>(minimal_words_number)
				+ " word(s) was/were expected, found " + boost::lexical_cast<std::string>(v.size()) + ".");
	}
	return v;
}

void ReferencesDatabase::Pim::readFromFile( std::string const & filename, std::vector<std::string> & names, SharedVectorOfStrings & target )
{
	mutexForSharedMemory = new boost::interprocess::named_mutex(boost::interprocess::open_or_create, "genomeIndex_mutex");
	semaphoreForSharedMemory = new boost::interprocess::named_semaphore(boost::interprocess::open_or_create, "genomeIndex_semaphore", 0);
	if ( ! semaphoreForSharedMemory->try_wait() ) {
		if ( mutexForSharedMemory->try_lock() ) {
			LogScopeWallTime scopeLog("Write data to shared memory");
			sharedMemory = new boost::interprocess::shared_memory_object(boost::interprocess::create_only, "genomeIndex_sharedMemory", boost::interprocess::read_write);
			std::vector<std::string> temp;
			readFromFile(filename, names, temp);
			unsigned memSize = 16 + 16*temp.size() + 16 + 16*names.size();
			for (std::string const & t: temp ) memSize += t.size();
			for (std::string const & t: names) memSize += t.size();
			sharedMemory->truncate(memSize);
			mappedMemory = new boost::interprocess::mapped_region(*sharedMemory,boost::interprocess::read_write);
			void * ptr = mappedMemory->get_address();
			target = SharedVectorOfStrings(ptr, temp);
			SharedVectorOfStrings sharedNames(ptr, names);
			for (unsigned count = 32; count; --count) semaphoreForSharedMemory->post();
			return;
		}
		semaphoreForSharedMemory->wait();
	}
	semaphoreForSharedMemory->post(); // we want to keep the semaphore open
	{
		LogScopeWallTime scopeLog("Connect to shared memory");
		sharedMemory = new boost::interprocess::shared_memory_object(boost::interprocess::open_only, "genomeIndex_sharedMemory", boost::interprocess::read_only);
		mappedMemory = new boost::interprocess::mapped_region(*sharedMemory,boost::interprocess::read_only);
		void * ptr = mappedMemory->get_address();
		target = SharedVectorOfStrings(ptr);
		SharedVectorOfStrings sharedNames(ptr);
		for (unsigned i = 0; i < sharedNames.size(); ++i) names.push_back(sharedNames[i].toString());
	}
}

void ReferencesDatabase::Pim::readFromFile( std::string const & filename, std::vector<std::string> & names, std::vector<std::string> & target )
{
	LogScopeWallTime scopeLog("Parse " + filename);
	std::ifstream file;
	file.open(filename);
	for (unsigned lineNo = 1; file.good(); ++lineNo) {
		std::string line;
		try {
			std::getline(file,line);
			boost::trim(line);
			if (line == "" || line[0] == '#') continue;
			if (line[0] == '>') {
				// new record
				std::string const name = split_with_single_space(line.substr(1),1)[0];
				names.push_back(name);
				target.push_back("");
			} else {
				if (target.empty()) throw std::runtime_error("First sequence without name!");
				convertToUpperCase(line);
				target.back() += line;
			}
		} catch (std::exception const & e) {
			throw std::runtime_error("Error when reading from the following file: " + filename + ", line: " + boost::lexical_cast<std::string>(lineNo) + ", message: " + e.what());
		}
	}
}

void ReferencesDatabase::Pim::readFromFile( std::string const & filename, std::vector<std::string> & names, std::vector<GeneralSeqAlignment> & target, bool transcripts )
{
	LogScopeWallTime scopeLog("Parse " + filename);
	std::ifstream file;
	file.open(filename);
	unsigned trgPos = 0;
	bool error = false;
	std::string name;
	for (unsigned lineNo = 1; file.good(); ++lineNo) {
		std::string line;
		try {
			std::getline(file,line);
			if (line == "" || line[0] == '#') continue;
			if (line[0] == '>') {
				// new record
				boost::trim(line);
				name = split_with_single_space(line.substr(1),1)[0];
				if (name == "") throw std::runtime_error("Name cannot be empty");
				names.push_back(name);
				target.push_back(GeneralSeqAlignment());
				trgPos = 0;
				error = false;
			} else {
				if (error) continue;
				if (target.empty()) throw std::runtime_error("First sequence without name!");
				if ( transcripts && (target.back().elements.size() > 1) && (target.back().elements.back().unalignedSeq != "") ) {
					std::cerr << "WARNING(" << name << "): Transcripts can have unaligned segments only at the beginning or at the end (poly(A))!!! (ignore the rest of transcript)" << std::endl;
					error = true;
					continue;
				}
				std::vector<std::string> words = split_with_single_space(line,5);
				GeneralSeqAlignment::Element e;
				e.unalignedSeq = words[0];
				convertToUpperCase(e.unalignedSeq);
				trgPos += e.unalignedSeq.size();
				std::string const refName = words[1];
				std::string const refStrand = words[2];
				unsigned const offset = boost::lexical_cast<unsigned>(words[3]);
				std::string const cigar = words[4];
				for (unsigned i = 5; i < words.size(); ++i) words[i-5] = words[i];
				words.resize(words.size()-5);
				for (std::string & w: words) convertToUpperCase(w);
				if (refName != "") {
					if ( transcripts && e.unalignedSeq != "" && ! target.back().elements.empty() ) {
						std::cerr << "WARNING(" << name << "): Transcripts can have unaligned segments only at the beginning or at the end (poly(A))!!! (ignore the rest of transcript)" << std::endl;
						error = true;
						continue;
					}
					e.alignment.set( cigar, ReferenceId(name2refSeq.at(refName)), (refStrand == "-"), offset, trgPos, words );
					trgPos += e.alignment.targetLength();
					if (transcripts && target.back().elements.size() > 0) {
						Alignment const & prevAlign = target.back().elements.back().alignment;
						if ( e.alignment.sourceRefId != prevAlign.sourceRefId || e.alignment.sourceStrandNegativ != prevAlign.sourceStrandNegativ ) {
							std::cerr << "WARNING(" << name << "): Transcripts cannot be aligned to two different references or opposite strands!!!  (ignore the rest of transcript)" << std::endl;
							error = true;
							continue;
						}
						if ( prevAlign.targetRegion().right() != e.alignment.targetRegion().left() ) {
							throw std::runtime_error("alignments must create continuous region in transcript space.");
						}
						if ( ! e.alignment.sourceStrandNegativ ) {
							if ( prevAlign.sourceRegion().right() >= e.alignment.sourceRegion().left() ) {
								throw std::runtime_error("alignments must be in correct order in genome space (1).");
							}
						} else {
							if ( e.alignment.sourceRegion().right() >= prevAlign.sourceRegion().left() ) {
								throw std::runtime_error("alignments must be in correct order in genome space (2).");
							}
						}
					}
				}
				target.back().elements.push_back(e);
			}
		} catch (std::exception const & e) {
			throw std::runtime_error("Error when reading from the following file: " + filename + ", line: " + boost::lexical_cast<std::string>(lineNo) + ", message: " + e.what());
		}
	}
}

inline std::vector<std::string> parseList(std::string v)
{
	std::vector<std::string> r;
	boost::trim(v);
	if (v == "") return r;
	boost::split(r,v,boost::is_any_of(","));
	for (auto & s: r) {
		boost::trim(s);
		if (s == "") throw std::runtime_error("Incorrect list: " + v);
	}
	return r;
}

void ReferencesDatabase::Pim::readGenes( std::string const & filename )
{
	LogScopeWallTime scopeLog("Parse " + filename);
	unsigned const colIdHgncId          = 0;
	unsigned const colIdHgncSymbol      = 1;
	unsigned const colIdHgncName        = 2;
	unsigned const colIdPreviousSymbols = 3;
	unsigned const colIdSynonyms        = 4;
	unsigned const colIdEntrezId        = 5;
	unsigned const colIdEnsemblId       = 6;
	std::vector<std::string> colsNames(7);
	colsNames[colIdHgncId] = "HGNC ID";
	colsNames[colIdHgncSymbol] = "Approved Symbol";
	colsNames[colIdHgncName] = "Approved Name";
	colsNames[colIdPreviousSymbols] = "Previous Symbols";
	colsNames[colIdSynonyms] = "Synonyms";
	colsNames[colIdEntrezId] = "Entrez Gene ID";
	colsNames[colIdEnsemblId] = "Ensembl Gene ID";
	TabSeparatedFile file(filename, colsNames);
	std::string const withdrawnSuffix = "~withdrawn";
	for ( std::vector<std::string> r;  file.readRecord(r); ) {
		unsigned const hgncId = boost::lexical_cast<unsigned>(r[colIdHgncId]);
		if (genes.size() <= hgncId) genes.resize(hgncId+1);
		genes[hgncId].hgncId = hgncId;
		genes[hgncId].hgncSymbol = r[colIdHgncSymbol];
		std::string::size_type const t = genes[hgncId].hgncSymbol.find("~withdrawn");
		if (t == std::string::npos) {
			genes[hgncId].active = true;
			genes[hgncId].ensemblId = r[colIdEnsemblId];
			genes[hgncId].hgncName = r[colIdHgncName];
			if (r[colIdEntrezId] != "") genes[hgncId].refSeqId = boost::lexical_cast<unsigned>(r[colIdEntrezId]);
			genes[hgncId].otherSymbols = parseList(r[colIdSynonyms]);
			genes[hgncId].obsoleteSymbols = parseList(r[colIdPreviousSymbols]);
		} else {
			genes[hgncId].active = false;
			genes[hgncId].hgncSymbol = genes[hgncId].hgncSymbol.substr(0,t);
		}
	}
	// ========== fill genesByNames
	unsigned number = 0;
	for (Gene const & g: genes) {
		if (! g.active) continue;
		++number;
		std::vector<std::string> v = g.obsoleteSymbols;
		v.insert(v.end(), g.otherSymbols.begin(), g.otherSymbols.end());
		if (g.ensemblId  != "") v.push_back(g.ensemblId);
		if (g.hgncName   != "") v.push_back(g.hgncName);
		if (g.hgncSymbol != "") v.push_back(g.hgncSymbol);
		if (g.refSeqId   != 0 ) v.push_back(boost::lexical_cast<std::string>(g.refSeqId));
		for (std::string & s: v) boost::trim(s);
		std::sort(v.begin(),v.end());
		v.erase( std::unique(v.begin(),v.end()), v.end() );
		for (std::string & s: v) genesBySymbol[s].push_back(g.hgncId);
	}
}

void ReferencesDatabase::Pim::readNames( std::string const & filename )
{
	LogScopeWallTime scopeLog("Parse " + filename);
	unsigned const colIdRsId    = 0;
	unsigned const colIdRefSeq  = 1;
	unsigned const colIdGenBank = 2;
	unsigned const colIdEnsembl = 3;
	unsigned const colIdLRG     = 4;
	std::vector<std::string> cols(5);
	cols[colIdRsId   ] = "Id";
	cols[colIdRefSeq ] = "RefSeq";
	cols[colIdGenBank] = "GenBank";
	cols[colIdEnsembl] = "ENSEMBL";
	cols[colIdLRG    ] = "LRG";
	TabSeparatedFile file(filename, cols);
	for ( std::vector<std::string> r;  file.readRecord(r);  ) {
		std::string rs = r[colIdRsId];
		while (rs.size() < 6) rs = "0" + rs;
		rs = "RS" + rs;
		std::vector<std::string> v;
		v.push_back(rs);
		for (unsigned i = 0; i < r.size(); ++i) {
			if (i == colIdRsId || r[i] == "") continue;
			v.push_back(r[i]);
		}
		ReferenceId refId;
		for (auto const & s: v) {
			if (name2refSeq.count(s)) {
				if (refId != ReferenceId::null) {
					throw std::runtime_error("ERROR: More than one sequence match to " + rs);
				}
				refId = ReferenceId(name2refSeq[s]);
			}
		}
		if (refId == ReferenceId::null) {
		//	std::cerr << "WARNING: There is no matching sequence for " << rs << "\n"; TODO
		} else {
			for (auto const & s: v) {
				if (name2refSeq.count(s)) continue;
				name2refSeq[s] = refId.value;
				names[refId.value].push_back(s);
			}
		}
	}
}

void ReferencesDatabase::Pim::readTranscriptMetadata( std::string const & filename )
{
	LogScopeWallTime scopeLog("Parse " + filename);
	metadata.resize(names.size());
	unsigned const colIdReference = 0;
	unsigned const colIdGene      = 1;
	unsigned const colIdCdsLeft   = 2;
	unsigned const colIdCdsRight  = 3;
	unsigned const colIdProtein   = 4;
	unsigned const colIdFrameshift = 5;
	std::vector<std::string> cols(6);
	cols[colIdReference] = "Reference";
	cols[colIdGene     ] = "Gene";
	cols[colIdCdsLeft  ] = "CDSleft";
	cols[colIdCdsRight ] = "CDSright";
	cols[colIdProtein  ] = "Protein";
	cols[colIdFrameshift]= "Frameshift";
	TabSeparatedFile file(filename, cols);
	unsigned unknownReferences = 0;
	unsigned unknownGenes = 0;
	for ( std::vector<std::string> r;  file.readRecord(r);  ) {
		std::string ref = r[colIdReference];
		if (name2refSeq.count(ref) == 0) {
			// std::cerr << "WARNING: Unknown reference " << ref << std::endl;
			++unknownReferences;
			continue;
		}
		unsigned const refId = name2refSeq[ref];
		if (r[colIdGene] != "") {
			if (r[colIdGene].substr(0,4) == "ENSG") r[colIdGene] = r[colIdGene].substr(0, r[colIdGene].find('.')); // cut version from ENSEMBLE genes
			unsigned geneId = std::numeric_limits<unsigned>::max();
			for (unsigned id: genesBySymbol[r[colIdGene]]) {
				if (boost::lexical_cast<std::string>(genes[id].refSeqId) == r[colIdGene]) {
					if (geneId < genes.size()) throw std::runtime_error("Not uniquely defined gene: " + r[colIdGene]);
					geneId = id;
				}
			}
			if (geneId < genes.size()) {
				metadata[refId].geneId = geneId;
			} else {
				//std::cerr << "WARNING: Unknown gene: " << r[colIdGene] << std::endl;
				++unknownGenes;
			}
		}
		if (r[colIdCdsLeft] != "" || r[colIdCdsRight] != "") {
			if (r[colIdCdsLeft] == "" || r[colIdCdsRight] == "") {
				throw std::runtime_error("Incomplete CDS definition for " + ref);
			}
			unsigned const left = boost::lexical_cast<unsigned>(r[colIdCdsLeft]);
			unsigned const right = boost::lexical_cast<unsigned>(r[colIdCdsRight]);
			metadata[refId].CDS.set(left, right);
			if (r[colIdFrameshift] != "") metadata[refId].frameOffset = boost::lexical_cast<unsigned>(r[colIdFrameshift]); // in some partial transcript in ENSEMBL start codon is cut
			if (r[colIdProtein] != "") {
				std::string const & name = r[colIdProtein];
				if (name2refSeq.count(name) == 0) {
					proteins2transcripts.push_back(ReferenceId(refId));
					metadata[refId].proteinId = names.size();
					name2refSeq[name] = names.size();
					names.push_back({name});
					ReferenceMetadata rm;
					rm.geneId = metadata[refId].geneId;
					metadata.push_back(rm);
				} else {
					std::cerr << "WARNING: Duplicated protein: " << name << "\n";
				}
			}
		}
	}
	if (unknownReferences) std::cerr << "WARNING: " << unknownReferences << " unknown references\n";
	if (unknownGenes) std::cerr << "WARNING: " << unknownGenes << " unknown genes\n";
}

void ReferencesDatabase::Pim::unspliceTranscripts()
{
	LogScopeWallTime scopeLog("Unsplice transcripts");
	transcriptsExons.clear();
	unsigned refId = offsetTranscripts;
	for (GeneralSeqAlignment & gsa: transcripts) {
		GeneralSeqAlignment::Element e = gsa.elements.at(0);
		std::string polyA = "";
		if (e.alignment.sourceRefId != ReferenceId::null) {
			transcriptsExons.push_back( { std::make_pair(e.alignment.targetRegion(),e.alignment.targetRegion()) } );
		} else {
			// special case - transcript not aligned (consists only of sequence)
			transcriptsExons.push_back( { std::make_pair(RegionCoordinates(0,e.unalignedSeq.size()),RegionCoordinates(0,e.unalignedSeq.size())) } );
		}
		for (unsigned i = 1; i < gsa.elements.size(); ++i) {
			GeneralSeqAlignment::Element const & e2 = gsa.elements[i];
			if (e2.alignment.sourceRefId == ReferenceId::null) {
				polyA = e2.unalignedSeq;
				break;
			}
			RegionCoordinates intronRegion;
			if (e.alignment.sourceStrandNegativ) {
				intronRegion.set(e2.alignment.sourceRegion().right(),e.alignment.sourceRegion().left());
			} else {
				intronRegion.set(e.alignment.sourceRegion().right(),e2.alignment.sourceRegion().left());
			}
			// ----- add intron
			Alignment a = Alignment::createIdentityAlignment( e.alignment.sourceRefId, e.alignment.sourceStrandNegativ
						, intronRegion.left()                // source left position
						, e.alignment.targetRegion().right() // target left position
						, intronRegion.length()              // length of alignment (the same for source & target)
						);
			e.alignment.append(a);
			// ----- add exon
			a = e2.alignment;
			a.targetLeftPosition = transcriptsExons.back().back().first.right() + intronRegion.length();
			e.alignment.append(a);
			// ----- save exon coordinates
			transcriptsExons.back().push_back(std::make_pair(a.targetRegion(),e2.alignment.targetRegion()));
		}
		gsa.elements = {e};
		if (polyA != "") {
			GeneralSeqAlignment::Element e2;
			e2.unalignedSeq = polyA;
			gsa.elements.push_back(e2);
		}
		++refId;
	}
}

void ReferencesDatabase::Pim::buildAlignmentsToMainGenome()
{
	LogScopeWallTime scopeLog("Build alignments to main genome");
	// ------ create empty vector with alignments
	alignmentsToMainGenome.clear();
	alignmentsToMainGenome.resize( mainGenome.size() );
	// ------ copy mapped reference to alignments
	for (unsigned id = 0; id < mappedReferences.size(); ++id) {
		GeneralSeqAlignment const & ga = mappedReferences[id];
		for (auto const & gae: ga.elements) {
			ReferenceId const mainRefId = gae.alignment.sourceRefId;
			if (mainRefId == ReferenceId::null) continue;
			RegionCoordinates const mainRegRegion = gae.alignment.sourceRegion();
			StringPointer srcSeq = mainGenome[mainRefId.value].substr(mainRegRegion.left(), mainRegRegion.length());
			Alignment const revAlignment = gae.alignment.reverse( srcSeq, ReferenceId(offsetMappedReferences+id) );
			alignmentsToMainGenome[mainRefId.value].push_back( RegionIndexElement(revAlignment) );
		}
	}
	// ------ copy transcripts to alignments
	for (unsigned id = 0; id < transcripts.size(); ++id) {
		GeneralSeqAlignment const & ga = transcripts[id];
		for (auto const & gae: ga.elements) {
			ReferenceId const mainRefId = gae.alignment.sourceRefId;
			if (mainRefId == ReferenceId::null) continue;
			RegionCoordinates const mainRegRegion = gae.alignment.sourceRegion();
			StringPointer const srcSeq = mainGenome[mainRefId.value].substr(mainRegRegion.left(), mainRegRegion.length());
			Alignment const revAlignment = gae.alignment.reverse( srcSeq, ReferenceId(offsetTranscripts+id) );
			alignmentsToMainGenome[mainRefId.value].push_back( RegionIndexElement(revAlignment) );
		}
	}
	// ----- organize created elements
	for (auto & v: alignmentsToMainGenome) {
		std::sort(v.begin(), v.end());
		unsigned maxEnd = 0;
		for (RegionIndexElement & e: v) {
			maxEnd = std::max(maxEnd, e.targetEnd);
			e.maxTargetEnd = maxEnd;
		}
	}
}

GeneralSeqAlignment ReferencesDatabase::Pim::calculateGeneralAlignment
						(GeneralSeqAlignment const & reference, RegionCoordinates const & region, bool exactTargetEdges) const
{
	GeneralSeqAlignment result;
	std::vector<GeneralSeqAlignment::Element>::const_iterator iE = reference.elements.begin();
	std::vector<GeneralSeqAlignment::Element>::const_iterator iE_end = reference.elements.end();
	// ------------ quick fix - index element if not indexed
	if (iE != iE_end && iE->targetOffset == std::numeric_limits<unsigned>::max()) {
		unsigned offset = 0;
		for (; iE < iE_end; ++iE) {
			GeneralSeqAlignment::Element const & e = *iE;
			e.targetOffset = offset;
			offset += e.unalignedSeq.size() + e.alignment.targetLength();
		}
		iE = reference.elements.begin();
	}
	// ------------ ommit the elements from the left side of the region
	auto comp = [](GeneralSeqAlignment::Element const & e1, GeneralSeqAlignment::Element const & e2)->bool { return (e1.targetOffset < e2.targetOffset); };
	GeneralSeqAlignment::Element tempE;
	tempE.targetOffset = region.left();
	if ( exactTargetEdges ) {
		iE = std::upper_bound(iE, iE_end, tempE, comp );
	} else {
		iE = std::lower_bound(iE, iE_end, tempE, comp );
	}
	if (iE != reference.elements.begin()) --iE;
	unsigned offset = iE->targetOffset;
	// ------------ copy the elements, calculate left and right margin to cut
	unsigned const leftMargin = region.left() - offset;
	for (; iE < iE_end; ++iE) {
		GeneralSeqAlignment::Element const & e = *iE;
		result.elements.push_back(e);
		offset += e.unalignedSeq.size() + e.alignment.targetLength();
		if (offset >= region.right()) break;
	}
	if (iE == iE_end) {
		// special case for empty region at the end
		if (region.length() == 0 && region.left() == reference.elements.back().alignment.targetRegion().right()) {
			result.elements.push_back(reference.elements.back());
			result.elements.back().unalignedSeq = "";
			result.elements.back().alignment = result.elements.back().alignment.targetSubalign(result.elements.back().alignment.targetLength());
			return result;
		} else {
			throw std::runtime_error("region out of range");
		}
	}
	unsigned const rightMargin = offset - region.right();
	if (rightMargin > 0) { // ----------- remove right margin (we need unchanged e.alignment for the if)
		GeneralSeqAlignment::Element & e = result.elements.back();
		if (rightMargin > e.alignment.targetLength()) {
			e.unalignedSeq = e.unalignedSeq.substr( 0, e.unalignedSeq.size()-(rightMargin-e.alignment.targetLength()) );
			e.alignment = Alignment();
		} else {
			e.alignment = e.alignment.targetSubalign( region.left(), region.length(), exactTargetEdges );
		}
	}
	{ // ----------- remove left margin
		GeneralSeqAlignment::Element & e = result.elements.front();
		if (leftMargin > e.unalignedSeq.size()) {
			e.alignment = e.alignment.targetSubalign( region.left(), region.length(), exactTargetEdges );
			e.unalignedSeq = "";
		} else {
			e.unalignedSeq = e.unalignedSeq.substr(leftMargin);
		}
	}
	return result;
}

unsigned ReferencesDatabase::hgncSymbolToGeneId(std::string const & geneName) const
{
	if (pim->genesBySymbol.count(geneName) == 0) throw std::runtime_error("Gene is not known: " + geneName);
	std::vector<unsigned> const & geneIds = pim->genesBySymbol.at(geneName);
	for (unsigned geneId: geneIds) {
		if (pim->genes.at(geneId).hgncSymbol == geneName) return geneId;
	}
	throw std::runtime_error("Cannot match any HGNC symbol: " + geneName);
}

ReferencesDatabase::ReferencesDatabase(std::string const & pPath) : pim(new Pim)
{
	std::map< std::string, std::pair<std::string,std::pair<unsigned,unsigned>> > filename2genomeBuilds; // fullPath -> (buildName,refId_range)
	// search for files
	std::string fileMainGenome = "";
	std::vector<std::string> filesGenomes;
	std::vector<std::string> filesGenes;
	std::vector<std::string> filesTranscripts;
	std::vector<std::string> filesProteins;
	boost::filesystem::path path(pPath);
	if ( ! boost::filesystem::exists(path) ) throw std::runtime_error("Directory does not exists: " + pPath);
	if ( ! boost::filesystem::is_directory(path) ) throw std::runtime_error("It is not a directory: " + pPath);
	path = boost::filesystem::canonical(path); // convert to absolute path
	for ( boost::filesystem::directory_entry & x : boost::make_iterator_range(boost::filesystem::directory_iterator(path),boost::filesystem::directory_iterator()) ) {
		std::string const ext = x.path().extension().string();
		std::string const basename = x.path().stem().string();
		if (ext == ".fasta") {
			if (basename.substr(0,5) == "main_") {
				if (fileMainGenome != "") throw std::runtime_error("There is more than one file matching main_*.fasta pattern!");
				fileMainGenome = x.path().string();
				filename2genomeBuilds[fileMainGenome].first = basename.substr(5);
			}
			if (basename.substr(0,9) == "proteins_") filesProteins.push_back(x.path().string());
		}
		if (ext == ".align") {
			if (basename.substr(0,6) == "build_") {
				filesGenomes.push_back(x.path().string());
				filename2genomeBuilds[x.path().string()].first = basename.substr(6);
			}
			if (basename.substr(0,6) == "genes_") filesGenes.push_back(x.path().string());
			if (basename.substr(0,6) == "trans_") filesTranscripts.push_back(x.path().string());
		}
	}
	if (fileMainGenome == "") throw std::runtime_error("There is no file matching main_*.fasta pattern!");

	// read sequence data
	std::vector<std::string> names;
	pim->readFromFile(fileMainGenome, names, pim->mainGenome);
	filename2genomeBuilds[fileMainGenome].second = std::make_pair(0,names.size());
	// generate index with names
	for (unsigned i = 0; i < names.size(); ++i) {
		pim->name2refSeq[names[i]] = i;
		pim->names.push_back({names[i]});
		if (pim->names.size() != pim->name2refSeq.size()) throw std::runtime_error("Repeated name: " + names[i] + " !!!");
	}

	pim->offsetMappedReferences = names.size();
	for (std::string const & f: filesGenomes) {
		filename2genomeBuilds[f].second.first = names.size();
		pim->readFromFile(f, names, pim->mappedReferences, false);
		filename2genomeBuilds[f].second.second = names.size();
	}
	for (std::string const & f: filesGenes  ) pim->readFromFile(f, names, pim->mappedReferences, false);
	pim->offsetTranscripts = names.size();
	for (std::string const & f: filesTranscripts) pim->readFromFile(f, names, pim->transcripts, true);
	pim->offsetProteins = names.size();

	// generate index with names
	for (unsigned i = pim->offsetMappedReferences; i < names.size(); ++i) {
		pim->name2refSeq[names[i]] = i;
		pim->names.push_back({names[i]});
		if (pim->names.size() != pim->name2refSeq.size()) throw std::runtime_error("Repeated name: " + names[i] + " !!!");
	}

	// read metadata
	pim->readGenes( (path / "hgnc.txt").string() );
	pim->readTranscriptMetadata( (path / "metadata_transcripts.txt").string() );
	pim->readNames( (path / "refs.txt").string() );

	// preprocess data
	pim->unspliceTranscripts();
	pim->buildAlignmentsToMainGenome();

	{ // calculate lengths
		LogScopeWallTime scopeLog("Calculate sequences lengths");
		for (unsigned i = 0; i < pim->mainGenome.size(); ++i) {
			pim->metadata[pim->offsetMainGenome+i].length = pim->mainGenome[i].size();
			pim->metadata[pim->offsetMainGenome+i].splicedLength = pim->mainGenome[i].size();
		}
		for (unsigned i = 0; i < pim->mappedReferences.size(); ++i) {
			for ( auto const & e : pim->mappedReferences[i].elements ) {
				pim->metadata[pim->offsetMappedReferences+i].length += e.unalignedSeq.size() + e.alignment.targetLength();
			}
			pim->metadata[pim->offsetMappedReferences+i].splicedLength = pim->metadata[pim->offsetMappedReferences+i].length;
		}
		for (unsigned i = 0; i < pim->transcripts.size(); ++i) {
			for ( auto const & e : pim->transcripts[i].elements ) {
				pim->metadata[pim->offsetTranscripts+i].length += e.unalignedSeq.size() + e.alignment.targetLength();
			}
			for ( auto const & e: pim->transcriptsExons[i] ) {
				pim->metadata[pim->offsetTranscripts+i].splicedLength += e.second.length();
			}
		}
		for (unsigned i = 0; i < pim->proteins2transcripts.size(); ++i) {
			unsigned length = (pim->metadata[pim->proteins2transcripts[i].value].CDS.length() - pim->metadata[pim->proteins2transcripts[i].value].frameOffset);
			length = (length + 2) / 3;
			if (pim->metadata[pim->proteins2transcripts[i].value].frameOffset) ++length;
			pim->metadata[pim->offsetProteins+i].length = length;
			pim->metadata[pim->offsetProteins+i].splicedLength = length;
		}
	}

	{ // fill genome build and chromosome fields
		for (auto const & kv: filename2genomeBuilds) {
			std::string const build = kv.second.first;
			for (unsigned i = kv.second.second.first; i < kv.second.second.second; ++i) {
				std::string nc_name = "";
				for (auto const & n: pim->names[i]) {
					if (n.substr(0,7) == "NC_0000") {
						nc_name = n;
					} else if (n == "NC_012920.1") {
						pim->metadata[i].genomeBuild = build;
						pim->metadata[i].chromosome = chrM;
						break;
					} else {
						if (pim->metadata[i].genomeBuild != "GRCh38") pim->metadata[i].genomeBuild = build;
					}
				}
				if (nc_name == "") continue;
				pim->metadata[i].genomeBuild = build;
				pim->metadata[i].chromosome = static_cast<Chromosome>(boost::lexical_cast<unsigned>(nc_name.substr(7,2)));
			}
		}
	}

	// calculate assignments between references and genes
	for (unsigned refId = 0; refId < pim->metadata.size(); ++refId) {
		ReferenceMetadata const & rm = pim->metadata[refId];
		if (rm.geneId > 0) pim->genes[rm.geneId].assignedReferences.push_back(ReferenceId(refId));
	}

	// calculate protein accession identifiers
	for ( unsigned refId = pim->offsetProteins;  refId < pim->names.size();  ++refId ) {
		std::string name = "";
		for (std::string const & n: pim->names[refId]) {
			std::string x = n.substr(0,4);
			if (x != "ENSP") x.pop_back();
			if ( x == "ENSP" || x == "NP_" || x == "XP_" || x == "AP_" ) {
				if (name != "") throw std::runtime_error("There is more than one name for protein: " + name + ", " + n);
				name = n;
			}
		}
		ASSERT( ! pim->names[refId].empty() );
		if (name == "") throw std::runtime_error("There is no name for protein: " + pim->names[refId].front());
		pim->metadata[refId].proteinAccessionIdentifier = calculateProteinAccessionIdentifier(name);
		pim->proteinAccessionIdentifier2referenceId[pim->metadata[refId].proteinAccessionIdentifier].value = refId;
	}
}

ReferencesDatabase::~ReferencesDatabase()
{
// we want to leave the stuff in the memory
//	if (pim->sharedMemory             != nullptr) pim->sharedMemory->remove("genomeIndex_sharedMemory");
//	if (pim->semaphoreForSharedMemory != nullptr) pim->semaphoreForSharedMemory->remove("genomeIndex_semaphore");
//	if (pim->mutexForSharedMemory     != nullptr) pim->mutexForSharedMemory->remove("genomeIndex_mutex");
	delete pim->mappedMemory;
	delete pim->sharedMemory;
	delete pim->semaphoreForSharedMemory;
	delete pim->mutexForSharedMemory;
	delete pim;
}

bool ReferencesDatabase::isSplicedRefSeq(ReferenceId refId) const
{
	return (refId.value >= pim->offsetTranscripts && refId.value < pim->offsetProteins);
}

bool ReferencesDatabase::isProteinReference(ReferenceId refId) const
{
	return (refId.value >= pim->offsetProteins);
}

// Returns refSeq ID associated with given name
ReferenceId ReferencesDatabase::getReferenceId(std::string const & name) const
{
	std::string name2 = "";
	// add leading zeroes for RS number
	if (name.substr(0,2) == "RS" && name.size() > 2 && name.size() < 8) {
		for (unsigned i = 2; i < name.size(); ++i) {
			if (name[i] < '0' || name[i] > '9') {
				name2 = name;
				break;
			}
		}
		if (name2 == "") {
			name2 = "RS" + std::string(8-name.size(), '0') + name.substr(2);
		}
	} else {
		name2 = name;
	}
	// main stuff
	auto const it = pim->name2refSeq.find(name2);
	if (it == pim->name2refSeq.end()) throw std::runtime_error("Unknown reference: " + name);
	return ReferenceId(it->second);
}

ReferenceId ReferencesDatabase::getReferenceId(ReferenceGenome refGenome, Chromosome chr) const
{
	for ( unsigned i = 0; i < pim->offsetTranscripts; ++i ) {
		if (pim->metadata[i].chromosome == chr) {
			if (pim->metadata[i].genomeBuild == toString(refGenome) || chr == Chromosome::chrM) return ReferenceId(i);
		}
	}
	throw std::runtime_error("Unknown combination of reference genome and chromosome");
}

// returns RS id
unsigned ReferencesDatabase::getCarRsId(ReferenceId refId) const
{
	std::vector<std::string> const refNames = this->getNames(refId);
	for (std::string const & n: refNames) {
		if (n.substr(0,2) == "RS" && n.size() > 2) {
			std::string const number = n.substr(2);
			unsigned value = 0;
			bool found = true;
			for (char c: number) {
				if (c < '0' || c > '9') {
					found = false;
					break;
				}
				value *= 10;
				value += (c - '0');
			}
			if (found) return value;
		}
	}
	throw std::runtime_error("This sequence does not have CAR RS id assigned");
}

// Returns names associated with this refSeq
std::vector<std::string> ReferencesDatabase::getNames(ReferenceId refId) const
{
	if (refId.value >= pim->names.size()) throw std::logic_error("There is no reference with given ID: " + boost::lexical_cast<std::string>(refId.value));
	return pim->names[refId.value];
}

// Returns CDS associated with given refId (it must be transcript)
RegionCoordinates ReferencesDatabase::getCDS(ReferenceId refId) const
{
	if (refId.value >= pim->metadata.size()) throw std::logic_error("There is no reference with given ID: " + boost::lexical_cast<std::string>(refId.value));
	return pim->metadata[refId.value].CDS;
}

static bool positionFromExon(SplicedCoordinate const & position, RegionCoordinates const & exon, int & offsetFromLeftBorder)
{
	if (position > SplicedCoordinate(exon.right(), '+', std::numeric_limits<unsigned>::max())) return false;
	if (position < SplicedCoordinate(exon.left (), '-', std::numeric_limits<unsigned>::max())) return false;
	offsetFromLeftBorder = position.position() - exon.left();
	if (position.offsetSize()) {
		if (position.position() == exon.left()) {
			offsetFromLeftBorder -= position.offsetSize();
		} else if (position.position() == exon.right()) {
			offsetFromLeftBorder += position.offsetSize();
		} else {
			throw std::logic_error("ReferencesDatabase::convertToUnsplicedRegion() - intronic position inside exon");
		}
	}
	return true;
}

// convert spliced coordinates <-> unspliced coordinates
RegionCoordinates ReferencesDatabase::convertToUnsplicedRegion(ReferenceId refId, SplicedRegionCoordinates const & region) const
{
	if (refId.value < pim->offsetTranscripts || refId.value >= pim->offsetProteins) {
		throw std::logic_error("There is no transcript with given ID: " + boost::lexical_cast<std::string>(refId.value));
	}
	auto const & exons = pim->transcriptsExons.at(refId.value - pim->offsetTranscripts);
	// ----- calculate left
	auto iExon = exons.begin();
	int offsetFromLeft = 0;
	for (; iExon != exons.end(); ++iExon) {
		if ( positionFromExon(region.left(), iExon->second, offsetFromLeft) ) break;
	}
	if (iExon == exons.end()) throw std::runtime_error("ReferencesDatabase::convertToUnsplicedRegion() - coordinates outside reference (1)");
	unsigned left = iExon->first.left();
	if (offsetFromLeft < 0) left -= static_cast<unsigned>(-offsetFromLeft); else left += static_cast<unsigned>(offsetFromLeft);
	// ----- calculate right
	for (; iExon != exons.end(); ++iExon) {
		if ( positionFromExon(region.right(), iExon->second, offsetFromLeft) ) break;
	}
	if (iExon == exons.end()) throw std::runtime_error("ReferencesDatabase::convertToUnsplicedRegion() - coordinates outside reference (2)");
	unsigned right = iExon->first.left();
	if (offsetFromLeft < 0) right -= static_cast<unsigned>(-offsetFromLeft); else right += static_cast<unsigned>(offsetFromLeft);
	// ----------------------
	return RegionCoordinates(left, right);
}

SplicedRegionCoordinates ReferencesDatabase::convertToSplicedRegion(ReferenceId refId, RegionCoordinates const & region) const
{
	if (refId.value < pim->offsetTranscripts || refId.value >= pim->offsetProteins) {
		throw std::logic_error("There is no transcript with given ID: " + boost::lexical_cast<std::string>(refId.value));
	}
	auto const & exons = pim->transcriptsExons.at(refId.value - pim->offsetTranscripts);
	// ----- calculate left
	SplicedCoordinate left;
	auto iExon = exons.begin();
	for (; iExon != exons.end(); ++iExon) {
		// inside intron before the exon
		if (region.left() < iExon->first.left()) {
			auto iPrevExon = iExon;
			--iPrevExon;
			unsigned const negativeOffset = iExon->first.left() - region.left();
			unsigned const positiveOffset = region.left() - iPrevExon->first.right();
			if (positiveOffset < negativeOffset) {
				left.set(iPrevExon->second.right(), '+', positiveOffset);
			} else {
				left.set(iExon->second.left(), '-', negativeOffset);
			}
			break;
		}
		// inside the exon
		if (region.left() < iExon->first.right()) {
			left.set(iExon->second.left() + (region.left() - iExon->first.left()), '-', 0);
			break;
		}
		// check the next exon
	}
	if (iExon == exons.end()) throw ExceptionCoordinateOutsideReference();
	// ----- calculate right
	SplicedCoordinate right;
	for (; iExon != exons.end(); ++iExon) {
		// inside intron before the exon
		if (region.right() <= iExon->first.left()) {
			auto iPrevExon = iExon;
			--iPrevExon;
			unsigned const negativeOffset = iExon->first.left() - region.right();
			unsigned const positiveOffset = region.right() - iPrevExon->first.right();
			if (positiveOffset < negativeOffset) {
				right.set(iPrevExon->second.right(), '+', positiveOffset);
			} else {
				right.set(iExon->second.left(), '-', negativeOffset);
			}
			break;
		}
		// inside the exon
		if (region.right() <= iExon->first.right()) {
			right.set(iExon->second.left() + (region.right() - iExon->first.left()), '+', 0);
			break;
		}
		// check the next exon
	}
	if (iExon == exons.end()) throw ExceptionCoordinateOutsideReference();
	// ----------------------
	return SplicedRegionCoordinates(left, right);
}


// get exons
std::vector<std::pair<RegionCoordinates,SplicedRegionCoordinates>> ReferencesDatabase::getExons(ReferenceId refId) const
{
    if (refId.value < pim->offsetTranscripts || refId.value >= pim->offsetProteins) {
		throw std::logic_error("There is no transcript with given ID: " + boost::lexical_cast<std::string>(refId.value));
	}
	auto const & exons = pim->transcriptsExons.at(refId.value - pim->offsetTranscripts);
    std::vector<std::pair<RegionCoordinates,SplicedRegionCoordinates>> result;
    result.reserve(exons.size());
    for (auto const & e: exons) {
        result.push_back(std::make_pair(e.first,SplicedRegionCoordinates(e.second)));
    }
    return result;
}


std::string ReferencesDatabase::getSequence(ReferenceId refId, RegionCoordinates region) const
{
	if (refId.value < pim->offsetMappedReferences) {
		if (region.right() > pim->mainGenome[refId.value].size()) {
			throw std::runtime_error("ReferencesDatabase::getSequence - position outside the reference");
		}
		return pim->mainGenome[refId.value].substr(region.left(), region.length());
	}
	if (refId.value < pim->offsetTranscripts) {
		GeneralSeqAlignment const reference = pim->calculateGeneralAlignment(pim->mappedReferences[refId.value-pim->offsetMappedReferences],region,true);
		std::string seq = "";
		for ( auto const & e : reference.elements ) {
			seq += e.unalignedSeq;
			if (! e.alignment.sourceRefId.isNull()) {
				std::string srcSeq = getSequence( e.alignment.sourceRefId, e.alignment.sourceRegion() );
				seq += e.alignment.processSourceSubSequence( srcSeq );
			}
		}
		return seq;
	}
	if (refId.value < pim->offsetProteins) {
		GeneralSeqAlignment const reference = pim->calculateGeneralAlignment(pim->transcripts[refId.value-pim->offsetTranscripts],region,true);
		std::string seq = "";
		for ( auto const & e : reference.elements ) {
			seq += e.unalignedSeq;
			if (! e.alignment.sourceRefId.isNull()) {
				std::string srcSeq = getSequence( e.alignment.sourceRefId, e.alignment.sourceRegion() );
				seq += e.alignment.processSourceSubSequence( srcSeq );
			}
		}
		return seq;
	}
	if (refId.value < pim->names.size()) {
		if (region.right() > pim->metadata[refId.value].length) {
			throw std::runtime_error("ReferencesDatabase::getSequence - position outside the reference");
		}
		if (region.length() == 0) return "";
		unsigned const transId = pim->proteins2transcripts[refId.value - pim->offsetProteins].value;
		unsigned const cdsLeft = pim->metadata[transId].CDS.left();
		region.set( region.left()*3 + cdsLeft, region.right()*3 + cdsLeft );
		// in case of protein cut in the middle of AA
		if (pim->metadata[transId].frameOffset) {
			region.incLeftPosition(pim->metadata[transId].frameOffset);
			region.decRightPosition( 3-pim->metadata[transId].frameOffset );
		}
		if (region.right() > pim->metadata[transId].splicedLength) {
			if (region.left() >= pim->metadata[transId].splicedLength) return std::string(region.length()/3,'X'); // TODO - problem with transcripts with additional subsequences
			region.setRight(pim->metadata[transId].splicedLength);
		}
		// -------
		auto const & exons = pim->transcriptsExons[transId - pim->offsetTranscripts];
		auto it = exons.begin();
		while (it != exons.end() && it->second.right() <= region.left()) ++it;
		std::string seq = "";
		if (it != exons.end()) {
			unsigned const offset = (region.left() - it->second.left());
			seq += getSequence( ReferenceId(transId), RegionCoordinates(it->first.left()+offset,it->first.right()) );
//std::cout << "D1 seq=" << seq << std::endl;
			++it;
		}
		while (it != exons.end() && it->second.left() < region.right()) {
			seq += getSequence( ReferenceId(transId), it->first );
//std::cout << "D2 seq=" << seq << std::endl;
			++it;
		}
		--it;
		if (region.right() < it->second.right()) {
			seq.resize( seq.size() - (it->second.right()-region.right()) );
//std::cout << "D3 seq=" << seq << std::endl;
		}
		seq = translateToAminoAcid2(seq);
		if (pim->metadata[transId].frameOffset) seq = "X" + seq;
		if ( seq.size() > 0 && (seq.back() == '*' || seq.back() == 'X') ) seq.pop_back(); // trim the stop codon
		return seq;
	}
	throw std::logic_error("There is no reference with given ID: " + boost::lexical_cast<std::string>(refId.value));
}


std::string ReferencesDatabase::getSplicedSequence(ReferenceId refId, RegionCoordinates splicedRegion) const
{
	if (refId.value < pim->offsetTranscripts || refId.value >= pim->offsetProteins) {
		throw std::logic_error("There is no transcript with given ID: " + boost::lexical_cast<std::string>(refId.value));
	}
	auto const & exons = pim->transcriptsExons.at(refId.value - pim->offsetTranscripts);
	// ----- calculate left
	auto iExon = exons.begin();
	for (; iExon != exons.end(); ++iExon) {
		if ( iExon->second.right() > splicedRegion.left() ) break;
	}
	if (iExon == exons.end()) throw std::runtime_error("ReferencesDatabase::getSplicedSequence() - coordinates outside reference");
	unsigned length = iExon->second.right() - splicedRegion.left();
	ASSERT(iExon->first.length() >= length);
	std::string seq = getSequence(refId, RegionCoordinates(iExon->first.right()-length,iExon->first.right()));
	// ----- add all exons in the middle
	while ( iExon != exons.end() ) {
		if (iExon->second.right() >= splicedRegion.right()) break;
		++iExon;
		seq += getSequence(refId, iExon->first);
	}
	// ----- add part of the exom from the right side
	if (iExon != exons.end()) {
		length = iExon->second.right() - splicedRegion.right();
		seq.resize(seq.size()-length);
	}
	// ----- return sequence
	return seq;
}

GeneralSeqAlignment ReferencesDatabase::getAlignmentFromMainGenome(ReferenceId refId, RegionCoordinates region) const
{
	if (refId.value < pim->offsetMappedReferences) {
		if (region.right() > pim->mainGenome[refId.value].size()) {
			throw std::runtime_error("ReferencesDatabase::getAlignmentFromMainGenome - position outside the reference");
		}
		return GeneralSeqAlignment::createIdentityAlignment( refId, region );
	}
	if (refId.value < pim->offsetTranscripts) {
		return pim->calculateGeneralAlignment( pim->mappedReferences[refId.value-pim->offsetMappedReferences], region );
	}
	if (refId.value < pim->offsetProteins) {
		return pim->calculateGeneralAlignment( pim->transcripts[refId.value-pim->offsetTranscripts], region );
	}
	if (refId.value < pim->names.size()) {
		if (region.right() > pim->metadata[refId.value].length) {
			throw std::runtime_error("ReferencesDatabase::getSequence - position outside the reference");
		}
		throw std::logic_error("There is no alignments for protein!!!");
		//return GeneralSeqAlignment::createIdentityAlignment( refId, region );
	}
	throw std::logic_error("There is no reference with given ID: " + boost::lexical_cast<std::string>(refId.value));
}


std::vector<GeneralSeqAlignment> ReferencesDatabase::getAlignmentsToMainGenome(ReferenceId refId, RegionCoordinates region) const
{
	if (refId.value >= pim->alignmentsToMainGenome.size()) {
		throw std::runtime_error("getAlignmentsToMainGenome(...) - ref id is not from main genome");
	}
	auto const & mainRefAlignments = pim->alignmentsToMainGenome[refId.value];

	// ---- find all matching alignments
	std::map<ReferenceId, std::vector<Alignment>> matchedAlignments;
	// find iterator to the first element to check
	auto lessMaxTargetEnd = [](unsigned value, Pim::RegionIndexElement const & r) -> bool { return (value < r.maxTargetEnd); };
	auto it = std::upper_bound( mainRefAlignments.begin(), mainRefAlignments.end(), region.left(), lessMaxTargetEnd );
	// go through the rest of the elements, until we go outside the region
	for ( ;  it != mainRefAlignments.end();  ++it) {
		if ( it->targetEnd <= region.left() ) continue;
		if ( it->alignment.targetLeftPosition >= region.right() ) break;
		Alignment const align = it->alignment.targetSubalign(region.left(), region.length()); // TODO - something like largest possible subalign ???
		if (align.targetRegion().right() <= region.left() || align.targetRegion().left() >= region.right() ) continue;
		matchedAlignments[it->alignment.sourceRefId].push_back( align );
	}

	// ---- build full alignments
	std::vector<GeneralSeqAlignment> results;
	for (auto const e: matchedAlignments) {
		GeneralSeqAlignment ga;
		unsigned pos = region.left();
		for (auto const a: e.second) {
			GeneralSeqAlignment::Element ne;
			ne.alignment = a;
			if (a.targetLeftPosition > pos) ne.unalignedSeq = getSequence( refId, RegionCoordinates(pos,a.targetLeftPosition) );
			ga.elements.push_back(ne);
			pos = a.targetRegion().right();
		}
		if (pos < region.right()) {
			GeneralSeqAlignment::Element ne;
			ne.unalignedSeq = getSequence( refId, RegionCoordinates(pos, region.right()) );
			ga.elements.push_back(ne);
		}
		results.push_back(ga);
	}

	return results;
}

// Return sequence length (unspliced)
unsigned ReferencesDatabase::getSequenceLength(ReferenceId refId) const
{
	if (refId.value >= pim->metadata.size()) throw std::logic_error("There is no reference with given ID: " + boost::lexical_cast<std::string>(refId.value));
	return pim->metadata[refId.value].length;
}

// Return metadata
ReferenceMetadata const & ReferencesDatabase::getMetadata(ReferenceId refId) const
{
	if (refId.value >= pim->metadata.size()) throw std::logic_error("There is no reference with given ID: " + boost::lexical_cast<std::string>(refId.value));
	return pim->metadata[refId.value];
}

// Return gene region
std::map<ReferenceId,std::vector<RegionCoordinates>> ReferencesDatabase::getGeneRegions(std::string const & hgncGeneSymbol) const
{
	unsigned const geneId = hgncSymbolToGeneId(hgncGeneSymbol);
	std::map<ReferenceId,std::vector<RegionCoordinates>> regions;
	for (ReferenceId refId: pim->genes[geneId].assignedReferences) {
		if (refId.value < pim->offsetMappedReferences) {
			regions[refId].push_back(RegionCoordinates(0,this->getSequenceLength(refId)));
		} else if (refId.value < pim->offsetTranscripts) {
			GeneralSeqAlignment const & gsa = pim->mappedReferences[refId.value-pim->offsetMappedReferences];
			for (auto const & e : gsa.elements) { regions[e.alignment.sourceRefId].push_back(e.alignment.sourceRegion()); }
		} else if (refId.value < pim->offsetProteins) {
			GeneralSeqAlignment const & gsa = pim->transcripts[refId.value-pim->offsetTranscripts];
			for (auto const & e : gsa.elements) { regions[e.alignment.sourceRefId].push_back(e.alignment.sourceRegion()); }
		} else if (refId.value < pim->names.size()) {
			regions[refId].push_back(RegionCoordinates(0,this->getSequenceLength(refId)));
		} else {
			throw std::logic_error("There is no reference with given ID: " + boost::lexical_cast<std::string>(refId.value));
		}
	}
	regions.erase(ReferenceId::null);
	for (auto & e: regions) {
		if (e.second.empty()) continue;
		std::sort(e.second.begin(),e.second.end());
		std::vector<RegionCoordinates> v;
		v.swap(e.second);
		for ( unsigned i = 0; i < v.size(); ) {
			unsigned j = i+1;
			unsigned right = v[i].right();
			for ( ; j < v.size() && v[j].left() <= right; ++j ) right = std::max(v[j].right(),right);
			e.second.push_back(RegionCoordinates(v[i].left(),right));
			i = j;
		}
	}
	return regions;
}

std::vector<unsigned> ReferencesDatabase::getMainGenomeReferencesLengths() const
{
	std::vector<unsigned> lengths(pim->offsetMappedReferences);
	for (unsigned i = 0; i < lengths.size(); ++i) lengths[i] = pim->metadata[i].length;
	return lengths;
}


Gene ReferencesDatabase::getGeneById(unsigned id) const
{
	if (id >= pim->genes.size()) return Gene();
	return pim->genes[id];
}

std::vector<Gene> ReferencesDatabase::getGenesByName(std::string const & name) const
{
	std::vector<Gene> r;
	if (pim->genesBySymbol.count(name) == 0) return r;
	for ( auto id: pim->genesBySymbol.at(name) ) {
		r.push_back(pim->genes[id]);
	}
	return r;
}

std::vector<Gene> const & ReferencesDatabase::getGenes() const
{
	return pim->genes;
}

std::vector<ReferenceId> ReferencesDatabase::getReferencesByName(std::string const & name) const
{
	std::vector<ReferenceId> refsId;
	if (pim->name2refSeq.count(name)) refsId.push_back(ReferenceId(pim->name2refSeq.at(name)));
	return refsId;
}

std::vector<ReferenceId> ReferencesDatabase::getReferencesByGene(std::string const & hgncGeneSymbol) const
{
	unsigned const geneId = hgncSymbolToGeneId(hgncGeneSymbol);
	std::vector<ReferenceId> results;
	for (unsigned i = 0; i < pim->metadata.size(); ++i) {
		if (pim->metadata[i].geneId == geneId) results.push_back(ReferenceId(i));
	}
	return results;
}

std::vector<ReferenceId> ReferencesDatabase::getReferencesByGene(unsigned geneId) const
{
	if (pim->genes.size() <= geneId || ! pim->genes[geneId].active) throw std::runtime_error("Gene is not known: " + boost::lexical_cast<std::string>(geneId));
	std::vector<ReferenceId> results;
	for (unsigned i = 0; i < pim->metadata.size(); ++i) {
		if (pim->metadata[i].geneId == geneId) results.push_back(ReferenceId(i));
	}
	return results;
}

ReferenceId ReferencesDatabase::getTranscriptForProtein(ReferenceId proteinId) const
{
	unsigned pid = proteinId.value;
	if ( pid < pim->offsetProteins || pid - pim->offsetProteins >= pim->proteins2transcripts.size() ) {
		return ReferenceId::null;
	}
	return pim->proteins2transcripts[pid - pim->offsetProteins];
}

ReferenceId ReferencesDatabase::getReferenceIdFromProteinAccessionIdentifier(uint64_t proteinAccessionIdentifier) const
{
	auto it = pim->proteinAccessionIdentifier2referenceId.find(proteinAccessionIdentifier);
	if (it == pim->proteinAccessionIdentifier2referenceId.end()) {
		throw std::logic_error("There is no reference with given protein access identifier: " + std::to_string(proteinAccessionIdentifier));
	}
	return it->second;
}

//std::vector<ReferenceMetadata> const & ReferencesDatabase::getReferencesMetadata() const
//{
//	return pim->metadata;
//}
