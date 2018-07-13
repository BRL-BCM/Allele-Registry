/*
 * Part of HTTPP.
 *
 * Distributed under the 3-clause BSD licence (See LICENCE.TXT file at the
 * project root).
 *
 * Copyright (c) 2014 Thomas Sanchez.  All rights reserved.
 *
 */

#include "httpp/utils/URL.hpp"

//#include <curl/curl.h>

namespace HTTPP
{
namespace UTILS
{

inline unsigned fromHex(char c)
{
	if (c >= '0' && c <= '9') return (c - '0');
	if (c >= 'A' && c <= 'F') return (c - 'A' + 10);
	if (c >= 'a' && c <= 'f') return (c - 'a' + 10);
	throw std::runtime_error("Incorrect character after % sign");
}

bool url_decode(std::string& fragment, bool replace_plus)
{
	// ============ my modification
	try {
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
		return true;
	} catch (std::exception const & e) {
		BOOST_LOG_TRIVIAL(error) << "cannot url_decode: " << fragment << " (" << e.what() << ")";
		return false;
	}
	// =============================
//    char* decoded = curl_unescape(fragment.data(), fragment.size());
//    if (decoded)
//    {
//        fragment = decoded;
//        curl_free(decoded);

//        if (replace_plus)
//        {
//            std::replace_if(std::begin(fragment),
//                            std::end(fragment),
//                            [](const char val)
//                            { return val == '+'; },
//                            ' ');
//        }

//        return true;
//    }
//    else
//    {
//        BOOST_LOG_TRIVIAL(error) << "cannot url_decode: " << fragment;
//        return false;
//    }
}

std::string decode(const std::string& fragment, bool replace_plus)
{
    std::string str = fragment;
    if (url_decode(str, replace_plus))
    {
        return str;
    }

    throw std::runtime_error("cannot decode: " + fragment);
}

bool url_encode(std::string& fragment)
{
//    char* encoded = curl_escape(fragment.data(), fragment.size());
//    if (encoded)
//    {
//        fragment = encoded;
//        curl_free(encoded);
        return true;
//    }
//    else
//    {
//        BOOST_LOG_TRIVIAL(error) << "cannot url_encode: " << fragment;
//        return false;
//    }
}

std::string encode(const std::string& fragment)
{
    std::string str = fragment;
    if (url_encode(str))
    {
        return str;
    }

    throw std::runtime_error("cannot decode: " + fragment);
}

} // namespace UTILS
} // namespace HTTPP

