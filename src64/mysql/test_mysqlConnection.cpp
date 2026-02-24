#include <iostream>
#include "mysqlConnection.hpp"



int main()
{

	MySqlConnectionParameters connParams;
	connParams.hostOrSocket = "/usr/local/brl/local/var/mysql.sock";
	connParams.dbName = "externalSources";
	connParams.user = "externalSources";
	connParams.password = "externalSources";

	for (unsigned i = 0; i < 10; ++i) {

		MySqlConnection::SP conn = MySqlConnection::connect(connParams);

		conn->execute("SELECT * FROM sources LIMIT 10");

		unsigned const num_fields = conn->numberOfFields();

		while (conn->fetchRow()) {
			for ( unsigned i = 0;  i < num_fields;  ++i ) {
				if (i == 0) {
					unsigned v = 0;
					conn->parseField(i,v);
					std::cout << "   " << v;
				} else {
					std::string v = "NULL";
					conn->parseField(i,v);
					std::cout << "   " << v;
				}
			}
			std::cout << std::endl;
		}
	}

	return 0;
}


/* =============== test for pure driver API
#include <mysql.h>

int main(void)
{
	if ( mysql_library_init(0, NULL, NULL) ) {
		std::cerr << "Could not initialize MySQL library" << std::endl;
		return 1;
	}

	if ( ! mysql_thread_safe() ) {
		std::cerr << "MySQL library is not thread-safe!!!" << std::endl;
		return -1;
	}

	MYSQL m;
	if ( mysql_init(&m) == nullptr ) {
		std::cerr << "Could not initialize MySQL driver" << std::endl;
		return 2;
	}

	if ( mysql_real_connect( &m, "127.0.0.1", "genboree", "genboree", "AlleleRegistry", 16002, NULL, 0 ) == nullptr ) {
		std::cerr << "Failed to connect to database: Error: " << mysql_error(&m) << std::endl;
		return 3;
	}

	std::string const sql = "SELECT * FROM insertions LIMIT 10";
	if ( mysql_real_query( &m, sql.data(), sql.size() ) ) {
		std::cerr << "Failed to execute a query: Error: " << mysql_error(&m) << std::endl;
		return 4;
	}

	MYSQL_RES * result = mysql_use_result(&m);
	if (result == nullptr) {
		std::cerr << "Failed to execute mysql_use_result: Error: " << mysql_error(&m) << std::endl;
		return 5;
	}

	unsigned const num_fields = mysql_num_fields(result);

	for ( MYSQL_ROW row; (row = mysql_fetch_row(result)) != nullptr; ) {
		for ( unsigned i = 0;  i < num_fields;  ++i ) {
			std::cout << "   " << ((row[i] == nullptr) ? "NULL" : row[i]);
		}
		std::cout << std::endl;
	}

	if ( mysql_errno(&m) ) {
		std::cerr << "Error when fetching data: Error: " << mysql_error(&m) << std::endl;
		return 6;
	}

	mysql_close(&m);

	mysql_thread_end();

	mysql_library_end();

	return 0;
}
======================== */
