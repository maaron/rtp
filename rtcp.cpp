#include "stdafx.h"

#include "rtcp.h"
#include "rtcp_packet.h"
#include "rtp_packet.h"
#include <boost\bind.hpp>

namespace media
{
    rtcp::rtcp(io_service& io, uint32_t ssrc)
        : io(io), local(io), ssrc(ssrc),
        bytes_sent(0),
        packets_sent(0),
        bytes_received(0),
        packets_received(0),
        remote_ssrc(0),
        fraction_lost(0),
        packets_lost(0),
        highest_sequence(0),
        jitter(0),
        last_sr(0),
        delay_last_sr(0)
    {
    }

    void rtcp::open(const ip::address& iface, int& port)
    {
        ip::udp::endpoint ep(iface, port);
        local.open(ep.protocol());
        local.bind(ip::udp::endpoint(iface, port));
        port = local.local_endpoint().port();
    }

    bool rtcp::try_open(const ip::address& iface, int port)
    {
        boost::system::error_code ec;
        ip::udp::endpoint ep(iface, port);
        local.open(ep.protocol());
        local.bind(ep, ec);
        return !ec;
    }

    void rtcp::set_remote(const ip::udp::endpoint& ep)
    {
        remote = ep;
    }

    void rtcp::stop()
    {
        local.close();
    }

    void rtcp::rtp_sent(rtp_packet& pkt)
    {
        bytes_sent += pkt.payload_size();
        packets_sent++;
    }

    void rtcp::rtp_received(rtp_packet& pkt)
    {
        bytes_received += pkt.payload_size();
        packets_received++;
    }

    int rtcp::get_send_time()
    {
        return 2500;
    }

    void rtcp::send_packet(uint64_t ntp_time, uint32_t rtp_time)
    {
        if (remote.port() == 0) return;

        char buf[2048];
        rtcp_packet pkt(buf, sizeof(buf));

        pkt.write_header();

        pkt.write_sender_info(ssrc,
            ntp_time, 
            rtp_time, 
            bytes_sent, 
            packets_sent);

        pkt.write_sender_block(remote_ssrc, 
            fraction_lost,
            packets_lost,
            highest_sequence,
            jitter,
            last_sr,
            delay_last_sr);

        local.send_to(buffer(buf, pkt.size()), remote);
    }
}