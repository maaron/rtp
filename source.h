#pragma once

#include "media.h"
#include "stream.h"
#include <boost\asio.hpp>
#include <memory>

namespace media
{
    using namespace boost::asio;

    class source
    {
        std::list<std::shared_ptr<stream> > streams;

    public:
        source();

        // Open RTP and RTCP sockets using the specified ports.  If either 
        // of rtp_port/rtcp_port are 0, upon return they will be set to the 
        // port that was allocated.
        void open_stream(const ip::address& iface, const char* cname, int& rtp_port, int& rtcp_port);

        // Starts media loopback with RTCP on all streams.  RTCP won't send 
        // the first packet until one is received from the peer (that's when 
        // the remote endpoint will be known).
        void start();

        // Stops all streams.  Upon successful completion, any active call 
        // to run() will return.
        void stop();

    private:
        
    };
}