#ifndef AUTHORIZATION_HPP_
#define AUTHORIZATION_HPP_

#include "mysql/mysqlConnection.hpp"

// returns false if user is not in 'Registry' group and true if user is assigned to 'Registry' group
// when login or password does not match it throws exception
bool authorization(MySqlConnectionParameters const & connParams, std::string fullUri, std::vector<std::string> const & allowedHosts);


#endif /* AUTHORIZATION_HPP_ */
