#include "smithWaterman.hpp"

inline void checkMove
	( SW_move moveType
	, int positionOnOrg
	, int diffRestOriginalModified
	, int cost
	, std::vector<std::set<SW_state_coordinates>> & statesToCheck
	, std::vector<std::vector<SW_state_value>> & availableStates
	, uint32_t const _maxCost
)
{
	if ( abs(diffRestOriginalModified) + cost > _maxCost || positionOnOrg >= availableStates.size() ) return;
	SW_state_value & value = availableStates[positionOnOrg][_maxCost + diffRestOriginalModified];
	if (value.cost < cost) return;
	if (value.cost > cost) {
		SW_state_coordinates swCoord(positionOnOrg,diffRestOriginalModified);
		// remove old state
		if (value.cost < statesToCheck.size()) statesToCheck[value.cost].erase( swCoord );
		// add new state
		value.cost = cost;
		value.lastMoves = 0;
		statesToCheck[value.cost].insert( swCoord );
	}
	// save the last move
	value.lastMoves |= static_cast<uint8_t>(1 << moveType);
}

inline void addToResult
	( std::vector<PlainSequenceModification> & results
	, unsigned positionOnOrg
	, std::string const & orgSeq
	, std::string const & newSeq
	)
{
	if ( (! results.empty()) && results.back().region.right() == positionOnOrg ) {
		results.back().region.incRightPosition(orgSeq.size());
		results.back().originalSequence += orgSeq;
		results.back().newSequence += newSeq;
	} else {
		PlainSequenceModification sm;
		sm.region.setLeftAndLength( positionOnOrg, orgSeq.size() );
		sm.newSequence = newSeq;
		sm.originalSequence = orgSeq;
		results.push_back(sm);
	}
}

void calculateNextMoves(std::vector<std::vector<SW_state_value>> & availableStates, uint32_t const _maxCost)
{
	std::set<SW_state_coordinates,std::greater<SW_state_coordinates>> toVisit;
	toVisit.insert( SW_state_coordinates(availableStates.size()-1,0) );
	while ( ! toVisit.empty() ) {
		// pop the first element
		auto coords = *(toVisit.begin());
		toVisit.erase(toVisit.begin());
		// check the element
		SW_state_value & value = availableStates[coords.positionOnOrg][_maxCost + coords.diffRestOriginalModified];
		for (unsigned m = SW_move::noChange; m < SW_move::unknown; ++m ) {
			if ( value.lastMoves & static_cast<uint8_t>(1 << m) ) {
				SW_state_coordinates coords2 = coords;
				switch (m) {
					case SW_move::noChange :
					case SW_move::replace  :
						--(coords2.positionOnOrg);
						break;
					case SW_move::deletion :
						--(coords2.positionOnOrg);
						++(coords2.diffRestOriginalModified);
						break;
					case SW_move::insertion:
						--(coords2.diffRestOriginalModified);
						break;
				}
				SW_state_value & value2 = availableStates[coords2.positionOnOrg][_maxCost + coords2.diffRestOriginalModified];
				if (value2.bestNextMove > m) {
					if (value2.bestNextMove == SW_move::unknown) toVisit.insert(coords2);
					value2.bestNextMove = static_cast<SW_move>(m);
				}
			}
		}
	}
}

bool tryDescribeAsProteinVariant
	( std::string const & __originalSequence
	, std::string const & __modifiedSequence
	, unsigned const offset
	, std::vector<PlainSequenceModification> & results
	, uint32_t const __maxCost
	)
{
	results.clear();

	std::string _originalSequence = __originalSequence;
	std::string _modifiedSequence = __modifiedSequence;
	int minSize = _originalSequence.size();
	if(minSize < _modifiedSequence.size()) minSize = _modifiedSequence.size();
	int matchIndex=0;
	for(int i=0;i<minSize;i++) {
		if(_modifiedSequence[_modifiedSequence.size()-1-i] != _originalSequence[_originalSequence.size()-1-i]) {
			matchIndex = i;
			break;
		}
	}
	std::string originalSequence = _originalSequence.substr(0,_originalSequence.size()-matchIndex);
	std::string modifiedSequence = _modifiedSequence.substr(0,_modifiedSequence.size()-matchIndex);

	uint32_t _maxCost = __maxCost;
	if(originalSequence.size()>modifiedSequence.size()) {
		_maxCost = (originalSequence.size()<_maxCost)?originalSequence.size():_maxCost;
	} else {
		_maxCost = (modifiedSequence.size()<_maxCost)?modifiedSequence.size():_maxCost;
	}

	// ----- special case - modified sequence == ""
	if (modifiedSequence.empty()) {
		if (! originalSequence.empty()) {
			PlainSequenceModification sm;
			sm.originalSequence = originalSequence;
			sm.region.set(offset, offset+originalSequence.size());
			results.push_back(sm);
		}
		return true;
	}

	if ( absdiff(originalSequence.size(),modifiedSequence.size()) > _maxCost ) return false;

	std::vector<std::set<SW_state_coordinates>> statesToCheck(_maxCost+1);  // index = cost
	std::vector<std::vector<SW_state_value>> availableStates(originalSequence.size()+1);  // index = positionOrg, (diff + _maxCost)
	for (auto & e: availableStates) e.resize(2*_maxCost+1);

	{ // start point
		SW_state_coordinates swc( 0, static_cast<int>(originalSequence.size()) - static_cast<int>(modifiedSequence.size()) );
		availableStates[0][_maxCost + swc.diffRestOriginalModified].cost = 0;
		statesToCheck[0].insert(swc);
	}

	for (unsigned totalCost = 0; totalCost < statesToCheck.size(); ++totalCost) {
		while ( ! statesToCheck[totalCost].empty() ) {
			// -------- get state to check
			SW_state_coordinates const state = *(statesToCheck[totalCost].begin());
			statesToCheck[totalCost].erase(state);
			// -------- check if the end was reached
			if (state.positionOnOrg == originalSequence.size() && state.diffRestOriginalModified == 0) {
				calculateNextMoves(availableStates, _maxCost);
				unsigned posOrg = 0;
				unsigned posMod = 0;
				unsigned diff   = _maxCost + static_cast<int>(originalSequence.size()) - static_cast<int>(modifiedSequence.size());
				for ( bool keepGoing = true;  keepGoing; ) {
					switch (availableStates[posOrg][diff].bestNextMove) {
						case SW_move::noChange :
							++posOrg;
							++posMod;
							break;
						case SW_move::replace  :
							addToResult(results, posOrg, originalSequence.substr(posOrg,1), modifiedSequence.substr(posMod,1));
							++posOrg;
							++posMod;
							break;
						case SW_move::deletion :
							addToResult(results, posOrg, originalSequence.substr(posOrg,1), "");
							++posOrg;
							--diff;
							break;
						case SW_move::insertion:
							addToResult(results, posOrg, "", modifiedSequence.substr(posMod,1));
							++posMod;
							++diff;
							break;
						case SW_move::unknown:
							keepGoing = false;
							break;
					}
				}
				// correct results by offset
				for (auto & v: results) {
					v.region.incRightPosition(offset);
					v.region.incLeftPosition(offset);
				}
				return true;
			}
			// -------- check all moves
			int const cost = availableStates[state.positionOnOrg][_maxCost + state.diffRestOriginalModified].cost;
			checkMove( SW_move::deletion , state.positionOnOrg+1, state.diffRestOriginalModified-1, cost+1, statesToCheck, availableStates, _maxCost );
			checkMove( SW_move::insertion, state.positionOnOrg  , state.diffRestOriginalModified+1, cost+1, statesToCheck, availableStates, _maxCost );
			unsigned const positionOnModified = state.positionOnOrg + modifiedSequence.size() + state.diffRestOriginalModified - originalSequence.size();
			if ( state.positionOnOrg < originalSequence.size() && positionOnModified < modifiedSequence.size() ) {
				if (originalSequence[state.positionOnOrg] == modifiedSequence[positionOnModified]) {
					checkMove( SW_move::noChange, state.positionOnOrg+1, state.diffRestOriginalModified, cost , statesToCheck, availableStates, _maxCost );
				} else {
					checkMove( SW_move::replace , state.positionOnOrg+1, state.diffRestOriginalModified, cost+1, statesToCheck, availableStates, _maxCost );
				}
			}
		}
	}
	return false;
}
