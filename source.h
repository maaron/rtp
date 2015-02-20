#pragma once

#include "media.h"
#include "stream.h"
#include <boost\asio.hpp>
#include <memory>

namespace media
{
    // This class represents a media source, which contains 1 or more media
    // streams.  This class provides a boost::asio::io_service object for
    // performing all the necessary IO, along with a number of threads bound to
    // it.  The current implementation creates one thread for each CPU.
    class source
    {
        boost::asio::io_service io;
        std::list<std::shared_ptr<stream> > streams;
        std::vector<boost::thread> io_threads;

    public:
        source();

        // Open RTP and RTCP sockets using the specified ports.  If either 
        // of rtp_port/rtcp_port are 0, upon return they will be set to the 
        // port that was allocated.
        void open_stream(const boost::asio::ip::address& iface, const char* cname, int& rtp_port, int& rtcp_port);

        // Starts media loopback with RTCP on all streams.  RTCP won't send 
        // the first packet until one is received from the peer (that's when 
        // the remote endpoint will be known).
        void start();

        // Stops all streams.
        void stop();

    private:
        
    };
}
