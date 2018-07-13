#include "mysqlConnection.hpp"
#include <mysql.h>
#include <errmsg.h>
#include <thread>
#include <mutex>
#include <chrono>



struct MySqlConnection::Pim
{
	static bool initialized;
	static std::mutex initialized_synch;
	MYSQL mysql;
	MYSQL_RES * result = nullptr;
	MYSQL_ROW row = nullptr;
	unsigned long * fieldsLengths = nullptr;
	MySqlConnectionParameters const params;

	Pim(MySqlConnectionParameters const & pParams) : params(pParams) {}

	void throwMySqlException(std::string const & msg)
	{
		throw std::runtime_error(msg + ". The error read from mysql driver: " + mysql_error(&(mysql)));
	}

	void connectToDatabase()
	{
		unsigned numberOfTries = 60; // wait max for 1 minute, TCP_WAIT state is usually set to 60 seconds
		std::string const localhost = "localhost";
		std::string const ip127_0_0_1 = "127.0.0.1";
		char const * host   = (params.port == 0) ? (localhost   .c_str()) : (params.hostOrSocket.c_str());
		char const * socket = (params.port != 0) ? (nullptr             ) : (params.hostOrSocket.c_str());
		if (params.port > 0 && params.hostOrSocket == "localhost") host = ip127_0_0_1.c_str();  // mysql switch to socket when it sees 'localhost'
		while ( mysql_real_connect( &(mysql), host, params.user.c_str(), params.password.c_str(), params.dbName.c_str(), params.port, socket, 0 ) == nullptr ) {
			if ( mysql_errno(&(mysql)) == CR_CONN_HOST_ERROR && numberOfTries > 0 ) {
				--numberOfTries;
				std::this_thread::sleep_for(std::chrono::seconds(1));
				continue;
			}
			throwMySqlException("Cannot connect to the database");
		}
	}
};

bool MySqlConnection::Pim::initialized = false;
std::mutex MySqlConnection::Pim::initialized_synch;


MySqlConnection::MySqlConnection(MySqlConnectionParameters const & params)
: pim(new Pim(params))
{
	{ // ---- global initialization of MySql library, if not yet initialized
		std::lock_guard<std::mutex> guard(Pim::initialized_synch);
		if (! Pim::initialized) {
			mysql_library_init(0, NULL, NULL);
			Pim::initialized = true;
		}
	}
	// ---- init driver "session"
	if ( mysql_init(&(pim->mysql)) == nullptr ) {
		delete pim;
		throw std::runtime_error("Could not initialize MySQL driver");
	}
	// ---- connect to database
	try {
		pim->connectToDatabase();
	} catch (...) {
		mysql_close( &(pim->mysql) );
		mysql_thread_end();
		delete pim;
		throw;
	}
}

MySqlConnection::SP MySqlConnection::connect(MySqlConnectionParameters const & params)
{
	return SP(new MySqlConnection(params));
}

MySqlConnection::~MySqlConnection()
{
	if ( pim->result != nullptr ) {
		mysql_free_result(pim->result);
	}
	mysql_close( &(pim->mysql) );
	mysql_thread_end();
	delete pim;
}

// return true if there are some query result to fetch
bool MySqlConnection::execute(std::string const & sqlQuery)
{
	// free result from last query, if not freed yet
	if ( pim->result != nullptr ) {
		mysql_free_result(pim->result);
		pim->result = nullptr;
		pim->row = nullptr;
	}
	// execute a query
	unsigned errorCode = 0;
	if ( mysql_real_query( &(pim->mysql), sqlQuery.data(), sqlQuery.size() ) ) {
		errorCode = mysql_errno(&(pim->mysql));
	}
	if ( errorCode == CR_SERVER_GONE_ERROR || errorCode == CR_SERVER_LOST ) {
		// reconnect and repeat query
		mysql_close( &(pim->mysql) );
		pim->connectToDatabase();
		errorCode = abs( mysql_real_query( &(pim->mysql), sqlQuery.data(), sqlQuery.size() ) );
	}
	if (errorCode != 0) {
		pim->throwMySqlException("Failed to execute a query: " + sqlQuery);
	}

	pim->result = mysql_use_result( &(pim->mysql) );
	if (pim->result == nullptr) {
		if (mysql_errno( &(pim->mysql) )) pim->throwMySqlException("Failed to execute mysql_use_result");
		return false;
	}
	return true;
}

unsigned MySqlConnection::autoIdFromLastStatement()
{
	 return mysql_insert_id( &(pim->mysql) );
}

// true - row fetched, false - end of results
bool MySqlConnection::fetchRow()
{
	if (pim->result == nullptr) return false;
	pim->row = mysql_fetch_row( pim->result );
	if (pim->row != nullptr) {
		pim->fieldsLengths = mysql_fetch_lengths(pim->result);
		if (pim->fieldsLengths == nullptr) {
			pim->row = nullptr;
			pim->throwMySqlException("Error when fetching fields lengths");
		}
		return true;
	}
	if (mysql_errno( &(pim->mysql) )) pim->throwMySqlException("Error when fetching data");
	mysql_free_result(pim->result);
	pim->result = nullptr;
	return false;
}

unsigned MySqlConnection::numberOfFields() const
{
	if ( pim->result == nullptr ) throw std::logic_error("There is no active query in MySql driver context - numberOfFields() called for nullptr");
	return mysql_num_fields(pim->result);
}

// true - value saved in parameter, false - NULL value, parameter has not been modified
bool MySqlConnection::parseField(unsigned index, std::string & out) const
{
	if ( pim->row == nullptr ) throw std::logic_error("There is no active row in MySql driver context - parseField() called for nullptr");
	if ( pim->row[index] == nullptr ) return false;
	out = pim->row[index];
	return true;
}

bool MySqlConnection::parseField(unsigned index, unsigned & out) const
{
	if ( pim->row == nullptr ) throw std::logic_error("There is no active row in MySql driver context - parseField() called for nullptr");
	if ( pim->row[index] == nullptr ) return false;
	out = 0u;
	for ( char const * p = pim->row[index];  *p != '\0';  ++p ) {
		if (*p < '0' || *p > '9') {
			throw std::logic_error("Incorrect value type, unsigned integer expected, got " + std::string(pim->row[index]));
		}
		out *= 10u;
		out += (*p - '0');
	}
	return true;
}

bool MySqlConnection::parseField(unsigned index, std::vector<uint8_t> & out) const
{
	if ( pim->row == nullptr ) throw std::logic_error("There is no active row in MySql driver context - parseField() called for nullptr");
	if ( pim->row[index] == nullptr ) return false;
	out.clear();
	out.reserve(pim->fieldsLengths[index]);
	for ( char const * p = pim->row[index];  p != pim->row[index] + pim->fieldsLengths[index];  ++p ) {
		out.push_back(static_cast<uint8_t>(*p));
	}
	return true;
}

std::string MySqlConnection::escapeString(std::string const & s)
{
	char * buf = new char[2*s.length()+1];
	mysql_real_escape_string(&(pim->mysql), buf, s.data(), s.length());
	std::string const out = buf;
	delete [] buf;
	return out;
}

