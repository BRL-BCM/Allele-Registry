#ifndef COMMONTOOLS_STRINGTOOLS_HPP_
#define COMMONTOOLS_STRINGTOOLS_HPP_

#include <string>
#include <algorithm>
#include "assert.hpp"

// search or cycle in candidate, candidate cannot be empty, candidate.size <= 10K
// return the smallest substring of candidate that is a cycle in candidate
inline std::string calculateCycle(std::string candidate)
{
	static std::vector<unsigned> const primeNumbers = { 2,3,5,7,11,13,17,19,23,29,31,37,41,43,47,53,59,61,67,71,73,79,83,89,97,101,103 };
	ASSERT(! candidate.empty());
	std::vector<unsigned> primesToCheck = primeNumbers;
	std::reverse(primesToCheck.begin(), primesToCheck.end());
	for ( bool divided = true;  divided ; ) {
		divided = false;
		while (primesToCheck.back() * primesToCheck.back() <= candidate.size()) {
			unsigned const prime = primesToCheck.back();
			if (candidate.size() % prime) continue;
			divided = true;
			unsigned const secondFactor = candidate.size() / prime;
			for (unsigned i = 0; i < prime; ++i) {
				for (unsigned j = 0; j < secondFactor; ++j) {
					if (candidate[i*secondFactor+j] != candidate[j]) {
						divided = false;
						i = prime;
						break;
					}
				}
			}
			if (divided) {
				candidate = candidate.substr(0,candidate.size()/prime);
				break;
			} else {
				primesToCheck.pop_back();
			}
		}
	}
	return candidate;
}


inline std::string rotateRight(std::string const & s, unsigned shift)
{
	if (s.empty()) return s;
	shift %= s.size();
	return ( s.substr(s.size()-shift) + s.substr(0,s.size()-shift) );
}


inline std::string rotateLeft(std::string const & s, unsigned shift)
{
	if (s.empty()) return s;
	shift %= s.size();
	return ( s.substr(shift) + s.substr(0,shift) );
}


#endif /* COMMONTOOLS_STRINGTOOLS_HPP_ */
