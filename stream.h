#pragma once

#include "connection_pair.h"
#include "rtcp.h"
#include <boost\asio.hpp>
#include <boost\thread.hpp>

namespace media
{
    using namespace boost::asio;

    class rtp_packet;
    class rtcp_packet;

    class stream
    {
        connection_pair connections;
        uint32_t ssrc;
        
        rtcp rtcp;
        
        void rtcp_timer_expired(const boost::system::error_code& ec);

        uint32_t remote_start_time;

        void start_rtp_receive();
        void start_rtcp_receive();
        void start_rtcp_timer();
        void loop_rtp_packet(rtp_packet&);

        void rtp_received(rtp_packet&);
        void rtcp_received(rtcp_packet&);
        void stop_request_received();

    public:
        stream(const char* cname);

        // Opens RTP and RTCP sockets for a single stream
        void open(const ip::address& iface, int& rtp_port, int& rtcp_port);

        // Starts send/receive operations on all streams.  Any calls to 
        // open() must be made prior to calling this method.
        void start(const ip::udp::endpoint rtp_peer, const ip::udp::endpoint rtcp_peer);

        // Same as above, but doesn't send anything until something is 
        // received from a remote peer (in order to determine the remote 
        // address/port).  This is particularly useful for loopback.
        void start();

        // Schedules an RTCP BYE message to be sent.  The message won't 
        // necessarily be sent upon return of this method, as it occurrs in 
        // the context of another thread.
        void bye();

        // Stops the stream by completing/canceling any outstanding 
        // operations, closing sockets, etc.
        void stop();
    };
}