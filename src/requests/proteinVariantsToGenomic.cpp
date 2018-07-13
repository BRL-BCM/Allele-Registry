#include "../core/variants.hpp"
//#include "../commonTools/fastString.hpp"
#include "proteinTools.hpp"
#include <map>


class Solver_SearchForGenomicCauses
{
public:
	struct State {
		std::vector< std::vector<PlainSequenceModification>::const_iterator > appliedRnaVariants; // combination of RNA variants
		unsigned rnaUmodifiedOffset = 0;        // original RNA offset, corresponds to right boundary of the last RNA modification or left boundary of current codon (whatever comes last)
		std::string rnaPrefix = std::string(""); // prefix of the RNA sequence, should be added at the beginning of the RNA calculated form the offset above
		// the name does not make sense - inline unsigned rnaPosition() const { return (rnaUmodifiedOffset - rnaPrefix.size()); }
	};

	ReferenceId const transcriptId;
	unsigned const transcriptStartCodon;                             // position of start codon on transcript
	std::string const transcriptSequence;
	std::vector<PlainSequenceModification> const transcriptVariants;
	std::string const proteinSequence;
	std::vector<PlainVariant> const proteinVariants;                 // variants to explain, must be sorted
	std::map<std::pair<unsigned,char>,std::vector<State>> statesAtTheBeginningOfProteinVariants;  // states to check: index = (AA position,new AA)
	std::vector< std::vector<PlainVariant> > outGenomicCauses;

	// offset % 3 (0,1,2) -> AA position -> AA (enum code) -> byte = state(4 bits) + 2 nucletides(4 bits, first(2 bits) + second(2 bits))
	// states (4bits):
	// 0 -> not possible (given AA cannot be reached)
	// 8 -> no SNPs (original AA)
	// 4 -> SNP on the left
	// 2 -> SNP in the middle
	// 1 -> SNP on the right
	// 5 -> SNP on the left and right
//	std::vector<std::vector<std::vector<uint8_t>>> allProteinSequences;
//
//	void calculateAllPossibleProteinsWithSNP()
//	{
//		std::string rnaSeq = transcriptSequence.substr(transcriptStartCodon+3) + "AA";
//		allProteinSequences.clear();
//		allProteinSequences.resize(3);
//		for (unsigned i = 0; i < 3; ++i) {
//			allProteinSequences[i].resize( (rnaSeq.size() - i) / 3, std::vector<uint8_t>(21,0) );
//			// check all codons
//			std::vector<PlainSequenceModification>::iterator iV;
//			iV = std::lower_bound( transcriptVariants.begin(), transcriptVariants.end(), transcriptStartCodon+3+i
//					  , [](PlainSequenceModification const & v, unsigned p)->bool{ return (v.region.left()<p); } );
//			for (unsigned iAA = 0; iAA < allProteinSequences[i].size(); ++iAA) {
//				std::string const codon = rnaSeq.substr( i+3*iAA, 3 );
//				AminoAcid const aa = oneLettersCode2aminoAcid(translateToAminoAcid(codon).substr(0,1));
//				allProteinSequences[i][iAA][aa] = 0x80;
//				for ( ;  iV != transcriptVariants.end() && iV->region.left() < transcriptStartCodon+3+i+3*iAA+3;  ++iV ) {
//					if (iV->region.length() != 1 || iV->newSequence.size() != 1) continue;
//					std::string codon
//				}
//			}
//		}
//	}

	// return first new AA in protein variant (must be right-shifted)
	inline char firstDifferentAminoAcid(PlainVariant const & pv)
	{
		ASSERT(! pv.modifications.empty());
		if ( ! pv.modifications.front().newSequence.empty() )
			return pv.modifications.front().newSequence.front();
		if ( proteinSequence.size() > pv.modifications.front().region.right() )
			return proteinSequence[pv.modifications.front().region.right()];
		return aminoAcid2oneLettersCode(AminoAcid::aaStopCodon)[0];
	}

	inline std::pair<unsigned,char> positionAndFirstDifferentAminoAcid(PlainVariant const & pv)
	{
		return std::make_pair(pv.modifications.front().region.left(), firstDifferentAminoAcid(pv));
	}

	// Recursive function that checks all possible combination of transcript variants.
	// Tracking stops in two cases:
	// -> on the first AA change - in this case corresponding protein variant is looked up; if successfully, one of the possible solution is saved
	// -> if offset between RNA and protein returns to 0 (rnaPositionDifference == 0) - if this case we return to identity relation
	void matchRnaVariantToProtein
		( unsigned aaPosition         // current aa position
		, unsigned rnaOffset          // current RNA position according to original sequence, always aligned to left border of current aa position
		, std::string rnaSequence     // RNA sequence starting from rnaOffset
		, std::vector<PlainSequenceModification>::const_iterator variant                           // variant to apply
		, std::vector< std::vector<PlainSequenceModification>::const_iterator > & currentVariants  // list of already applied variants
		)
	{
		if (currentVariants.size() > 5) return; // no more than 5 variants
//DebugVar(currentVariants.size());
//DebugVar(aaPosition);
//DebugVar(rnaOffset);
//DebugVar(*variant);
//for (auto a: currentVariants) { std::cout << a->region.left() << "," << a->region.right() << "," << a->newSequence << " "; }
//std::cout << std::endl;
		ASSERT(rnaOffset <= variant->region.left() && variant->region.left() < rnaOffset + 3); // given variant must start in the first codon/aa
		// ==== apply variant
		rnaSequence = applyVariantToTheSequence(rnaSequence, rnaOffset, *variant);
		currentVariants.push_back(variant);
		// this is part of sequence being effect by applying the variant, it cannot contain any other variants
		unsigned const rnaLocked = variant->region.left() + variant->newSequence.size() - rnaOffset;
		unsigned const aaLocked = rnaLocked / 3;
		unsigned const rnaLockedRest = rnaLocked - 3*aaLocked;
		// update rnaOffset
		rnaOffset = variant->region.right() - rnaLockedRest;
		// difference between original rna position and current rna position (for the same aa position)
		int const rnaPositionDifference = static_cast<int>(transcriptStartCodon + 3*(aaPosition + aaLocked)) - static_cast<int>(rnaOffset);

		// ==== current status
		//
		//   variant is drawn as =====
		//
		//          aaPosition
		//              |
		// amino-acid   |==========================|=---------
		//              |         aaLocked         |
		//
		//       old rnaOffset             new rnaOffset   <---- rnaOffset is position according to ORIGINAL sequence, not current rnaSequence with applied variants!!!
		//              |                          |
		//        RNA   |--========================|=|--------
        //              |           rnaLocked        |
        //              |                          | |
        //              	                        A
        //                                          |
		//                                   rnaLockedRest (0, 1 or 2)

		// ==== check sequence
		// number of matching aa
		unsigned matchingAa = 0;
		for ( ;  aaPosition + matchingAa < proteinSequence.size() && rnaSequence.size() >= 3*(matchingAa+1);  ++matchingAa ) {
			// --- check, if we are not aligned to original sequence
			if ( rnaPositionDifference == 0 && 3*matchingAa >= rnaLocked ) {
				// the rest must match, so we stop here
				currentVariants.pop_back();
				return;
			}
			// --- check current aa
			if ( translateToAminoAcid(rnaSequence.substr(3*matchingAa,3))[0] != proteinSequence[aaPosition+matchingAa] ) break;
		}

		// ==== save if one of the possible solution
		{
			char newAA = '?';
			if ( rnaSequence.size() >= 3*(matchingAa+1) ) {
				newAA = translateToAminoAcid(rnaSequence.substr(3*matchingAa,3))[0];
			}
			auto const key = std::make_pair(aaPosition+matchingAa,newAA);
			if (statesAtTheBeginningOfProteinVariants.count(key)) {
				State s;
				s.appliedRnaVariants = currentVariants;
				if (matchingAa < aaLocked) {
					s.rnaPrefix = rnaSequence.substr( 3*matchingAa, 3*(aaLocked-matchingAa) + rnaLockedRest );
					s.rnaUmodifiedOffset = rnaOffset + rnaLockedRest;
				} else {
					s.rnaPrefix = rnaSequence.substr( 3*matchingAa, 3 );
					s.rnaUmodifiedOffset = rnaOffset + 3*(matchingAa-aaLocked) + 3;
				}
				statesAtTheBeginningOfProteinVariants[key].push_back(s);
			}
		}

		// ==== exit, if no further combinations are possible
		if ( aaPosition + matchingAa >= proteinSequence.size() || matchingAa < aaLocked || rnaPositionDifference == 0 ) {
			currentVariants.pop_back();
			return;
		}

		// ==== try to apply RNA variants
		auto iRnaVariant = std::lower_bound( ++variant, transcriptVariants.end(), rnaOffset + rnaLockedRest + 1  // there must be 1 bp break between variants
							, [](PlainSequenceModification const & v, unsigned p)->bool{ return (v.region.left()<p); } );
		unsigned const rnaMatched = rnaOffset + 3*(matchingAa-aaLocked);
		for ( ;  iRnaVariant != transcriptVariants.end() && iRnaVariant->region.left() < rnaMatched+3;  ++iRnaVariant ) {
			unsigned const aaShift = (iRnaVariant->region.left() - rnaOffset) / 3;
			matchRnaVariantToProtein( aaPosition + aaLocked + aaShift, rnaOffset + 3*aaShift, rnaSequence.substr(3*(aaLocked+aaShift)), iRnaVariant, currentVariants );
		}

		// ==== go back
		currentVariants.pop_back();
	}


	void matchRnaVariantToModifiedProtein
		( unsigned aaOffset           // current AA position according to original sequence
		, unsigned aaEndOfVariant     // aa position, where current protein variant ends
		, std::string aaSequence      // AA sequence starting from aaOffset
		, unsigned rnaOffset          // current RNA position according to original sequence, always aligned to left border of current AA offset
		, std::string rnaSequence     // RNA sequence starting from rnaOffset
		, std::vector<PlainSequenceModification>::const_iterator variant                           // variant to apply
		, std::vector< std::vector<PlainSequenceModification>::const_iterator > & currentVariants  // list of already applied variants
		, std::vector< std::vector< std::vector<PlainSequenceModification>::const_iterator >> & solutions // output for solutions
		)
	{
		if (currentVariants.size() > 5) return; // no more than 5 variants
		ASSERT(rnaOffset <= variant->region.left() && variant->region.left() < rnaOffset + 3); // given variant must start in the first codon/aa
		// ==== apply variant
		currentVariants.push_back(variant);
		rnaSequence = applyVariantToTheSequence(rnaSequence, rnaOffset, *variant);
		// this is part of sequence being effect by applying the variant, it cannot contain any other variants
		unsigned const rnaLocked = variant->region.left() + variant->newSequence.size() - rnaOffset;
		unsigned const aaLocked = rnaLocked / 3;
		unsigned const rnaLockedRest = rnaLocked - 3*aaLocked;
		// update aaOffset and rnaOffset
		aaOffset += aaLocked;
		rnaOffset = variant->region.right() - rnaLockedRest;
		// difference between original rna position and current rna position (for the same aa position)
		int const dnaPositionDifference = static_cast<int>(transcriptStartCodon + 3*aaOffset) - static_cast<int>(rnaOffset);

		// ==== current status
		//
		//   variant is drawn as =====
		//
		//       old aaOffset              new aaOffset
		//              |
		// amino-acid   |==========================|===-------
		//              |         aaLocked         |
		//
		//       old rnaOffset             new rnaOffset   <---- rnaOffset is position according to ORIGINAL sequence, not current rnaSequence with applied variants!!!
		//              |                          |
		//        RNA   |--========================|=|--------
        //              |           rnaLocked        |
        //              |                          | |
        //              	                        A
        //                                          |
		//                                   rnaLockedRest (0, 1 or 2)

		// ==== check sequence
		// number of matching aa
		unsigned matchingAa = 0;
		bool solutionFound = false;
		for ( ;  rnaSequence.size() >= 3*(matchingAa+1);  ++matchingAa ) {
			// --- check, if we do not reach the end of protein variant
			if ( matchingAa >= aaSequence.size() ) {
				solutionFound = true;
				break;
			}
			// --- check, if we are not aligned to original sequence
			if ( dnaPositionDifference == 0 && 3*matchingAa >= rnaLocked && aaOffset - aaLocked + matchingAa >= aaEndOfVariant ) { // the rest must match, so we stop here
				solutionFound = true;
				break;
			}
			// --- check current aa
			if ( translateToAminoAcid(rnaSequence.substr(3*matchingAa,3))[0] != aaSequence[matchingAa] ) break;
		}

		// ==== exit if solution was found
		if (solutionFound) {
			solutions.push_back(currentVariants);
			currentVariants.pop_back();
			return;
		}

		// ==== exit, if no further combinations are possible
		if ( matchingAa < aaLocked ) {
			currentVariants.pop_back();
			return;
		}

		// ==== try to apply rna variants
		auto iRnaVariant = std::lower_bound( ++variant, transcriptVariants.end(), rnaOffset + rnaLockedRest + 1  // there must be 1 bp break between variants
							, [](PlainSequenceModification const & v, unsigned p)->bool{ return (v.region.left()<p); } );
		unsigned const rnaMatched = rnaOffset + 3*(matchingAa-aaLocked);
		for ( ;  iRnaVariant != transcriptVariants.end() && iRnaVariant->region.left() < rnaMatched+3;  ++iRnaVariant ) {
			// we have to check variants changing aa/rna shift and all variants from first non-matching codon
			if ( iRnaVariant->region.length() != iRnaVariant->newSequence.size() || iRnaVariant->region.right() > rnaMatched ) {
				unsigned const aaShift = (iRnaVariant->region.left() - rnaOffset) / 3;
				matchRnaVariantToModifiedProtein(aaOffset + aaShift, aaEndOfVariant, aaSequence.substr(aaLocked+aaShift), rnaOffset + 3*aaShift, rnaSequence.substr(3*(aaLocked+aaShift)), iRnaVariant, currentVariants, solutions );
			}
		}

		// ==== go back
		currentVariants.pop_back();
	}


	// return all matching variant combinations
	void matchRnaVariantsToProteinVariants()
	{
		// ==== initialization, set initial state for standard case (no frameshift)
		statesAtTheBeginningOfProteinVariants.clear();
		for (auto const & pv: proteinVariants) {
			// create empty entry
			statesAtTheBeginningOfProteinVariants[positionAndFirstDifferentAminoAcid(pv)].clear();
		}
		outGenomicCauses.clear();
		outGenomicCauses.resize(proteinVariants.size());

		// ==== search for all possible combinations that may cause frameshift before protein variants
		auto iRnaVariant = std::lower_bound( transcriptVariants.begin(), transcriptVariants.end(), transcriptStartCodon
									  , [](PlainSequenceModification const & v, unsigned p)->bool{ return (v.region.left()<p); } );
		unsigned const rnaLastPossibleFrameshift = transcriptStartCodon + 3 * proteinVariants.back().modifications.front().region.left() + 2;
		std::vector< std::vector<PlainSequenceModification>::const_iterator > currentVariants;
		for ( ;  iRnaVariant != transcriptVariants.end() && (iRnaVariant->region.left() <= rnaLastPossibleFrameshift);  ++iRnaVariant ) {
			unsigned const aaOffset = (iRnaVariant->region.left() - transcriptStartCodon) / 3;
			unsigned const rnaOffset = transcriptStartCodon + 3 * aaOffset;
			matchRnaVariantToProtein(aaOffset, rnaOffset, transcriptSequence.substr(rnaOffset), iRnaVariant, currentVariants);
			ASSERT(currentVariants.empty());
		}
//DebugVar(statesAtTheBeginningOfProteinVariants);// return;
		// ==== complete searches for all protein variants
		for ( unsigned i = 0;  i < proteinVariants.size();  ++i ) {
			std::vector< std::vector< std::vector<PlainSequenceModification>::const_iterator >> solutions;
			// --- prepare protein sequence and offsett
			unsigned aaOffset = proteinVariants[i].modifications.front().region.left();
			std::string const aaSequence = applyVariantsToTheSequence(proteinSequence.substr(aaOffset), aaOffset, proteinVariants[i].modifications);
			for (auto const & m: proteinVariants[i].modifications) aaOffset += m.region.length();
			for (auto const & m: proteinVariants[i].modifications) aaOffset -= m.newSequence.size();
			// --- try to start from each matching state
			for (State & s: statesAtTheBeginningOfProteinVariants[positionAndFirstDifferentAminoAcid(proteinVariants[i])]) {
				// --- set base parameters
				std::string rnaSequence = s.rnaPrefix + transcriptSequence.substr(s.rnaUmodifiedOffset);
				// --- calculate number of matching aa
				unsigned aaMatching = 0;
				for ( ;  aaMatching < aaSequence.size() && 3*(aaMatching+1) <= rnaSequence.size();  ++aaMatching ) {
					if ( translateToAminoAcid(rnaSequence.substr(3*aaMatching,3))[0] != aaSequence[aaMatching] ) break;
				}
				// if whole protein was parsed, we have a solution
				if ( aaMatching >= aaSequence.size() ) {
					solutions.push_back(s.appliedRnaVariants);
					continue;
				}
				// --- iterate over RNA variants
				auto iRnaVariant = std::lower_bound( transcriptVariants.begin(), transcriptVariants.end(), s.rnaUmodifiedOffset
												  , [](PlainSequenceModification const & v, unsigned p)->bool{ return (v.region.left()<p); } );
				for ( ;  iRnaVariant != transcriptVariants.end() && iRnaVariant->region.left() < s.rnaUmodifiedOffset + 3*aaMatching + 3 - s.rnaPrefix.size();  ++iRnaVariant ) {
					unsigned const aaShift = (iRnaVariant->region.left() - s.rnaUmodifiedOffset + s.rnaPrefix.size()) / 3;
					matchRnaVariantToModifiedProtein( aaOffset + aaShift, proteinVariants[i].modifications.back().region.right(), aaSequence.substr(aaShift)
													, s.rnaUmodifiedOffset + 3*aaShift - s.rnaPrefix.size(), rnaSequence.substr(3*aaShift), iRnaVariant, s.appliedRnaVariants, solutions);
				}
			}
			// --- save solutions
			for (auto & v: solutions) {
				PlainVariant pv;
				pv.refId = transcriptId;
				for (auto it: v) pv.modifications.push_back(*it);
				outGenomicCauses[i].push_back(pv);
			}
		}
	}

public:
	Solver_SearchForGenomicCauses
		( ReferenceId const pTranscriptId
		, unsigned const pTranscriptStartCodon
		, std::string const & pTranscriptSequence
		, std::vector<PlainSequenceModification> const & pTranscriptVariants
		, std::string const pProteinSequence
		, std::vector<PlainVariant> const & pProteinVariants
		) : transcriptId(pTranscriptId), transcriptStartCodon(pTranscriptStartCodon), transcriptSequence(pTranscriptSequence)
		, transcriptVariants(pTranscriptVariants), proteinSequence(pProteinSequence), proteinVariants(pProteinVariants)
	{}

	void solve() { matchRnaVariantsToProteinVariants(); }

	std::vector< std::vector<PlainVariant> > const & solution() const { return outGenomicCauses; }
};


std::vector< std::vector<PlainVariant> > searchForMatchingTranscriptVariants
	( ReferenceId const transcriptId
	, unsigned const transcriptStartCodon
	, std::string const & transcriptSequence
	, std::vector<PlainSequenceModification> & transcriptVariants
	, std::string const proteinSequence
	, std::vector<PlainVariant> const & proteinVariants
	)
{
	// sort transcript variants
	std::sort( transcriptVariants.begin(), transcriptVariants.end() );
	// sort protein variants
	std::vector<unsigned> proteinVariantsIndexes(proteinVariants.size());
	for (unsigned i = 0; i < proteinVariantsIndexes.size(); ++i) proteinVariantsIndexes[i] = i;
	auto compareByIndex = [&proteinVariants](unsigned i,unsigned j)->bool{ return (proteinVariants[i].modifications < proteinVariants[j].modifications); };
	std::sort( proteinVariantsIndexes.begin(), proteinVariantsIndexes.end(), compareByIndex );
	std::vector<PlainVariant> sortedProteinVariants;
	sortedProteinVariants.reserve(proteinVariants.size());
	for (unsigned i: proteinVariantsIndexes) sortedProteinVariants.push_back(proteinVariants[i]);
	// run solver
	Solver_SearchForGenomicCauses s( transcriptId, transcriptStartCodon, transcriptSequence, transcriptVariants, proteinSequence, sortedProteinVariants );
	s.solve();
	// prepare solution - revert sorting
	std::vector< std::vector<PlainVariant> > results(s.solution().size());
	for (unsigned i = 0; i < proteinVariants.size(); ++i) results[proteinVariantsIndexes[i]] = s.solution()[i];
	return results;
}

std::ostream& operator<<(std::ostream& str, Solver_SearchForGenomicCauses::State const & s)
{
	str << "{[";
	for (unsigned i = 0; i < s.appliedRnaVariants.size(); ++i) {
		if (i) str << ",";
		str << *(s.appliedRnaVariants[i]);
	}
	str << "]," << s.rnaUmodifiedOffset << "," << std::string(s.rnaPrefix) << "}";
	return str;
}
