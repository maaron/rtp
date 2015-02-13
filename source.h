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
        io_service io;
        uint32_t ssrc;
        std::list<std::shared_ptr<stream> > streams;

    public:
        source();

        // Open RTP and RTCP sockets using the specified ports.  If either 
        // of rtp_port/rtcp_port are 0, upon return they will be set to the 
        // port that was allocated.
        void open_stream(const ip::address& iface, int& rtp_port, int& rtcp_port);

        // Starts media loopback with RTCP on all streams.  RTCP won't send 
        // the first packet until one is received from the peer (that's when 
        // the remote endpoint will be known).
        void start();

        // Stops all streams.  Upon successful completion, any active call 
        // to run() will return.
        void stop();

        // This should be called from a thread chosen to perform network IO 
        // work.  The stream object doesn't provide it's own thread.  NOT 
        // thread-safe (only call this once).  This method only returns upon 
        // a successful call to the stop() method.
        void run();

    private:
        
    };
}