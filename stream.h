#pragma once

#include "rtp.h"
#include "rtcp.h"
#include <boost\asio.hpp>

namespace media
{
    using namespace boost::asio;

    class rtp_packet;
    class rtcp_packet;

    class stream
    {
        io_service& io;
        rtp rtp;
        rtcp rtcp;
        bool timer_started;
        deadline_timer rtcp_timer;
        void rtcp_timer_expired(const boost::system::error_code& ec);

        uint32_t ssrc;
        uint32_t remote_start_time;
        uint64_t ntp_start_time;
        uint32_t rtp_start_time;

        uint32_t get_rtp_time(uint64_t ntp_time);

        void start_rtp_receive();
        void start_rtcp_receive();
        void start_rtcp_timer();
        void loop_rtp_packet(rtp_packet&);
        void loop_rtcp_packet(rtcp_packet&);

        void rtp_received(rtp_packet&);
        void rtcp_received(rtcp_packet&);
        void stop_request_received();

    public:
        stream(io_service& io, uint32_t ssrc);

        void open(const ip::address& iface, int& rtp_port, int& rtcp_port);
        void start();
        void start(const ip::udp::endpoint rtp_peer, const ip::udp::endpoint rtcp_peer);
        void stop();
    };
}