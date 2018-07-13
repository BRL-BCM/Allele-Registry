#include "mysql/mysqlConnection.hpp"
#include "commonTools/assert.hpp"
#include "core/exceptions.hpp"
#include <memory>

#include <boost/uuid/sha1.hpp>

inline unsigned fromHex(char c)
{
	if (c >= '0' && c <= '9') return (c - '0');
	if (c >= 'A' && c <= 'F') return (c - 'A' + 10);
	if (c >= 'a' && c <= 'f') return (c - 'a' + 10);
	throw ExceptionIncorrectRequest("URL formatted incorrectly: unexpected character after % sign");
}

inline void url_decode(std::string& fragment)
{
	std::string::iterator it1 = fragment.begin();
	std::string::iterator it2 = it1;

	for ( ;  it1 != fragment.end();  ++it1, ++it2 ) {
		if (*it1 == '%') {
			if (++it1 == fragment.end()) break;
			unsigned ascii = fromHex(*it1) * 16;
			if (++it1 == fragment.end()) break;
			ascii += fromHex(*it1);
			*it2 = static_cast<char>(ascii);
		} else {
			*it2 = *it1;
		}
	}
	fragment.erase(it2, fragment.end());
}

inline std::string sha1hex(std::string const & data)
{
	boost::uuids::detail::sha1 sha1;
	unsigned int hash[5];
	sha1.process_bytes(data.data(),data.size());
	sha1.get_digest(hash);
	std::string r(40,' ');
	for ( unsigned i=0;  i<5;  ++i ) {
		unsigned div = 1 << 28;
		for (unsigned j = 0; j < 8; ++j) {
			unsigned const val = hash[i] / div;
			hash[i] %= div;
			div >>= 4;
			r[i*8+j] = (val < 10) ? ('0'+val) : ('a'+(val-10));
		}
	}
	return r;
}

bool authorization(MySqlConnectionParameters const & dbParams, std::string fullUri, std::vector<std::string> const & allowedHosts)
{
	std::string gbLogin;
	std::string gbToken;
	std::string gbTime;

	ASSERT(! fullUri.empty());
	auto it2 = fullUri.end();
	auto it = it2;
	for ( --it;  it != fullUri.begin();  --it ) {
		if (*it == '&' || *it == '?') {
			auto it1 = it;
			++it1;
			if (it1 < it2) {
				std::string s(it1, it2);
				if (s.substr(0,8) == "gbLogin=") {
					gbLogin = s.substr(8);
				} else if (s.substr(0,8) == "gbToken=") {
					gbToken = s.substr(8);
				} else if (s.substr(0,7) == "gbTime=") {
					gbTime = s.substr(7);
				}
			}
			it2 = it;
			if (!(gbLogin.empty() || gbToken.empty() || gbTime.empty())) break;
			if (*it == '?') throw ExceptionIncorrectRequest("The following three parameters must be set together (for authorization): gbLogin, gbToken, gbTime");
		}
	}
	if (*it2 == '?') ++it2;
	fullUri = std::string(fullUri.begin(),it2);

	url_decode(gbLogin);
	url_decode(gbTime);
	url_decode(gbToken);

	unsigned timeSent;
	try {
		timeSent = boost::lexical_cast<unsigned>(gbTime);
	} catch (...) {
		throw ExceptionIncorrectRequest("Parameter gbTime is incorrect. It must be unsigned integer - number of seconds since epoch");
	}
	unsigned timeNow = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
	if ( std::max(timeNow,timeSent) - std::min(timeNow,timeSent) > 43200 ) { // this value is set in genboree
		throw ExceptionAuthorizationError("The time window for the token has expired. Is your system clock approximately correct? Or did you compose the URL many hours ago?");
	}

	// escape login to use it in SQL query
	std::string sqlGbLogin = "";
	for (auto c: gbLogin) {
		if (c == '\'') sqlGbLogin.push_back('\'');
		sqlGbLogin.push_back(c);
	}

	MySqlConnection::SP db = MySqlConnection::connect(dbParams);
	std::string sql = "SELECT userId, password FROM genboreeuser WHERE name='" + sqlGbLogin + "'";
	std::string psswd = "";
	unsigned userId = 0;
	if ( ! (db->execute(sql) && db->fetchRow() && db->parseField(0,userId) && db->parseField(1,psswd)) ) {
		throw ExceptionAuthorizationError("There is no user with given login");
	}

	std::vector<std::string> urls;
	urls.reserve(allowedHosts.size() * 2);

	for (auto host: allowedHosts) {
		urls.push_back( "http://"  + host + fullUri );
		urls.push_back( "https://" + host + fullUri );
	}

	std::string identity = sha1hex(gbLogin + psswd);
	bool authenticated = false;
	for (auto url: urls) {
		std::string token = sha1hex(url + identity + gbTime);
		if (token == gbToken) {
			authenticated = true;
			break;
		}
	}
	if (!authenticated) throw ExceptionAuthorizationError("Wrong password");

    sql = "SELECT usergroup.groupId FROM usergroup"
    		" INNER JOIN genboreegroup ON (usergroup.groupId = genboreegroup.groupId)"
    		" WHERE usergroup.userId=" + boost::lexical_cast<std::string>(userId)
    		+ " AND genboreegroup.groupName='Registry'" ; // TODO - hardcoded group name
    return ( db->execute(sql) && db->fetchRow() );
}
