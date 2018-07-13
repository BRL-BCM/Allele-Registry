#ifndef MYSQLCONNECTION_HPP_
#define MYSQLCONNECTION_HPP_

#include <string>
#include <vector>
#include <cstdint>
#include <memory>


struct MySqlConnectionParameters
{
	std::string dbName;
	std::string hostOrSocket;
	unsigned    port = 0;
	std::string user = "";
	std::string password = "";
};


// This class may be used by many threads, but particular object can be created and used by single thread only
class MySqlConnection
{
private:
	struct Pim;
	Pim * pim;
	MySqlConnection(MySqlConnectionParameters const &);
public:
	typedef std::shared_ptr<MySqlConnection> SP;
	static SP connect(MySqlConnectionParameters const &);
	~MySqlConnection();
	// return true if there are some query result to fetch
	bool execute(std::string const & sqlQuery);
	unsigned autoIdFromLastStatement();
	// true - row fetched, false - end of results
	bool fetchRow();
	unsigned numberOfFields() const;
	// true - value saved in parameter, false - NULL value, parameter has not been modified
	bool parseField(unsigned index, std::string & out) const;
	bool parseField(unsigned index, unsigned    & out) const;
	bool parseField(unsigned index, std::vector<uint8_t> & out) const;  // uses clear() & reserve()
	// escape string
	std::string escapeString(std::string const &);
};



#endif /* MYSQLCONNECTION_HPP_ */
