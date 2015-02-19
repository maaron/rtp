#include "stdafx.h"

#include "media.h"
#include "rtcp.h"
#include "rtcp_packet.h"
#include "rtp_packet.h"
#include <boost\bind.hpp>

namespace media
{
    rtcp::rtcp(connection_pair& c, uint32_t ssrc, const char* cname)
        : connection(c),
        ssrc(ssrc), 
        cname(cname),
        bytes_sent(0),
        packets_sent(0)
    {
    }

    void rtcp::rtp_sent(rtp_packet& pkt)
    {
        bytes_sent += pkt.payload_size();
        packets_sent++;
    }

    void rtcp::rtp_received(rtp_packet& pkt)
    {
        auto src = peers.find(pkt.get_ssrc());
        if (src != peers.end())
        {
            src->second.bytes_received += pkt.payload_size();
            src->second.packets_received++;
        }
    }

    void rtcp::bye()
    {
        // TODO - this is simplified
        send_bye();
    }

    void rtcp::rtcp_received(rtcp_packet& pkt)
    {
        // TODO process this packet
    }

    void rtcp::send_report()
    {
        char buf[2048];
        rtcp_packet pkt(buf, sizeof(buf));

        uint64_t ntp_time = get_ntp_time();
        uint32_t rtp_time = connection.get_rtp_time(ntp_time);

        // Sender Report Packet
        pkt.write_sender_report(ssrc,
            ntp_time, 
            rtp_time, 
            bytes_sent, 
            packets_sent);

        // Sender Report Block
        for (auto s = peers.begin(); s != peers.end(); s++)
        {
            pkt.write_sender_report_block(
                s->first,
                s->second.fraction_lost,
                s->second.packets_lost,
                s->second.highest_sequence,
                s->second.jitter,
                s->second.last_sr,
                s->second.delay_last_sr);
        }

        // Sdes Packet
        pkt.write_sdes(ssrc);
        pkt.write_sdes_cname(cname.c_str());
        pkt.write_sdes_end();

        connection.send_rtcp(pkt);
    }

    void rtcp::send_bye()
    {
        char buf[2048];
        rtcp_packet pkt(buf, sizeof(buf));

        pkt.write_sdes(ssrc);
        pkt.write_sdes_cname(cname.c_str());
        pkt.write_sdes_end();

        pkt.write_bye(ssrc);
        pkt.write_bye_reason(bye_reason.c_str());
        
        connection.send_rtcp(pkt);
    }
}