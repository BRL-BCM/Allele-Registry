#include "json.hpp"
#include <boost/test/minimal.hpp>


int test_main(int argc, char ** argv)
{

	json::Value a = json::Integer(4);

	json::Array arr;
	arr.push_back("str");
	arr.push_back(a);
	arr.push_back(json::Integer(5));
	arr.push_back(true);

	a = json::Integer(6);

	json::Object obj;
	obj["aaa"] = "str";
	obj["bbb"] = a;
	obj["ccc"] = json::Integer(7);
	obj["ddd"] = false;
	obj["eee"] = arr;

	json::Value doc = obj;
	std::string const out = doc.toString();

	BOOST_CHECK( out == "{\"aaa\":\"str\",\"bbb\":6,\"ccc\":7,\"ddd\":false,\"eee\":[\"str\",4,5,true]}" );

	doc["AAA"] = "aaaa";

	BOOST_CHECK( doc.toString() == "{\"AAA\":\"aaaa\",\"aaa\":\"str\",\"bbb\":6,\"ccc\":7,\"ddd\":false,\"eee\":[\"str\",4,5,true]}" );

	return 0;
}
