#include "alignment.hpp"
#include <algorithm>
#include <boost/lexical_cast.hpp>


// the length of given source must equal sourceLength
std::string Alignment::processSourceSubSequence(std::string const & source) const
{
	if (source.size() != this->sourceLength()) throw std::runtime_error("Alignment::processSequence(source) - length of source does not match");
	std::string::const_iterator isrc = source.begin();
	std::string sourceCompl;
	if (this->sourceStrandNegativ) {
		sourceCompl = source;
		convertToReverseComplementary(sourceCompl);
		isrc = sourceCompl.begin();
	}
	std::string target = "";
	for (auto const & e : elements) {
		target += std::string(isrc, isrc + e.matchedBp) + e.insertedSeq;
		isrc += e.matchedBp + e.deletedBp;
	}
	return target;
}



unsigned Alignment::targetCutLeft_exactAlignment(unsigned & lengthToCut)
{
	unsigned lengthOfSourceCut = 0;
	unsigned lengthOfTargetCut = 0;
	// cut whole elements from left when possible
	auto it = elements.begin();
	for ( ;  it != elements.end();  ++it ) {
		if (lengthOfTargetCut + it->targetLength() > lengthToCut) break;
		lengthOfTargetCut += it->targetLength();
		lengthOfSourceCut += it->sourceLength();
	}
	// cut part of the first of remaining elements
	if ( (it != elements.end()) && (lengthToCut > lengthOfTargetCut) ) {
		{ // cut matched part
			unsigned const toCut = std::min(lengthToCut-lengthOfTargetCut, it->matchedBp);
			lengthOfTargetCut += toCut;
			lengthOfSourceCut += toCut;
			it->matchedBp -= toCut;
		}
		{ // cut indel
			unsigned const toCut = lengthToCut-lengthOfTargetCut;
			if (toCut > 0) {
				if (it->deletedBp == 0) {
					lengthOfTargetCut += toCut;
					it->insertedSeq = it->insertedSeq.substr(toCut);
				} else if (it->deletedBp == it->insertedSeq.size()) {
					lengthOfTargetCut += toCut;
					lengthOfSourceCut += toCut;
					it->deletedBp -= toCut;
					it->insertedSeq = it->insertedSeq.substr(toCut);
				} else {
					// cut whole element and adjust cut length
					lengthOfTargetCut += it->insertedSeq.size();
					lengthOfSourceCut += it->deletedBp;
					++it;
				}
			}
		}
	}
	// save the rest of changes to object
	elements.erase(elements.begin(),it);
	targetLeftPosition += lengthOfTargetCut;
	if (! sourceStrandNegativ) sourceLeftPosition += lengthOfSourceCut;
	// check the result
	if ( lengthToCut > lengthOfTargetCut ) throw std::out_of_range("Alignment::targetCutLeft_exactAlignment");
	lengthToCut = lengthOfTargetCut;
	return lengthOfSourceCut;
}

unsigned Alignment::targetCutRight_exactAlignment(unsigned & lengthToCut)
{
	unsigned lengthOfSourceCut = 0;
	unsigned lengthOfTargetCut = 0;
	// cut whole elements from right when possible
	auto it = elements.end();
	while ( it != elements.begin() ) {
		auto it2 = it;
		--it2;
		if (lengthOfTargetCut + it2->targetLength() > lengthToCut) break;
		--it;
		lengthOfTargetCut += it->targetLength();
		lengthOfSourceCut += it->sourceLength();
	}
	// cut part of the last of remaining elements
	if ( (it != elements.begin()) && (lengthOfTargetCut < lengthToCut) ) {
		auto it2 = it;
		--it2;
		unsigned toCut = lengthToCut - lengthOfTargetCut;
		if (it2->insertedSeq.size() > toCut) {
			if (it2->deletedBp == 0) {
				lengthOfTargetCut += toCut;
				it2->insertedSeq = it2->insertedSeq.substr(0,it2->insertedSeq.size()-toCut);
			} else if (it2->deletedBp == it2->insertedSeq.size()) {
				lengthOfTargetCut += toCut;
				lengthOfSourceCut += toCut;
				it2->deletedBp -= toCut;
				it2->insertedSeq = it2->insertedSeq.substr(0,it2->insertedSeq.size()-toCut);
			} else {
				lengthOfTargetCut += it2->insertedSeq.size();
				lengthOfSourceCut += it2->deletedBp;
				if (it2->matchedBp > 0) {
					it2->deletedBp = 0;
					it2->insertedSeq = "";
				} else {
					// cut whole element
					--it;
				}
			}
		} else {
			// cut indel
			toCut -= it2->insertedSeq.size();
			lengthOfTargetCut += it2->insertedSeq.size() + toCut;
			lengthOfSourceCut += it2->deletedBp + toCut;
			it2->deletedBp = 0;
			it2->insertedSeq = "";
			it2->matchedBp -= toCut;
		}
	}
	// save the rest of changes to the object
	elements.erase(it,elements.end());
	if (sourceStrandNegativ) sourceLeftPosition += lengthOfSourceCut;
	// check the result
	if ( lengthToCut > lengthOfTargetCut ) throw std::out_of_range("Alignment::targetCutRight_exactAlignment");
	lengthToCut = lengthOfTargetCut;
	return lengthOfSourceCut;
}

unsigned Alignment::targetCutLeft_exactTarget(unsigned lengthToCut)
{
	unsigned lengthOfSourceCut = 0;
	unsigned lengthOfTargetCut = 0;
	// cut whole elements from left when possible
	auto it = elements.begin();
	for ( ;  it != elements.end();  ++it ) {
		if (lengthOfTargetCut + it->targetLength() > lengthToCut) break;
		lengthOfTargetCut += it->targetLength();
		lengthOfSourceCut += it->sourceLength();
	}
	// cut part of the first of remaining elements
	if ( (it != elements.end()) && (lengthToCut > lengthOfTargetCut) ) {
		unsigned const toCut = lengthToCut-lengthOfTargetCut;
		if (toCut < it->matchedBp) {
			lengthOfTargetCut += toCut;
			lengthOfSourceCut += toCut;
			it->matchedBp -= toCut;
		} else {
			lengthOfTargetCut += toCut;
			lengthOfSourceCut += it->matchedBp + it->deletedBp;
			it->insertedSeq = it->insertedSeq.substr(toCut-it->matchedBp);
			it->matchedBp = 0;
			it->deletedBp = 0;
		}
	}
	// save the rest of changes to object
	elements.erase(elements.begin(),it);
	targetLeftPosition += lengthOfTargetCut;
	if (! sourceStrandNegativ) sourceLeftPosition += lengthOfSourceCut;
	// check the result
	if ( lengthToCut > lengthOfTargetCut ) throw std::out_of_range("Alignment::targetCutLeft_exactTarget");
	return lengthOfSourceCut;
}

unsigned Alignment::targetCutRight_exactTarget(unsigned lengthToCut)
{
	unsigned lengthOfSourceCut = 0;
	unsigned lengthOfTargetCut = 0;
	// cut whole elements from right when possible
	auto it = elements.end();
	while ( it != elements.begin() ) {
		auto it2 = it;
		--it2;
		if (lengthOfTargetCut + it2->targetLength() > lengthToCut) break;
		--it;
		lengthOfTargetCut += it->targetLength();
		lengthOfSourceCut += it->sourceLength();
	}
	// cut part of the last of remaining elements
	if ( (it != elements.begin()) && (lengthOfTargetCut < lengthToCut) ) {
		auto it2 = it;
		--it2;
		unsigned toCut = lengthToCut - lengthOfTargetCut;
		lengthOfTargetCut += toCut;
		if (it2->insertedSeq.size() > toCut) {
			it2->insertedSeq = it2->insertedSeq.substr(0,it2->insertedSeq.size()-toCut);
			toCut = 0;
		} else {
			toCut -= it2->insertedSeq.size();
			it2->insertedSeq = "";
		}
		lengthOfSourceCut += it2->deletedBp + toCut;
		it2->deletedBp = 0;
		it2->matchedBp -= toCut;
	}
	// save the rest of changes to the object
	elements.erase(it,elements.end());
	if (sourceStrandNegativ) sourceLeftPosition += lengthOfSourceCut;
	// check the result
	if ( lengthToCut > lengthOfTargetCut ) throw std::out_of_range("Alignment::targetCutRight_exactTarget");
	return lengthOfSourceCut;
}


// pos and length may lay outside the sequence, like for string::substr - it returns alignments for intersection
// if there is no intersection, the exception is thrown
Alignment Alignment::targetSubalign(unsigned pos, unsigned length, bool exactTargetEdges) const
{
	// ---- calculate intersection (adjust parameters)
	if (pos < targetLeftPosition) {
		unsigned const diff = targetLeftPosition - pos;
		pos = targetLeftPosition;
		if (length < diff) throw std::out_of_range("Alignment::targetSubalign");
		length -= diff;
	}
	unsigned const targetRightPosition = targetLeftPosition + targetLength();
	if (pos > targetRightPosition) throw std::out_of_range("Alignment::targetSubalign");
	if (length > targetRightPosition - pos) length = targetRightPosition - pos;
	// ---- left & right cut
	unsigned targetLeftCut  = pos - targetLeftPosition;
	unsigned targetRightCut = targetRightPosition - pos - length;
	Alignment r = *this;
	if (exactTargetEdges) {
		r.targetCutLeft_exactTarget(targetLeftCut);
		r.targetCutRight_exactTarget(targetRightCut);
	} else {
		r.targetCutLeft_exactAlignment(targetLeftCut);
		r.targetCutRight_exactAlignment(targetRightCut);
	}
	return r;
}


// calculate reverse - switch source <-> target
Alignment Alignment::reverse(StringPointer const & source, ReferenceId targetRefId) const
{
	Alignment r;
	r.sourceLeftPosition = targetLeftPosition;
	r.targetLeftPosition = sourceLeftPosition;
	r.sourceStrandNegativ = sourceStrandNegativ;
	r.sourceRefId = targetRefId;
	if (sourceStrandNegativ) {
		// copy elements to target alignment and revert them
		std::vector<Element>::const_iterator iE = elements.begin();
		if (iE->matchedBp > 0) {
			r.elements.push_back(Element());
			r.elements.front().matchedBp = iE->matchedBp;
		}
		while ( true ) {
			r.elements.push_back(Element());
			r.elements.back().deletedBp = iE->deletedBp;
			r.elements.back().insertedSeq = iE->insertedSeq;
			if (++iE == elements.end()) break;
			r.elements.back().matchedBp = iE->matchedBp;
		}
		if (r.elements.back().matchedBp == 0 && r.elements.back().deletedBp == 0 && r.elements.back().insertedSeq.empty()) r.elements.pop_back();
		std::reverse(r.elements.begin(), r.elements.end());
	} else {
		// copy elements to target alignment
		r.elements = elements;
	}
	// change deleted <-> inserted
	unsigned sourcePos = 0;
	std::vector<Element>::iterator iE = r.elements.begin();
	for (; iE != r.elements.end(); ++iE) {
		unsigned const orgDeletedBp = iE->deletedBp;
		sourcePos += iE->matchedBp;
		iE->deletedBp = iE->insertedSeq.size();
		iE->insertedSeq = source.substr(sourcePos,orgDeletedBp);
		sourcePos += orgDeletedBp;
	}
	// return result
	return r;
}

void Alignment::append(Alignment const & a2)
{
	if (sourceRefId != a2.sourceRefId) throw std::logic_error("Alignment::append() - source reference ID does not match");
	if (sourceStrandNegativ != a2.sourceStrandNegativ) throw std::logic_error("Alignment::append() - source strand does not match");
	RegionCoordinates const rt1 = targetRegion();
	RegionCoordinates const rt2 = a2.targetRegion();
	if (rt1.right() != rt2.left()) throw std::logic_error("Alignment::append() - target region does not match");
	RegionCoordinates const rs1 = sourceRegion();
	RegionCoordinates const rs2 = a2.sourceRegion();
	if (sourceStrandNegativ) {
		if (rs1.left() != rs2.right()) throw std::logic_error("Alignment::append() - source region does not match");
	} else {
		if (rs1.right() != rs2.left()) throw std::logic_error("Alignment::append() - source region does not match");
	}
	if (a2.elements.empty()) return;
	if (elements.empty()) {
		*this = a2;
		return;
	}
	// append alignment a2 - check case when the last element from this can be connected with the first element from a2
	auto it = a2.elements.begin();
	if (elements.back().deletedBp == 0 && elements.back().insertedSeq.empty()) {
		elements.back().matchedBp += it->matchedBp;
		elements.back().deletedBp = it->deletedBp;
		elements.back().insertedSeq = it->insertedSeq;
		++it;
	} else if (it->matchedBp == 0) {
		if (elements.back().deletedBp == elements.back().insertedSeq.size() && it->deletedBp == it->insertedSeq.size()) {
			elements.back().deletedBp += it->deletedBp;
			elements.back().insertedSeq += it->insertedSeq;
			++it;
		}
	}
	elements.insert( elements.end(), it, a2.elements.end() );
	if (sourceStrandNegativ) sourceLeftPosition = a2.sourceLeftPosition;
}


static std::vector<std::pair<char,unsigned>> parseCigar(std::string const & cigar)
{
	std::vector<std::pair<char,unsigned>> v;
	std::string::const_iterator iC = cigar.begin();
	std::string::const_iterator iCend = cigar.end();
	unsigned count = 0;
	for ( ;  iC != iCend;  ++iC ) {
		if (isdigit(*iC)) {
			count *= 10;
			count += *iC - '0';
			continue;
		}
		if (count == 0) continue;
//		if ( count == 0 ) {
//			throw std::runtime_error("Incorrect CIGAR string: missing number (or 0) after parsing " + std::string(cigar.begin(),iC) + " from " + cigar);
//		}
		v.push_back( std::make_pair(*iC,count) );
		count = 0;
	}
	if (count) throw std::runtime_error("Incorrect CIGAR string: missing symbol at the end of CIGAR string: " + cigar);
	return v;
}

// set value
void Alignment::set ( std::string const & cigar, ReferenceId srcRefId, bool srcStrandNegativ
					, std::string const & srcSubseq, unsigned srcPos
					, std::string const & trgSubseq, unsigned trgPos )
{
	std::string srcSeqCopy;
	if (srcStrandNegativ) {
		srcSeqCopy = srcSubseq;
		convertToReverseComplementary(srcSeqCopy);
	}
	std::string::const_iterator iS = (srcStrandNegativ) ? (srcSeqCopy.begin()) : (srcSubseq.begin());
	std::string::const_iterator iT = trgSubseq.begin();
	std::string::const_iterator const iSend = (srcStrandNegativ) ? (srcSeqCopy.end()) : (srcSubseq.end());
	std::string::const_iterator const iTend = trgSubseq.end();
	Alignment a;
	a.sourceLeftPosition = srcPos;
	a.targetLeftPosition = trgPos;
	a.sourceStrandNegativ = srcStrandNegativ;
	a.sourceRefId = srcRefId;
	Alignment::Element e;
	auto checkSrcLength = [&](unsigned length) { if (iSend - iS < length) throw std::runtime_error("Incorrect CIGAR or source string: source string too short"); };
	auto checkTrgLength = [&](unsigned length) { if (iTend - iT < length) throw std::runtime_error("Incorrect CIGAR or target string: target string too short"); };
	auto addMatch       = [&](unsigned length) { if (e.deletedBp || e.insertedSeq.size()) { a.elements.push_back(e); e = Element(); };
												 e.matchedBp += length; iS += length; iT += length; };
	auto addInsertion   = [&](unsigned length) { e.insertedSeq += std::string(iT,iT+length); iT += length; };
	auto addDeletion    = [&](unsigned length) { e.deletedBp += length; iS += length; };
	auto const vc = parseCigar(cigar);
	for ( auto const & c : vc ) {
		unsigned count = c.second;
		switch (c.first) {
			case 'M':
				checkSrcLength(count);
				checkTrgLength(count);
				for (; count; --count ) {
					if (*iS == *iT) {
						addMatch(1);
					} else {
						addDeletion(1);
						addInsertion(1);
					}
				}
				break;
			case 'I':
				checkTrgLength(count);
				addInsertion(count);
				break;
			case 'D':
				checkSrcLength(count);
				addDeletion(count);
				break;
			case '=':
				checkSrcLength(count);
				checkTrgLength(count);
				for (unsigned i = 0; i < count; ++i) if ( *(iS+i) != *(iT+i) ) throw std::runtime_error("CIGAR string do not match sequences, wrong = bp");
				addMatch(count);
				break;
			case 'X':
				checkSrcLength(count);
				checkTrgLength(count);
				for (unsigned i = 0; i < count; ++i) if ( *(iS+i) == *(iT+i) ) throw std::runtime_error("CIGAR string do not match sequences, wrong X bp");
				addDeletion(count);
				addInsertion(count);
				break;
			default : throw std::runtime_error("Incorrect CIGAR string: alignment symbol not supported: " + c.first);
		}
		//std::cout << std::string(iC+1,iCend) << " " << std::string(iS,iSend) << " " << std::string(iT,iTend) << std::endl;
	}
	if (e.matchedBp || e.deletedBp || e.insertedSeq.size()) a.elements.push_back(e);
	if (iS != iSend) throw std::runtime_error("Incorrect CIGAR or source string: source string too long, rest=" + std::string(iS,iSend));
	if (iT != iTend) throw std::runtime_error("Incorrect CIGAR or target string: target string too long, rest=" + std::string(iT,iTend));
	*this = a;
}

// set value
void Alignment::set ( std::string const & cigar, ReferenceId srcRefId, bool srcStrandNegativ
					, unsigned srcPos, unsigned trgPos, std::vector<std::string> const & insertedOrModifiedSequences )
{
	Alignment a;
	a.sourceLeftPosition = srcPos;
	a.targetLeftPosition = trgPos;
	a.sourceStrandNegativ = srcStrandNegativ;
	a.sourceRefId = srcRefId;
	Alignment::Element e;
	unsigned seqToInsert = 0;
	auto addMatch       = [&](unsigned length) { if (e.deletedBp || e.insertedSeq.size()) { a.elements.push_back(e); e = Element(); };
												 e.matchedBp += length; };
	auto addInsertion   = [&](unsigned length) { if ( seqToInsert >= insertedOrModifiedSequences.size()
													|| insertedOrModifiedSequences[seqToInsert].size() != length)
													throw std::logic_error("Alignment::set(...): vector of sequences does not match CIGAR string");
												 e.insertedSeq += insertedOrModifiedSequences[seqToInsert++]; };
	auto addDeletion    = [&](unsigned length) { e.deletedBp += length; };
	auto const pc = parseCigar(cigar);
	for ( auto const & c: pc ) {
		unsigned count = c.second;
		switch (c.first) {
			case 'I':
				addInsertion(count);
				break;
			case 'D':
				addDeletion(count);
				break;
			case '=':
				addMatch(count);
				break;
			case 'X':
				addDeletion(count);
				addInsertion(count);
				break;
			default : throw std::runtime_error("Incorrect CIGAR string: alignment symbol not supported: " + c.first);
		}
		//std::cout << std::string(iC+1,iCend) << " " << std::string(iS,iSend) << " " << std::string(iT,iTend) << std::endl;
	}
	if (e.matchedBp || e.deletedBp || e.insertedSeq.size()) a.elements.push_back(e);
	if ( seqToInsert != insertedOrModifiedSequences.size() ) throw std::logic_error("Alignment::set(...): vector of sequences does not match CIGAR string");
	*this = a;
}


bool Alignment::isPerfectMatch() const
{
	if (elements.empty()) return true;
	if (elements.size() > 1) return false;
	if (elements[0].deletedBp != elements[0].insertedSeq.size()) return false;
	if (elements[0].deletedBp > 0 && elements[0].matchedBp > 0) return false;
	return true;
}


bool Alignment::isIdentical() const
{
	if (elements.empty()) return true;
	if (elements.size() > 1) return false;
	if (elements[0].deletedBp != 0 ) return false;
	if (! elements[0].insertedSeq.empty()) return false;
	return true;
}


// returns CIGAR string
std::string Alignment::cigarString(bool withInsertions) const
{
	std::string s;
	for (auto const & e: elements) {
		if (e.matchedBp) {
			s += boost::lexical_cast<std::string>(e.matchedBp) + "=";
		}
		if (e.deletedBp == 0 && e.insertedSeq.empty()) continue;
		if (e.deletedBp == e.insertedSeq.size()) {
			s += boost::lexical_cast<std::string>(e.deletedBp) + "X";
			continue;
		}
		if (e.deletedBp) {
			s += boost::lexical_cast<std::string>(e.deletedBp) + "D";
		}
		if (! e.insertedSeq.empty()) {
			s += boost::lexical_cast<std::string>(e.insertedSeq.size()) + "I";
		}
	}
	if (withInsertions) {
		for (auto const & e: elements) {
			if (! e.insertedSeq.empty()) s += " " + e.insertedSeq;
		}
	}
	return s;
}

// create alignment from cigar, positions and sequences
Alignment Alignment::createFromFullSequences( std::string const & cigar, ReferenceId srcRefId, bool srcNegativeStrand, std::string const & srcFullSeq
											, unsigned srcPos, std::string const & trgFullSeq, unsigned trgPos)
{
	Alignment a;
	auto const vc = parseCigar(cigar);
	unsigned srcLength = 0, trgLength = 0;
	for (auto const & c: vc) {
		switch (c.first) {
		case 'X':
		case 'M':
		case '=':
			srcLength += c.second;
			trgLength += c.second;
			break;
		case 'I':
			trgLength += c.second;
			break;
		case 'D':
			srcLength += c.second;
			break;
		default:
			throw std::runtime_error("Incorrect CIGAR string: alignment symbol not supported: " + c.first);
		}
	}

	std::string const srcSeq = srcFullSeq.substr(srcPos, srcLength);
	std::string const trgSeq = trgFullSeq.substr(trgPos, trgLength);
	a.set(cigar, srcRefId, srcNegativeStrand, srcSeq, srcPos, trgSeq, trgPos);
	return a;
}


Alignment Alignment::createIdentityAlignment(ReferenceId srcRefId, bool srcNegativeStrand, unsigned srcPos, unsigned trgPos, unsigned length)
{
	Alignment a;
	a.sourceLeftPosition = srcPos;
	a.targetLeftPosition = trgPos;
	a.sourceStrandNegativ = srcNegativeStrand;
	a.sourceRefId = srcRefId;
	a.elements.resize(1);
	a.elements[0].matchedBp = length;
	return a;
}


GeneralSeqAlignment GeneralSeqAlignment::createIdentityAlignment(ReferenceId refId, RegionCoordinates const & region)
{
	GeneralSeqAlignment g;
	g.elements.resize(1);
	g.elements[0].alignment.set(boost::lexical_cast<std::string>(region.length()) + "=", refId, false, region.left(), region.left(), std::vector<std::string>());
	return g;
}


std::string GeneralSeqAlignment::toString() const
{
	if (this->elements.empty()) return "{}";
	std::string s = "{" + elements.front().toString();
	for (unsigned i = 1; i < elements.size(); ++i) {
		s += "," + elements[i].toString();
	}
	s += "}";
	return s;
}


std::string Alignment::toString() const
{
	std::string s = "{";
	s += boost::lexical_cast<std::string>(this->sourceRefId.value) + (sourceStrandNegativ?'-':'+')  + ";";
	s += boost::lexical_cast<std::string>(this->sourceRegion().toString()) + "->";
	s += boost::lexical_cast<std::string>(this->targetRegion().toString()) + ";";
	s += cigarString() + "}";
	return s;
}

