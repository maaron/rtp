#include "stdafx.h"

#include "source.h"
#include "rtp_packet.h"
#include "rtcp_packet.h"
#include <boost\bind.hpp>
#include "log.h"

namespace media
{
    source::source()
    {
        // Create a thread object for each CPU (but don't start them yet)
        io_threads.resize(boost::thread::hardware_concurrency());
    }

    void source::open_stream(const ip::address& iface, const char* cname, int& rtp_port, int& rtcp_port)
    {
        auto s = std::make_shared<stream>(io, cname);
        s->open(iface, rtp_port, rtcp_port);
        streams.push_back(s);
    }

    void source::start()
    {
        // Start streams.  This causes socket/timer operations to begin, but 
        // none will be completed until we start the IO threads below.
        for (auto it = streams.begin(); it != streams.end(); it++)
        {
            (*it)->start();
        }

        // Start IO threads
        for (auto t = io_threads.begin(); t != io_threads.end(); t++)
            *t = boost::thread(boost::bind(&io_service::run, &io));
    }

    void source::stop()
    {
        // Stop streams.  This should cause all socket/timer operations to 
        // be canceled/aborted, but may not actually complete until the IO 
        // threads are joined below.
        for (auto it = streams.begin(); it != streams.end(); it++)
        {
            (*it)->stop();
        }

        // Join IO threads
        for (auto t = io_threads.begin(); t != io_threads.end(); t++)
            t->join();
    }
}