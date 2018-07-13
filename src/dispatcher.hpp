#ifndef DISPATCHER_HPP_
#define DISPATCHER_HPP_

#include <vector>
#include <memory>
#include "referencesDatabase/referencesDatabase.hpp"
#include "httpp/http/Protocol.hpp"


class Dispatcher
{
private:
	struct Pim;
	Pim * pim;
public:
	// constructor & destructor
	Dispatcher(Configuration const & configuration);
	~Dispatcher();
	// not copyable, not movable
	Dispatcher(Dispatcher const &) = delete;
	Dispatcher(Dispatcher &&) = delete;
	Dispatcher& operator=(Dispatcher const &) = delete;
	Dispatcher& operator=(Dispatcher &&) = delete;
	// methods
	void processRequest
		( std::string const & fullUrl
		, HTTPP::HTTP::Method const & httpMethod               // GET, POST, PUT etc.
		, std::string const & httpPath                         // path from URL (like /xxx/yyy.html, no host)
		, std::vector<HTTPP::HTTP::KV> const & parameters      // parameters from URL
		, std::shared_ptr<std::vector<char>> body              // request body, nullptr if none
		, HTTPP::HTTP::HttpCode & httpStatus                   // output: response status
		, std::string & contentType                            // output: set value of the content-type here
		, std::function<std::string()> & callbackNextBodyChunk // output: callback producing response body
		, std::string & redirected                             // output: set if request should be redirected
		);
};

#endif /* DISPATCHER_HPP_ */
