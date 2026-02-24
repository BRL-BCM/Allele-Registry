#include <string>
#include <iostream>

#include <boost/interprocess/ipc/message_queue.hpp>
#include <boost/archive/text_iarchive.hpp>

#include "info.hpp"

using namespace boost::interprocess;

int main ()
{
    try
    {  
        message_queue mq
            (
             open_only,
             "mq"
            );
        message_queue::size_type recvd_size;
        unsigned int priority;

        sharedDataEntry me;

        std::stringstream iss;
        std::string serialized_string;
        serialized_string.resize(MAX_SIZE);
        mq.receive(&serialized_string[0], MAX_SIZE, recvd_size, priority);
        iss << serialized_string;

        boost::archive::text_iarchive ia(iss);
        ia >> me;

        for(auto const & d: me.getData()) {
            std::cout << d.getType() << "\t" << d.getId() << std::endl;
        }
    }
    catch(interprocess_exception &ex)
    {  
        std::cerr << ex.what() << std::endl;
    }

    //message_queue::remove("mq");
}
