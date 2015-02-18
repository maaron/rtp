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
    }

    void source::open_stream(const ip::address& iface, const char* cname, int& rtp_port, int& rtcp_port)
    {
        auto s = std::make_shared<stream>(cname);
        s->open(iface, rtp_port, rtcp_port);
        streams.push_back(s);
    }

    void source::start()
    {
        for (auto it = streams.begin(); it != streams.end(); it++)
        {
            (*it)->start();
        }
    }

    void source::stop()
    {
        for (auto it = streams.begin(); it != streams.end(); it++)
        {
            (*it)->stop();
        }
    }
    
    void source::run()
    {
        LOG("Starting RTP stream\n");
        io.run();
    }

    
}