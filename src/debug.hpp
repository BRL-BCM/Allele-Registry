#ifndef DEBUG_HPP_
#define DEBUG_HPP_

#include <iostream>
#include <vector>
#include <map>
#include <thread>

#define DOUT(xxx) std::cout << (std::this_thread::get_id()) << ": " << (xxx) << std::endl;
#define DERR(xxx) std::cerr << (xxx) << std::endl;

//template<typename tContainer>
//std::ostream & operator<<(std::ostream & str, tContainer const v)
//{
//	str << "[";
//	for (auto it = v.begin(); it != v.end(); ++it) {
//		if (it != v.begin()) str << ",";
//		str << *it;
//	}
//	str << "]";
//  return str;
//}

template<typename tValue>
std::ostream & operator<<(std::ostream & str, std::vector<tValue> const & v)
{
	str << "[";
	for (auto it = v.begin(); it != v.end(); ++it) {
		if (it != v.begin()) str << ",";
		str << *it;
	}
	str << "]";
	return str;
}

template<typename tKey,typename tValue>
std::ostream & operator<<(std::ostream & str, std::map<tKey,tValue> const & v)
{
	str << "{";
	for (auto it = v.begin(); it != v.end(); ++it) {
		if (it != v.begin()) str << ",";
		str << it->first << "=>" << it->second;
	}
	str << "}";
	return str;
}

template<typename tKey,typename tValue>
std::ostream & operator<<(std::ostream & str, std::pair<tKey,tValue> const & v)
{
	str << "(" << v.first << "," << v.second << ")";
	return str;
}

#define DebugVar(x) { std::cout << #x << "=" << (x) << std::endl; }

#define DebugScope(x) DebugScopeObj debugScopeObj(x);

struct DebugScopeObj {
	std::string fName;
	DebugScopeObj(std::string name) : fName(name)
	{
		std::cout << "Enter scope " << fName << std::endl;
	}
	~DebugScopeObj()
	{
		std::cout << "Leave scope " << fName << std::endl;
	}
};

#endif /* DEBUG_HPP_ */
