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
        : ssrc(rand32()), 
        rtcp(connections, ssrc, cname), 
        init(false)
    {
    }

    void stream::open(const ip::address& iface, int& rtp_port, int& rtcp_port)
    {
        connections.open(iface, rtp_port, rtcp_port);
    }

    void stream::start()
    {
        connections.receive_rtp(boost::bind(
            &stream::rtp_received, this, _1));

        connections.receive_rtcp(boost::bind(
            &stream::rtcp_received, this, _1));

        connections.start();
    }

    void stream::start(const ip::udp::endpoint rtp_peer, const ip::udp::endpoint rtcp_peer)
    {
        connections.set_rtp_peer(rtp_peer);
        connections.set_rtcp_peer(rtcp_peer);

        connections.receive_rtp(boost::bind(
            &stream::rtp_received, this, _1));

        connections.receive_rtcp(boost::bind(
            &stream::rtcp_received, this, _1));

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
        if (init)
        {
	        // When we receive the first RTP packet from the peer, save the
	        // timestamp so that it can be used to determine the time deltas
	        // for future packets.  This is needed in order to generate our own
	        // timestamps with the same intervals, but using a different
	        // (random) start time.
            remote_start_time = pkt.get_timestamp();
            init = false;
        }

        // Generate a local timestamp
        pkt.set_timestamp(connections.get_rtp_start() + 
            (pkt.get_timestamp() - remote_start_time));

        // Change the SSRC to our identifier
        pkt.set_ssrc(ssrc);
        
        // Send the packet back to the peer
        connections.send_rtp(pkt);

        // Notify RTCP so it can update its sender stats.
        rtcp.rtp_sent(pkt);
    }
}
