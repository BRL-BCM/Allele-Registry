#include "identifiersTools.hpp"
#include "canonicalization.hpp"
#include "../core/exceptions.hpp"
#include "../commonTools/stringTools.hpp"
#include "../commonTools/parseTools.hpp"
#include <memory>


inline NormalizedGenomicVariant mapToGRCh37(ReferencesDatabase const * referencesDb, NormalizedGenomicVariant const & varDef)
{
	ASSERT( referencesDb->getMetadata(varDef.refId).genomeBuild == toString(ReferenceGenome::rgGRCh38) ); // TODO main genome
	NormalizedGenomicVariant out;
	// ============================== map from main genome to GRCh37
	std::vector<NormalizedSequenceModification> allAlignments;
	for (NormalizedSequenceModification const & gmod: varDef.modifications) {
		// TODO - get rid of that mess below
		// ------ AWFUL WORKAROUND FOR PURE INSERTIONS - the underlying alignments logic must be reviewed, especially for zero-length regions
		RegionCoordinates tempRegion = gmod.region;
		bool const awfulWorkaround = (tempRegion.length() == 0);
		if (awfulWorkaround) tempRegion.incRightPosition(1);
		// --------------------------------------
		std::vector<GeneralSeqAlignment> const alignments = referencesDb->getAlignmentsToMainGenome(varDef.refId, tempRegion);
		for (GeneralSeqAlignment const& ga: alignments) {
			if (! ga.isPerfectMatch()) continue;
			Alignment const a = ga.toAlignment();
			if ( referencesDb->getMetadata(a.sourceRefId).genomeBuild != toString(ReferenceGenome::rgGRCh37) ) continue;
			NormalizedSequenceModification mod = gmod;
			mod.region = a.sourceRegion();
			if (awfulWorkaround) {
				if (a.sourceStrandNegativ) mod.region.incLeftPosition(1); else mod.region.decRightPosition(1);
			}
			if (a.sourceStrandNegativ) {
				if (mod.category == variantCategory::duplication || mod.category == variantCategory::shiftableDeletion) {
					mod.originalSequence = rotateLeft(mod.originalSequence, mod.region.length() - mod.originalSequence.size());
				}
				convertToReverseComplementary(mod.originalSequence);
				convertToReverseComplementary(mod.insertedSequence);
			}
			if (out.refId.isNull()) {
				out.refId = a.sourceRefId;
			} else if (out.refId != a.sourceRefId) {
				throw ExceptionNoConsistentAlignment("", "Not unique mapping to GRCh37");  //TODO
			}
			out.modifications.push_back(mod);
		}
	}
	if (varDef.modifications.size() != out.modifications.size()) {
		throw ExceptionNoConsistentAlignment("", "Not unique mapping to GRCh37");  // TODO
	}
	std::sort(out.modifications.begin(), out.modifications.end());
	return out;
}


void parseIdentifierMyVariantInfo
		( ReferencesDatabase    const * referencesDb
		, ReferenceGenome               refGenome
		, std::string           const & id
		, IdentifierWellDefined       & outId
		, NormalizedGenomicVariant    & outVariant
		)
{
	// ===== copy string to buffer
	std::unique_ptr<char[]> buf(new char[id.size()+1]);
	strncpy(buf.get(), id.data(), id.size());
	buf[id.size()] = '\0';
	char const * ptr = buf.get();
	// ===== parse HGVS - chromosome and region
	Chromosome chromosome;
	uint32_t position;
	uint32_t length;
	switch (refGenome) {
		case ReferenceGenome::rgGRCh38: outId.idType = identifierType::MyVariantInfo_hg38; break;
		case ReferenceGenome::rgGRCh37: outId.idType = identifierType::MyVariantInfo_hg19; break;
		default: ASSERT(false);
	}
	try {
		parse(ptr, "chr");
		parse(ptr, chromosome);
		parse(ptr, ":g.");
		parse(ptr, position);
	} catch (std::exception const & e) {
		throw ExceptionUnknownFormatOfMyVariantInfoIdentifier(e.what());
	}
	if (position == 0) throw ExceptionUnknownFormatOfMyVariantInfoIdentifier("Incorrect variation region (position set to 0)");
	if (tryParse(ptr, "_")) {
		unsigned pos2;
		parse(ptr,pos2);
		if (pos2 <= position) throw ExceptionUnknownFormatOfMyVariantInfoIdentifier("Incorrect variation region");
		length = pos2 - position + 1;
		if (length > 10000) throw ExceptionVariationTooLong(length, id);
	} else {
		length = 1;
	}
	std::string refSeq = "";
	std::string newSeq = "";
	// ===== parse HGVS - type, correct position, parse SNP
	if (tryParse(ptr, "delins")) {
		outId.expressionType = IdentifierWellDefined::delins;
		--position;
	} else if (tryParse(ptr, "del")) {
		outId.expressionType = IdentifierWellDefined::deletion;
		--position;
	} else if (tryParse(ptr, "ins")) {
		if (length != 2) throw ExceptionUnknownFormatOfMyVariantInfoIdentifier("Incorrect variation region");
		outId.expressionType = IdentifierWellDefined::delins;
		length = 0;
	} else if (tryParse(ptr, "dup")) {
		outId.expressionType = IdentifierWellDefined::duplication;
		--position;
	} else if (length == 1 && (*ptr == 'A' || *ptr == 'C' || *ptr == 'G' || *ptr == 'T') ) {
		outId.expressionType = IdentifierWellDefined::delins;
		--position;
		refSeq = std::string(1, *ptr);
		++ptr;
		if ( ! tryParse(ptr, ">") ) throw ExceptionUnknownFormatOfMyVariantInfoIdentifier("Unknown expression type (character '>' was expected)");
		if (*ptr == 'A' || *ptr == 'C' || *ptr == 'G' || *ptr == 'T') {
			newSeq = std::string(1, *ptr);
			++ptr;
		} else {
			throw ExceptionUnknownFormatOfMyVariantInfoIdentifier("Incorrect SNP definition");
		}
	} else {
		throw ExceptionUnknownFormatOfMyVariantInfoIdentifier("Unknown expression type");
	}
	// ===== parse HGVS - inserted sequence or reference sequence / length, finish parsing
	if (outId.expressionType == IdentifierWellDefined::delins && newSeq.empty()) {
		parseNucleotideSequence(ptr, newSeq);
		if (newSeq == "") throw ExceptionUnknownFormatOfMyVariantInfoIdentifier("Insertion without sequence");
	} else if (outId.expressionType == IdentifierWellDefined::deletion) {
		unsigned refLength;
		if ( tryParse(ptr, refLength) ) {
			if (length != refLength) throw ExceptionUnknownFormatOfMyVariantInfoIdentifier("Parsed length do not correspond to variant's region's length");
			outId.expressionType = IdentifierWellDefined::deletionWithLength;
		} else {
			parseNucleotideSequence(ptr, refSeq);
			if (refSeq != "") {
				if (refSeq.size() != length) throw ExceptionUnknownFormatOfMyVariantInfoIdentifier("Parsed original sequence do not correspond to variant's region's length");
				outId.expressionType = IdentifierWellDefined::deletionWithSequence;
			}
		}
	} else if (outId.expressionType == IdentifierWellDefined::duplication) {
		unsigned refLength;
		if ( tryParse(ptr, refLength) ) {
			if (length != refLength) throw ExceptionUnknownFormatOfMyVariantInfoIdentifier("Parsed length do not correspond to variant's region's length");
			outId.expressionType = IdentifierWellDefined::duplicationWithLength;
		} else {
			parseNucleotideSequence(ptr, refSeq);
			if (refSeq != "") {
				if (refSeq.size() != length) throw ExceptionUnknownFormatOfMyVariantInfoIdentifier("Parsed original sequence do not correspond to variant's region's length");
				outId.expressionType = IdentifierWellDefined::duplicationWithSequence;
			}
		}
	}
	if (*ptr != '\0') throw ExceptionUnknownFormatOfMyVariantInfoIdentifier("Cannot parse suffix '" + std::string(ptr) + "'");
//	if (id.toString() != id) throw std::runtime_error("Generated id different than original expression: " + id.toString()); // TODO
	// ===== prepare HgvsVariant
	HgvsVariant hgvsVar(true);
	hgvsVar.genomic.refId = referencesDb->getReferenceId( refGenome, chromosome );
	hgvsVar.genomic.firstChromosome.resize(1);
	hgvsVar.genomic.firstChromosome.back().region = RegionCoordinates(position, position + length);
	hgvsVar.genomic.firstChromosome.back().originalSequence = refSeq;
	HgvsGenomicSequence hgs;
	switch (outId.expressionType) {
		case IdentifierWellDefined::deletion:
		case IdentifierWellDefined::deletionWithLength:
		case IdentifierWellDefined::deletionWithSequence:
			break;
		case IdentifierWellDefined::delins:
			hgs.sequence = newSeq;
			hgvsVar.genomic.firstChromosome.back().newSequence.push_back(hgs);
			break;
		case IdentifierWellDefined::duplication:
		case IdentifierWellDefined::duplicationWithLength:
		case IdentifierWellDefined::duplicationWithSequence:
			hgs.refId = hgvsVar.genomic.refId;
			hgs.region = hgvsVar.genomic.firstChromosome.back().region;
			hgvsVar.genomic.firstChromosome.back().newSequence.push_back(hgs);
			hgvsVar.genomic.firstChromosome.back().newSequence.push_back(hgs);
			break;
		default:
			ASSERT(false);
	}
	// ===== canonicalization, set output variant
	outVariant = canonicalize(referencesDb, hgvsVar.genomic);
	// ===== check difference in positions - set last parameters in identifier
	PlainSequenceModification const rightAligned = outVariant.modifications.front().rightAligned();
	uint32_t const positionRightAligned = rightAligned.region.left();
	ASSERT(positionRightAligned >= position);
	ASSERT(positionRightAligned < position + 10000);
	if (   outId.expressionType != IdentifierWellDefined::duplication
		&& outId.expressionType != IdentifierWellDefined::duplicationWithLength
		&& outId.expressionType != IdentifierWellDefined::duplicationWithSequence )
	{
		outId.shiftToRightAligned = positionRightAligned - position;
		uint32_t const minRightEdge = std::max( position, outVariant.modifications.front().region.left() ) + rightAligned.region.length();
		ASSERT(position + length >= minRightEdge);
		outId.rightExtend = (position + length) - minRightEdge;
	} else {
		outId.shiftToRightAligned = positionRightAligned - (position + length);
		outId.rightExtend = 0;
	}
}


void parseIdentifierExACgnomAD
		( ReferencesDatabase    const * referencesDb
		, bool                          isExAC        // true - ExAC, false - gnomAD
		, std::string           const & id
		, IdentifierWellDefined       & outIdentifier
		, NormalizedGenomicVariant           & outVariant
		)
{
	// ===== copy string to buffer
	std::unique_ptr<char[]> buf(new char[id.size()+1]);
	strncpy(buf.get(), id.data(), id.size());
	buf[id.size()] = '\0';
	char const * ptr = buf.get();
	// ===== parse expresion: chr-position-ref-alt
	Chromosome chromosome;
	uint32_t position;
	std::string refSeq = "";
	std::string newSeq = "";
	parse(ptr, chromosome);
	parse(ptr, "-");
	parse(ptr, position);
	if (position == 0) throw ExceptionUnknownFormatOfExACOrGnomADIdentifier("Incorrect variation region (position set to 0)");
	--position;
	parse(ptr, "-");
	parseNucleotideSequence(ptr, refSeq);
	if (refSeq == "") throw ExceptionUnknownFormatOfExACOrGnomADIdentifier("Cannot parse reference sequence");
	parse(ptr, "-");
	parseNucleotideSequence(ptr, newSeq);
	if (newSeq == "") throw ExceptionUnknownFormatOfExACOrGnomADIdentifier("Cannot parse alternative sequence");
	if (*ptr != '\0') throw ExceptionUnknownFormatOfExACOrGnomADIdentifier("Unparsed suffix '" + std::string(ptr) + "'");
	// ===== canonicalize & set out variant
	PlainVariant varDef;
	varDef.refId = referencesDb->getReferenceId( ReferenceGenome::rgGRCh37, chromosome );
	varDef.modifications.resize(1);
	varDef.modifications[0].region.setLeftAndLength(position, refSeq.size());
	varDef.modifications[0].originalSequence = refSeq;
	varDef.modifications[0].newSequence = newSeq;
	outVariant = canonicalizeGenomic(referencesDb, varDef);
	// ===== set out identifier
	outIdentifier.idType = (isExAC) ? (identifierType::ExAC) : (identifierType::gnomAD);
	outIdentifier.expressionType = IdentifierWellDefined::noHgvs;
	outIdentifier.shiftToRightAligned = outIdentifier.rightExtend = 0;
}


std::string buildIdentifierMyVariantInfo
		( ReferencesDatabase    const * referencesDb
		, IdentifierWellDefined const & id
		, NormalizedGenomicVariant     const & varDef
		)
{
	ASSERT(id.idType == identifierType::MyVariantInfo_hg19 || id.idType == identifierType::MyVariantInfo_hg38);
	// ===== calculate left & right aligned definitions
	PlainVariant variant;
	PlainSequenceModification rightAligned;
	if (id.idType == identifierType::MyVariantInfo_hg19) {
		NormalizedGenomicVariant v = mapToGRCh37(referencesDb, varDef);
		variant = v.leftAligned();
		rightAligned = v.modifications.front().rightAligned();
	} else {
		variant = varDef.leftAligned();
		rightAligned = varDef.modifications.front().rightAligned();
	}
	// ===== adjust left edge
//	std::cout << "left = " << variant.modifications.front().region.toString() << "\t" << variant.modifications.front().originalSequence << "\t" << variant.modifications.front().newSequence << std::endl;
//	std::cout << "right= " << rightAligned.region.toString() << "\t" << rightAligned.originalSequence << "\t" << rightAligned.newSequence << std::endl;
//	std::cout << id.shiftToRightAligned << "\t" << id.rightExtend << std::endl;
	PlainSequenceModification & m = variant.modifications.front();
	if (m.region.left() + id.shiftToRightAligned > rightAligned.region.left()) {
		// we have to add some extra bp from the left
		unsigned const extend = (m.region.left() + id.shiftToRightAligned) - rightAligned.region.left();
		std::string const seq = referencesDb->getSequence( variant.refId, RegionCoordinates(m.region.left()-extend, m.region.left()) );
		m.originalSequence = seq + m.originalSequence;
		m.newSequence      = seq + m.newSequence;
		m.region.decLeftPosition(extend);
	} else {
		// we have to shift variant to right
		unsigned const shift = rightAligned.region.left() - (m.region.left() + id.shiftToRightAligned);
		m.originalSequence = rotateLeft(m.originalSequence, shift);
		m.newSequence      = rotateLeft(m.newSequence     , shift);
		m.region.incRightPosition(shift);
		m.region.incLeftPosition (shift);
	}
	// ===== adjust right edge
	if (id.rightExtend > 0) {
		std::string const seq = referencesDb->getSequence( variant.refId, RegionCoordinates(m.region.right(), m.region.right()+id.rightExtend) );
		m.originalSequence = m.originalSequence + seq;
		m.newSequence      = m.newSequence      + seq;
		m.region.incRightPosition(id.rightExtend);
	}
	// ===== create HGVS - chromosome and region
	std::string s = "chr" + ::toString(referencesDb->getMetadata(variant.refId).chromosome) + ":g.";
	if (   id.expressionType == IdentifierWellDefined::duplication
		|| id.expressionType == IdentifierWellDefined::duplicationWithLength
		|| id.expressionType == IdentifierWellDefined::duplicationWithSequence )
	{
		// temporary correction for duplication
		m.region.decLeftPosition(m.newSequence.size());
	}
	if (m.region.length() == 0) {
		s += boost::lexical_cast<std::string>(m.region.left()) + "_" + boost::lexical_cast<std::string>(m.region.left() + 1);
	} else {
		s += boost::lexical_cast<std::string>(m.region.left() + 1);
		if (m.region.length() > 1) s += "_" + boost::lexical_cast<std::string>(m.region.right());
	}
	// ===== create HGVS - modification & rest
	switch (id.expressionType) {
		case IdentifierWellDefined::deletion:
			s += "del";
			break;
		case IdentifierWellDefined::deletionWithLength:
			s += "del" + boost::lexical_cast<std::string>(m.originalSequence.size());
			break;
		case IdentifierWellDefined::deletionWithSequence:
			s += "del" + m.originalSequence;
			break;
		case IdentifierWellDefined::duplication:
			s += "dup";
			break;
		case IdentifierWellDefined::duplicationWithLength:
			s += "dup" + boost::lexical_cast<std::string>(m.newSequence.size());
			break;
		case IdentifierWellDefined::duplicationWithSequence:
			s += "dup" + m.newSequence;
			break;
		case IdentifierWellDefined::delins:
			if (m.region.length() == 1 && m.newSequence.length() == 1) {
				// snp
				s += m.originalSequence + ">" + m.newSequence;
			} else if (m.region.length() == 0) {
				s += "ins" + m.newSequence;
			} else {
				s += "delins" + m.newSequence;
			}
			break;
		default:
			ASSERT(false);
	}
	return s;
}


void buildIdentifierExACgnomAD
		( ReferencesDatabase    const * referencesDb
		, IdentifierWellDefined const & id
		, NormalizedGenomicVariant     const & varDef
		, std::string                 & outChr
		, std::string                 & outPos
		, std::string                 & outRef
		, std::string                 & outAlt
		)
{
	ASSERT(id.idType == identifierType::ExAC || id.idType == identifierType::gnomAD);
	// ===== calculate left aligned definition
	PlainVariant variant = mapToGRCh37(referencesDb, varDef).leftAligned();
	// ===== add bp from left side if needed
	PlainSequenceModification & m = variant.modifications.front();
	if (m.newSequence.empty() || m.originalSequence.empty()) {
		if (m.region.left() > 0) {
			m.region.decLeftPosition(1);
			std::string bp = referencesDb->getSequence(variant.refId, RegionCoordinates(m.region.left(),m.region.left()+1));
			m.originalSequence = bp + m.originalSequence;
			m.newSequence = bp + m.newSequence;
		}
	}
	// ===== set output parameters
	outChr = toString( referencesDb->getMetadata(variant.refId).chromosome );
	outPos = boost::lexical_cast<std::string>(m.region.left()+1);
	outRef = m.originalSequence;
	outAlt = m.newSequence;
}

