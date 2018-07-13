#include "../core/variants.hpp"
#include <set>
#include <map>

enum SW_move : uint8_t {
	  noChange  = 0
	, replace   = 1
	, deletion  = 2
	, insertion = 3
	, unknown   = 4
};


struct SW_state_value {
	uint8_t lastMoves = 0;
	uint8_t cost = std::numeric_limits<uint8_t>::max();
	SW_move bestNextMove = SW_move::unknown;
};
//std::ostream & operator<<(std::ostream & s, SW_state_value const & v)
//{
//	s << "(" << std::hex << v.lastMoves << "," << std::dec << v.cost << "," << (int)(v.bestNextMove) << ")";
//	return s;
//}


struct SW_state_coordinates {
	unsigned positionOnOrg;
	int diffRestOriginalModified;
	SW_state_coordinates(unsigned posOrg, int diff) : positionOnOrg(posOrg), diffRestOriginalModified(diff) {}
	// sorting by positionOriginal, positionModified  (ascending)
	inline bool operator<(SW_state_coordinates const & v) const
	{
		if (positionOnOrg == v.positionOnOrg) return (diffRestOriginalModified < v.diffRestOriginalModified);
		return (positionOnOrg < v.positionOnOrg);
	}
};
RELATIONAL_OPERATORS(SW_state_coordinates);

inline unsigned absdiff(unsigned a, unsigned b)
{
	if (a < b) return (b-a);
	return (a-b);
}

//const int8_t maxCost = 7;

inline void checkMove
	( SW_move moveType
	, unsigned positionOnOrg
	, int diffRestOriginalModified
	, unsigned cost
	, std::map<unsigned,std::set<SW_state_coordinates>> & statesToCheck
	, std::vector<std::map<int,SW_state_value>> & availableStates
)
{
	//TODO - some limitations: abs(diffRestOriginalModified) + cost > maxCost ||
	if ( positionOnOrg >= availableStates.size() ) return;
	SW_state_value & value = availableStates[positionOnOrg][diffRestOriginalModified];
	if (value.cost < cost) return;
	if (value.cost > cost) {
		SW_state_coordinates swCoord(positionOnOrg,diffRestOriginalModified);
		// remove old state
		statesToCheck[value.cost].erase( swCoord );
		if (statesToCheck[value.cost].empty()) statesToCheck.erase(value.cost);
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

void calculateBestNextMoves(std::vector<std::map<int,SW_state_value>> & availableStates)
{
	std::set<SW_state_coordinates,std::greater<SW_state_coordinates>> toVisit;
	toVisit.insert( SW_state_coordinates(availableStates.size()-1,0) );
	while ( ! toVisit.empty() ) {
		// pop the first element
		auto coords = *(toVisit.begin());
		toVisit.erase(toVisit.begin());
		// check the element
		SW_state_value & value = availableStates[coords.positionOnOrg][coords.diffRestOriginalModified];
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
				SW_state_value & value2 = availableStates[coords2.positionOnOrg][coords2.diffRestOriginalModified];
				if (value2.bestNextMove > m) {
					if (value2.bestNextMove == SW_move::unknown) toVisit.insert(coords2);
					value2.bestNextMove = static_cast<SW_move>(m);
				}
			}
		}
	}
}

bool tryDescribeAsProteinVariant
	( std::string const & originalSequence
	, std::string const & modifiedSequence
	, unsigned const offset
	, std::vector<PlainSequenceModification> & results
	)
{
	results.clear();
	// TODO - test for inserted sequence
	//	if ( absdiff(originalSequence.size(),modifiedSequence.size()) > maxCost ) return false;

	std::map<unsigned, std::set<SW_state_coordinates>> statesToCheck;  // index = cost
	std::vector<std::map<int,SW_state_value>> availableStates(originalSequence.size()+1);  // index = positionOrg, diff

	{ // start point
		SW_state_coordinates swc( 0, static_cast<int>(originalSequence.size()) - static_cast<int>(modifiedSequence.size()) );
		availableStates[0][swc.diffRestOriginalModified].cost = 0;
		statesToCheck[0].insert(swc);
	}

	while ( ! statesToCheck.empty() ) {
		// -------- get state to check
		unsigned const totalCost = statesToCheck.begin()->first;
		SW_state_coordinates const state = *(statesToCheck.begin()->second.begin());
		statesToCheck.begin()->second.erase(state);
		if (statesToCheck.begin()->second.empty()) statesToCheck.erase(statesToCheck.begin());
		// -------- check if the end was reached
		if (state.positionOnOrg == originalSequence.size() && state.diffRestOriginalModified == 0) {
			calculateBestNextMoves(availableStates);
			unsigned posOrg = 0;
			unsigned posMod = 0;
			int diff = static_cast<int>(originalSequence.size()) - static_cast<int>(modifiedSequence.size());
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
		uint8_t const cost = availableStates[state.positionOnOrg][state.diffRestOriginalModified].cost;
		checkMove( SW_move::deletion , state.positionOnOrg+1, state.diffRestOriginalModified-1, cost+1, statesToCheck, availableStates );
		checkMove( SW_move::insertion, state.positionOnOrg  , state.diffRestOriginalModified+1, cost+1, statesToCheck, availableStates );
		unsigned const positionOnModified = state.positionOnOrg + modifiedSequence.size() + state.diffRestOriginalModified - originalSequence.size();
		if ( state.positionOnOrg < originalSequence.size() && positionOnModified < modifiedSequence.size() ) {
			if (originalSequence[state.positionOnOrg] == modifiedSequence[positionOnModified]) {
				checkMove( SW_move::noChange, state.positionOnOrg+1, state.diffRestOriginalModified, cost , statesToCheck, availableStates );
			} else {
				checkMove( SW_move::replace , state.positionOnOrg+1, state.diffRestOriginalModified, cost+1, statesToCheck, availableStates );
			}
		}
	}
	return false;
}
