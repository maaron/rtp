#pragma once

#include "media.h"
#include "rtcp.h"
#include "rtp.h"
#include <boost\asio.hpp>

namespace media
{
    using namespace boost::asio;

    class stack
    {
        io_service io;
        rtp rtp;
        rtcp rtcp;
        deadline_timer rtcp_timer;
        void rtcp_timer_expired(const boost::system::error_code& ec);

        uint32_t remote_start_time;
        uint64_t ntp_start_time;
        uint32_t rtp_start_time;
        uint32_t ssrc;

    public:
        stack(ip::udp::endpoint rtp_endpoint, ip::udp::endpoint rtcp_endpoint);
        void stop();
        void run();

    private:
        uint32_t get_rtp_time(uint64_t ntp_time);

        void start_rtp_receive();
        void start_rtcp_receive();
        void start_rtcp_timer();
        void loop_rtp_packet(rtp_packet&);
        void loop_rtcp_packet(rtcp_packet&);

        void rtp_received(rtp_packet&);
        void rtcp_received(rtcp_packet&);
        void stop_request_received();
    };
}