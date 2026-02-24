#ifndef SMITHWATERMAN_HPP_
#define SMITHWATERMAN_HPP_

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
	int cost = std::numeric_limits<int>::max();
	SW_move bestNextMove = SW_move::unknown;
	SW_state_value(uint8_t _lastMoves = 0, int _cost = std::numeric_limits<int>::max()) : lastMoves(_lastMoves), cost(_cost) {};
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

// return true <=> the protein variant can be created
// results contains variant def <=> the function returned true
// results is cleared upon entry
//bool tryDescribeAsProteinVariant
//	( std::string const & originalSequence
//	, std::string const & modifiedSequence
//	, unsigned const offset
//	, std::vector<PlainSequenceModification> & results
//	);

bool tryDescribeAsProteinVariant
	( std::string const & originalSequence
	, std::string const & modifiedSequence
	, unsigned const offset
	, std::vector<PlainSequenceModification> & results
	, uint32_t const _maxCost
	);

bool tryDescribeAsProteinVariant_Full
	( std::string const & originalSequence
	, std::string const & modifiedSequence
	, unsigned const offset
	, std::vector<PlainSequenceModification> & results
	);


#endif /* SMITHWATERMAN_HPP_ */
