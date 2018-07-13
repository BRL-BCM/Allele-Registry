#include "dispatcher.hpp"
#include <iostream>

struct Dispatcher::Pim {};

Dispatcher::Dispatcher(Configuration const & conf) : pim(nullptr) {}

Dispatcher::~Dispatcher() {}

void Dispatcher::processRequest
	( std::string const & fullUrl                          // fullUrl (original)
	, HTTPP::HTTP::Method const & httpMethod               // GET, POST, PUT etc.
	, std::string const & httpPath                         // path from URL (like /xxx/yyy.html, no host)
	, std::vector<HTTPP::HTTP::KV> const & parameters      // parameters from URL
	, std::shared_ptr<std::vector<char>> body              // request body, nullptr if none
	, HTTPP::HTTP::HttpCode & httpStatus                   // response status
	, std::string & contentType
	, std::function<std::string()> & callbackNextBodyChunk // output: callback producing response body
	, std::string & requests
	)
{
	std::cout << "Method: " << HTTPP::HTTP::to_string(httpMethod) << std::endl;
	std::cout << "Path: " << httpPath << std::endl;
	std::cout << "Parameters:\n";
	for (auto const & kv: parameters) std::cout << "  ->" << kv.first << "=" << kv.second << std::endl;
	unsigned const bodySize = (body != nullptr) ? (body->size()) : (0);
	std::cout << "Body size: " << bodySize << std::endl;
	httpStatus = HTTPP::HTTP::HttpCode::Ok;
	contentType = "text/plain";
	if (bodySize == 0) {
		callbackNextBodyChunk = []()->std::string { return ""; };
	} else {
		callbackNextBodyChunk = [=]()->std::string
			{
				unsigned chunkSize = 4123123;
				if (body->size() < chunkSize) chunkSize = body->size();
				std::string const r( &(*body)[0], &(*body)[0] + chunkSize );
				body->erase( body->begin(), body->begin()+chunkSize );
				return r;
			};
	}
}
