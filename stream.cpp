#include "stdafx.h"

#include "media.h"
#include "stream.h"
#include "rtp_packet.h"
#include "rtcp_packet.h"
#include <boost\bind.hpp>
#include "log.h"

namespace media
{
    stream::stream(const char* cname)
        : ssrc(rand32()), rtcp(ssrc, cname), rtcp_timer(io)
    {
    }

    void stream::open(const ip::address& iface, int& rtp_port, int& rtcp_port)
    {
        connections.open(iface, rtp_port, rtcp_port);
    }

    void stream::start()
    {
        timer_started = false;
        start_rtp_receive();
        start_rtcp_receive();

        rtp_start_time = rand32();
        ntp_start_time = get_ntp_time();

        io_thread = boost::thread(boost::bind(&io_service::run, &io));
    }

    void stream::start(const ip::udp::endpoint rtp_peer, const ip::udp::endpoint rtcp_peer)
    {
        connections.first().set_remote(rtp_peer);
        connections.second().set_remote(rtcp_peer);

        start_rtp_receive();
        start_rtcp_receive();
        start_rtcp_timer();

        rtp_start_time = rand32();
        ntp_start_time = get_ntp_time();

        io_thread = boost::thread(boost::bind(&io_service::run, &io));
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

    void stream::bye(const char* reason)
    {
        rtcp.send_bye(reason);
    }

    void stream::stop()
    {
        
    }

    void stream::rtcp_timer_expired(const boost::system::error_code& ec)
    {
        if (!ec)
        {
            auto ntp_time = get_ntp_time();
            auto rtp_time = get_rtp_time(ntp_time);
            char buf[2048];
            rtcp_packet pkt(buf, sizeof(buf));
            rtcp.build_packet(ntp_time, rtp_time, pkt);

            connections.second().send(buf, pkt.compound_size());
        }
        start_rtcp_timer();
    }

    void stream::start_rtp_receive()
    {
        connections.first().async_receive(boost::bind(&stream::rtp_received, this, _1, _2));
    }

    void stream::start_rtcp_receive()
    {
        connections.second().async_receive(boost::bind(&stream::rtcp_received, this, _1, _2));
    }

    void stream::rtp_received(void* data, size_t size)
    {
        rtp_packet pkt(data, size);

        rtcp.rtp_received(pkt);
        loop_rtp_packet(pkt);
        
        start_rtp_receive();
    }

    void stream::rtcp_received(void* data, size_t size)
    {
        rtcp_packet pkt(data, size);
        rtcp.rtcp_received(pkt);

        if (!timer_started) start_rtcp_timer();
        start_rtcp_receive();
    }

    void stream::stop_request_received()
    {
        io.post(boost::bind(&stream::stop_request_received, this));
    }

    void stream::loop_rtp_packet(rtp_packet& pkt)
    {
        pkt.set_timestamp(rtp_start_time + 
            (pkt.get_timestamp() - remote_start_time));

        pkt.set_ssrc(ssrc);
        
        connections.first().send(pkt.data(), pkt.size());
        rtcp.rtp_sent(pkt);
    }
}