#include "stdafx.h"

#include "rtcp.h"
#include "rtcp_packet.h"
#include "rtp_packet.h"
#include <boost\bind.hpp>

namespace media
{
    rtcp::rtcp(uint32_t ssrc, const char* cname)
        : ssrc(ssrc), cname(cname),
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

    int rtcp::get_send_time()
    {
        return 2500;
    }

    void rtcp::build_packet(uint64_t ntp_time, uint32_t rtp_time, rtcp_packet& pkt)
    {
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
    }

    void rtcp::send_bye(const char* reason)
    {
        char buf[2048];
        rtcp_packet pkt(buf, sizeof(buf));

        pkt.write_sdes(ssrc);
        pkt.write_sdes_cname(cname.c_str());
        pkt.write_sdes_end();

        pkt.write_bye(ssrc);
        pkt.write_bye_reason(reason);
        
        local.send_to(buffer(buf, pkt.compound_size()), remote);
    }
}