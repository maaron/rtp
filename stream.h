#pragma once

#include "udp_connection_pair.h"
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
        io_service io;
        boost::thread io_thread;
        udp_connection_pair connections;
        uint32_t ssrc;
        
        rtcp rtcp;
        
        bool timer_started;
        deadline_timer rtcp_timer;
        void rtcp_timer_expired(const boost::system::error_code& ec);

        uint32_t remote_start_time;
        uint64_t ntp_start_time;
        uint32_t rtp_start_time;

        uint32_t get_rtp_time(uint64_t ntp_time);

        void start_rtp_receive();
        void start_rtcp_receive();
        void start_rtcp_timer();
        void loop_rtp_packet(rtp_packet&);

        void rtp_received(void* data, size_t size);
        void rtcp_received(void* data, size_t size);
        void stop_request_received();
        void run_io();

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
        void bye(const char* reason);

        // Stops the stream by completing/canceling any outstanding 
        // operations, closing sockets, etc.
        void stop();
    };
}