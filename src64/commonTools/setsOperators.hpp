#ifndef COMMONTOOLS_SETSOPERATORS_HPP_
#define COMMONTOOLS_SETSOPERATORS_HPP_

#include <vector>
#include <algorithm>
#include <iterator>


	template<class tElement>
	inline std::vector<tElement> set_difference(std::vector<tElement> const & a, std::vector<tElement> const & b)
	{
		std::vector<tElement> r;
		if (&a != &b) std::set_difference( a.begin(), a.end(), b.begin(), b.end(), std::back_inserter(r) );
		return r;
	}

	template<class tElement>
	inline std::vector<tElement> set_union(std::vector<tElement> const & a, std::vector<tElement> const & b)
	{
		std::vector<tElement> r;
		if (&a == &b) {
			r = a;
		} else {
			std::set_union( a.begin(), a.end(), b.begin(), b.end(), std::back_inserter(r) );
		}
		return r;
	}

	template<class tElement>
	inline std::vector<tElement> set_intersection(std::vector<tElement> const & a, std::vector<tElement> const & b)
	{
		std::vector<tElement> r;
		if (&a == &b) {
			r = a;
		} else {
			std::set_intersection( a.begin(), a.end(), b.begin(), b.end(), std::back_inserter(r) );
		}
		return r;
	}


#endif /* COMMONTOOLS_SETSOPERATORS_HPP_ */
