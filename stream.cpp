#include "stdafx.h"

#include "media.h"
#include "stream.h"
#include "rtp_packet.h"
#include "rtcp_packet.h"
#include <boost\bind.hpp>
#include "log.h"

namespace media
{
    stream::stream(io_service& io, uint32_t ssrc)
        : io(io), rtp(io), rtcp(io, ssrc), rtcp_timer(io), ssrc(ssrc)
    {
    }

    void stream::open(const ip::address& iface, int& rtp_port, int& rtcp_port)
    {
        if (rtp_port == 0)
        {
            for (int i = 0; i < 10; i++)
            {
                rtp.open(iface, rtp_port);

                if (rtcp_port == 0)
                {
                    rtcp_port = rtp_port + 1;
                    if (rtcp.try_open(iface, rtcp_port))
                        return;
                }
                else rtcp.open(iface, rtcp_port);

                rtp_port += 2;
            }

            throw std::exception("Failed to allocate RTP/RTCP port pair");
        }
        else rtp.open(iface, rtp_port);

        if (rtcp_port == 0)
        {
            rtcp_port = rtp_port + 1;
            if (!rtcp.try_open(iface, rtcp_port))
            {
                rtcp_port = 0;
                rtcp.open(iface, rtcp_port);
            }
        }
        else rtcp.open(iface, rtcp_port);
    }

    void stream::start()
    {
        timer_started = false;
        start_rtp_receive();
        start_rtcp_receive();

        rtp_start_time = rand32();
        ntp_start_time = get_ntp_time();
    }

    void stream::start(const ip::udp::endpoint rtp_peer, const ip::udp::endpoint rtcp_peer)
    {
        rtp.set_remote(rtp_peer);
        rtcp.set_remote(rtcp_peer);

        start_rtp_receive();
        start_rtcp_receive();
        start_rtcp_timer();

        rtp_start_time = rand32();
        ntp_start_time = get_ntp_time();
    }

    uint32_t stream::get_rtp_time(uint64_t ntp_time)
    {
        return rtp_start_time + (uint32_t)(ntp_time - ntp_start_time);
    }

    void stream::start_rtcp_timer()
    {
        timer_started = true;
        rtcp_timer.expires_from_now(boost::posix_time::milliseconds(rtcp.get_send_time()));
        rtcp_timer.async_wait(boost::bind(&stream::rtcp_timer_expired, this, placeholders::error));
    }

    void stream::stop()
    {
        rtcp_timer.cancel();
        rtp.stop();
        rtcp.stop();
    }

    void stream::rtcp_timer_expired(const boost::system::error_code& ec)
    {
        if (!ec)
        {
            auto ntp_time = get_ntp_time();
            auto rtp_time = get_rtp_time(ntp_time);
            rtcp.send_packet(ntp_time, rtp_time);
        }
        start_rtcp_timer();
    }

    void stream::start_rtp_receive()
    {
        rtp.async_receive(boost::bind(&stream::rtp_received, this, _1));
    }

    void stream::start_rtcp_receive()
    {
        rtcp.async_receive(boost::bind(&stream::rtcp_received, this, _1));
    }

    void stream::rtp_received(rtp_packet& pkt)
    {
        loop_rtp_packet(pkt);
        start_rtp_receive();
    }

    void stream::rtcp_received(rtcp_packet& pkt)
    {
        LOG("RTCP received\n");
        if (!timer_started) start_rtcp_timer();
        start_rtcp_receive();
    }

    void stream::stop_request_received()
    {
        rtp.stop();
        rtcp.stop();
    }

    void stream::loop_rtp_packet(rtp_packet& pkt)
    {
        pkt.set_timestamp(rtp_start_time + 
            (pkt.get_timestamp() - remote_start_time));

        pkt.set_ssrc(ssrc);
        
        rtp.send(pkt);
        rtcp.rtp_sent(pkt);
    }
}