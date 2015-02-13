#include "stdafx.h"

#include "stack.h"
#include "rtp_packet.h"
#include "rtcp_packet.h"
#include <boost\bind.hpp>
#include "log.h"

namespace media
{
    stack::stack(ip::udp::endpoint rtp_endpoint, ip::udp::endpoint rtcp_endpoint)
        : rtp(io, rtp_endpoint), rtcp(io, rtcp_endpoint), rtcp_timer(io)
    {
        start_rtp_receive();
        start_rtcp_receive();
        start_rtcp_timer();

        ssrc = rand();
        rtp_start_time = rand();
        ntp_start_time = get_ntp_time();
    }

    void stack::run()
    {
        LOG("Starting RTP stack\n");
        io.run();
    }

    uint32_t stack::get_rtp_time(uint64_t ntp_time)
    {
        return rtp_start_time + (uint32_t)(ntp_time - ntp_start_time);
    }

    void stack::start_rtcp_timer()
    {
        rtcp_timer.expires_from_now(boost::posix_time::milliseconds(rtcp.get_send_time()));
        rtcp_timer.async_wait(boost::bind(&stack::rtcp_timer_expired, this, placeholders::error));
    }

    void stack::stop()
    {
        rtcp_timer.cancel();
        rtp.stop();
        rtcp.stop();
    }

    void stack::rtcp_timer_expired(const boost::system::error_code& ec)
    {
        if (!ec)
        {
            auto ntp_time = get_ntp_time();
            auto rtp_time = get_rtp_time(ntp_time);
            rtcp.send_packet(ntp_time, rtp_time);
        }
    }

    void stack::start_rtp_receive()
    {
        rtp.async_receive(boost::bind(&stack::rtp_received, this, _1));
    }

    void stack::start_rtcp_receive()
    {
        rtcp.async_receive(boost::bind(&stack::rtcp_received, this, _1));
    }

    void stack::rtp_received(rtp_packet& pkt)
    {
        loop_rtp_packet(pkt);
        start_rtp_receive();
    }

    void stack::rtcp_received(rtcp_packet& pkt)
    {
        start_rtcp_receive();
    }

    void stack::stop_request_received()
    {
        rtp.stop();
        rtcp.stop();
    }

    void stack::loop_rtp_packet(rtp_packet& pkt)
    {
        pkt.set_timestamp(rtp_start_time + 
            (pkt.get_timestamp() - remote_start_time));

        pkt.set_ssrc(ssrc);
        
        rtp.send(pkt);
        rtcp.rtp_sent(pkt);
    }
}