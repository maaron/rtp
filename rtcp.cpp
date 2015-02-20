#include "stdafx.h"

#include "media.h"
#include "rtcp.h"
#include "rtcp_packet.h"
#include "rtp_packet.h"
#include <boost\bind.hpp>

namespace media
{
    rtcp::rtcp_peer_info::rtcp_peer_info()
        : bytes_received(0),
        packets_received(0),
        highest_sequence(0),
        jitter(0),
        last_sr(0)
    {
        timestamp_last_sr.seconds = 0;
        timestamp_last_sr.fractional = 0;
    }

    rtcp::rtcp(rtp_service& c, uint32_t ssrc, const char* cname)
        : connection(c),
        ssrc(ssrc), 
        cname(cname),
        bytes_sent(0),
        packets_sent(0),
        init(true)
    {
    }

    rtcp::~rtcp()
    {
        LOG("RTCP closing\n");
    }

    void rtcp::rtp_sent(rtp_packet& pkt)
    {
        bytes_sent += pkt.payload_size();
        packets_sent++;
    }

    void rtcp::rtp_received(rtp_packet& pkt)
    {
        auto it = peers.find(pkt.get_ssrc());
        if (it != peers.end())
        {
            auto& src = it->second;
            
            src.bytes_received += pkt.payload_size();
            src.packets_received++;
            src.highest_sequence = std::max(src.highest_sequence, pkt.get_sequence_number());
            // TODO
        }
    }

    void rtcp::timer_expired()
    {
        send_report();

        connection.start_timer(2000, 
            boost::bind(&rtcp::timer_expired, this));
    }

    void rtcp::bye()
    {
        // TODO
        send_bye();
    }

    void rtcp::rtcp_received(rtcp_packet& pkt)
    {
        if (init)
        {
            connection.start_timer(2000, 
                boost::bind(&rtcp::timer_expired, this));

            init = false;
        }

        // Loop through compound packets
        do
        {
            rtcp_header header;
            pkt.read_header(header);

            if (header.V == 2)
            {
                switch (header.PT)
                {
                case RTCP_SR:
                    sender_report_received(header, pkt); break;

                case RTCP_RR:
                    receiver_report_received(header, pkt); break;

                case RTCP_SDES:
                    sdes_received(header, pkt); break;

                case RTCP_BYE:
                    bye_received(header, pkt); break;
                }
            }
        } while (pkt.move_next());
    }

    void rtcp::sender_report_received(rtcp_header& header, rtcp_packet& pkt)
    {
        sender_report sr;
        pkt.read_sender_report(sr);

        auto& peer = peers[sr.ssrc];

        peer.last_sr = 
            ((sr.ntp_msw << 16) & 0xffff0000) |
            ((sr.ntp_lsw >> 16) & 0x0000ffff);

        peer.timestamp_last_sr = get_ntp_time();
    }

    void rtcp::receiver_report_received(rtcp_header& header, rtcp_packet& pkt)
    {
    }

    void rtcp::sdes_received(rtcp_header& header, rtcp_packet& pkt)
    {
    }

    void rtcp::bye_received(rtcp_header& header, rtcp_packet& pkt)
    {
        for (int i = 0; i < header.RC; i++)
        {
            auto ssrc = pkt.read_ssrc();
            peers.erase(ssrc);
        }
    }

    uint32_t calc_dlsr(ntp_time_t now, ntp_time_t lsr_time)
    {
        uint32_t seconds = now.seconds - lsr_time.seconds;
        uint64_t fractional = now.fractional - lsr_time.fractional;

        return (uint32_t)(seconds * 65536 + 
            (fractional * 65536) / 
            ((uint64_t)std::numeric_limits<uint32_t>::max + 1));
    }

    void rtcp::send_report()
    {
        char buf[2048];
        rtcp_packet pkt(buf, sizeof(buf));

        ntp_time_t ntp_time = get_ntp_time();
        uint32_t rtp_time = connection.get_rtp_time(ntp_time);

        // Sender Report Packet
        sender_report sr;
        sr.ssrc = ssrc;
        sr.ntp_msw = ntp_time.seconds;
        sr.ntp_lsw = ntp_time.fractional;
        sr.rtp_timestamp = rtp_time;
        sr.octet_count = bytes_sent;
        sr.packet_count = packets_sent;
        pkt.write_sender_report(sr);

        // Sender Report Block
        for (auto s = peers.begin(); s != peers.end(); s++)
        {
            report_block b;
            b.ssrc = s->first;
            b.fraction_lost = 0;
            b.cumulative_lost = 0;
            b.extended_seq_received = s->second.highest_sequence;
            b.interarrival_jitter = s->second.jitter;
            b.last_sr = s->second.last_sr;
            b.delay_since_last_sr = calc_dlsr(
                get_ntp_time(), s->second.timestamp_last_sr);

            pkt.write_sender_report_block(b);
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

        connection.send_rtcp(pkt);
    }
}