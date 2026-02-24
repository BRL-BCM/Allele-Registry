#include <boost/interprocess/ipc/message_queue.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <iostream>

#include "info.hpp"

using namespace boost::interprocess;

int main ()
{
    message_queue::remove("mq");
    //try
    //{  
    //    message_queue mq
    //        (
    //         open_or_create,
    //         "mq",
    //         100,
    //         MAX_SIZE
    //        );

    //    sharedDataEntry me;
    //    me.add(true, 10000);

    //    std::stringstream oss;

    //    boost::archive::text_oarchive oa(oss);
    //    oa << me;

    //    std::string serialized_string(oss.str());
    //    mq.send(serialized_string.data(), serialized_string.size(), 0);
    //}
    //catch(interprocess_exception &ex)
    //{  
    //    std::cerr << ex.what() << std::endl;
    //}
}
