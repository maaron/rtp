#include "stdafx.h"

#include <algorithm>
#include "media.h"
#include "stream.h"
#include "rtp_packet.h"
#include "rtcp_packet.h"
#include <boost\bind.hpp>
#include "log.h"

namespace media
{
    stream::stream(const char* cname)
        : ssrc(rand32()), rtcp(connections, ssrc, cname)
    {
    }

    void stream::open(const ip::address& iface, int& rtp_port, int& rtcp_port)
    {
        connections.open(iface, rtp_port, rtcp_port);
    }

    void stream::start()
    {
        start_rtp_receive();
        start_rtcp_receive();
        connections.start();
    }

    void stream::start(const ip::udp::endpoint rtp_peer, const ip::udp::endpoint rtcp_peer)
    {
        connections.set_rtp_peer(rtp_peer);
        connections.set_rtcp_peer(rtcp_peer);

        start_rtp_receive();
        start_rtcp_receive();
        connections.start();
    }

    void stream::bye()
    {
        connections.get_io_service().post(boost::bind(
            &rtcp::bye, &rtcp));
    }

    void stream::stop()
    {
        connections.stop();
    }

    void stream::start_rtp_receive()
    {
        connections.receive_rtp(boost::bind(&stream::rtp_received, this, _1));
    }

    void stream::start_rtcp_receive()
    {
        connections.receive_rtcp(boost::bind(&stream::rtcp_received, this, _1));
    }

    void stream::rtp_received(rtp_packet& pkt)
    {
        rtcp.rtp_received(pkt);
        loop_rtp_packet(pkt);
        start_rtp_receive();
    }

    void stream::rtcp_received(rtcp_packet& pkt)
    {
        rtcp.rtcp_received(pkt);
        start_rtcp_receive();
    }

    void stream::loop_rtp_packet(rtp_packet& pkt)
    {
        pkt.set_timestamp(connections.get_rtp_start() + 
            (pkt.get_timestamp() - remote_start_time));

        pkt.set_ssrc(ssrc);
        
        connections.send_rtp(pkt);
        rtcp.rtp_sent(pkt);
    }
}