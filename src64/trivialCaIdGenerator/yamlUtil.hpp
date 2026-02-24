#ifndef YAML_UTIL_HPP
#define YAML_UTIL_HPP

namespace {
	void convert(YAML::Node const & src, json::Value & trg)
	{
		if (src.IsMap()) {
			for ( auto it = src.begin();  it != src.end();  ++it ) {
				convert( it->second, trg[it->first.as<std::string>()] );
			}
		} else if (src.IsSequence()) {
			for ( unsigned i = 0;  i < src.size();  ++i ) {
				json::Value newVal;
				convert( src[i], newVal );
				trg.push_back(newVal);
			}
		} else if (src.IsScalar()) {
//			std::string const s = src.Scalar();
//			unsigned i = 0;
//			while ( i < s.size() && s[i] >= '0' && s[i] <= '9' ) ++i;
//			if ( i > 0 && i == s.size()) {
//				trg = src.as<json::Integer>();
//			} else {
//				trg = src;
//			}
			trg = src.Scalar();
		} else if (src.IsNull()) {
			trg = trg.null;
		}
	}

	json::Value readConfigurationFile(std::string const & path)
	{
		YAML::Node config;
		try {
			config = YAML::LoadFile(path);
		} catch (std::exception const & e) {
			throw std::runtime_error("Cannot read configuration file " + path + ": " + e.what());
		}
		json::Value data;
		convert(config, data);
		return data;
	}

	void extractField( json::Value & node, std::string & target, std::vector<std::string> path )
	{
		ASSERT(! path.empty());
		std::vector<json::Value*> nodes;
		nodes.push_back(&node);
		std::string fullPath = "";
		for (std::string const & s: path) {
			if (! fullPath.empty()) fullPath.append(".");
			if ( nodes.back()->type() != json::ValueType::object ) {
				throw std::runtime_error("The element at the location \"" + fullPath + "\" is not an object");
			}
			fullPath.append(s);
			if ( nodes.back()->asObject().count(s) == 0 ) {
				throw std::runtime_error("The location \"" + fullPath + "\" does not exist");
			}
			nodes.push_back( &(nodes.back()->asObject()[s]) );
		}
		if ( nodes.back()->type() != json::ValueType::string ) {
			throw std::runtime_error("The element at the location \"" + fullPath + "\" is not a string");
		}
		target = nodes.back()->asString();
		nodes.pop_back();
		while (! nodes.empty()) {
			nodes.back()->asObject().erase(path.back());
			if (! nodes.back()->asObject().empty()) break;
			nodes.pop_back();
			path.pop_back();
		}
	}

	void extractField( json::Value & node, unsigned & target, std::vector<std::string> path )
	{
		std::string s;
		extractField( node, s, path );
		unsigned i = 0;
		while ( i < s.size() && s[i] >= '0' && s[i] <= '9' ) ++i;
		if ( i > 0 && i == s.size()) {
			target = std::stoi(s);
		} else {
			std::string fullPath = path.front();
			for ( unsigned i = 1; i < path.size(); ++i ) {
				fullPath.append(".");
				fullPath.append(path[i]);
			}
			throw std::runtime_error("The element at the location \"" + fullPath + "\" is not an unsigned integer");
		}
	}

	void extractField( json::Value & node, std::vector<std::string> & target, std::vector<std::string> path )
	{
		ASSERT(! path.empty());
		std::vector<json::Value*> nodes;
		nodes.push_back(&node);
		std::string fullPath = "";
		for (std::string const & s: path) {
			if (! fullPath.empty()) fullPath.append(".");
			if ( nodes.back()->type() != json::ValueType::object ) {
				throw std::runtime_error("The element at the location \"" + fullPath + "\" is not an object");
			}
			fullPath.append(s);
			if ( nodes.back()->asObject().count(s) == 0 ) {
				throw std::runtime_error("The location \"" + fullPath + "\" does not exist");
			}
			nodes.push_back( &(nodes.back()->asObject()[s]) );
		}
		target.clear();
		if ( nodes.back()->type() != json::ValueType::null ) {
			if ( nodes.back()->type() != json::ValueType::array ) {
				throw std::runtime_error("The element at the location \"" + fullPath + "\" is not an array");
			}
			json::Array & arr = nodes.back()->asArray();
			for ( auto & n: arr ) target.push_back(n.asString());
		}
		nodes.pop_back();
		while (! nodes.empty()) {
			nodes.back()->asObject().erase(path.back());
			if (! nodes.back()->asObject().empty()) break;
			nodes.pop_back();
			path.pop_back();
		}
	}
}

#endif

