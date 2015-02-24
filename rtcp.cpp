#include "stdafx.h"

#include "media.h"
#include "rtcp.h"
#include "rtcp_packet.h"
#include "rtp_packet.h"
#include <boost\bind.hpp>

#define RTP_SEQ_MOD (1<<16)

namespace media
{
    rtcp::sender::sender()
    {
        init_seq(this, 0);
        timestamp_last_sr.seconds = 0;
        timestamp_last_sr.fractional = 0;
    }

    rtcp::rtcp(rtp_service& c, uint32_t ssrc, const char* cname)
        : connection(c),
        ssrc(ssrc), 
        cname(cname),
        bytes_sent(0),
        packets_sent(0),
        init(true),
        engine((unsigned long)get_ntp_time().fractional)
    {
    }

    rtcp::~rtcp()
    {
    }

    void rtcp::rtp_sent(rtp_packet& pkt)
    {
        bytes_sent += pkt.payload_size();
        packets_sent++;
    }

    void rtcp::rtp_received(rtp_packet& pkt)
    {
        if (!sending_bye)
        {
            uint32_t ssrc = pkt.get_ssrc();
            members[ssrc];

            auto it = senders.find(ssrc);
            if (it == senders.end())
            {
                init_seq(&senders[ssrc], pkt.get_sequence_number());
            }
            else
            {
                update_seq(&it->second, pkt.get_sequence_number());
            }
        }
    }

    void rtcp::timer_expired()
    {
        if (!sending_bye)
        {
            send_report();

            connection.start_timer(2000,
                boost::bind(&rtcp::timer_expired, this));
        }
    }

    void rtcp::bye()
    {
        // TODO - The algorithm described in RFC 3550 6.3.7 is not implemented.  Instead, we just send the BYE immediately.
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
        
        members[sr.ssrc];
        auto it = senders.find(sr.ssrc);
        if (it != senders.end())
        {
            auto& sender = it->second;

            sender.last_sr =
                ((sr.ntp_msw << 16) & 0xffff0000) |
                ((sr.ntp_lsw >> 16) & 0x0000ffff);

            sender.timestamp_last_sr = get_ntp_time();

            for (int i = 0; i < header.RC; i++)
            {
                report_block block;
                pkt.read_report_block(block);
                report_block_received(block);
            }
        }
    }

    void rtcp::receiver_report_received(rtcp_header& header, rtcp_packet& pkt)
    {
        for (int i = 0; i < header.RC; i++)
        {
            report_block block;
            pkt.read_report_block(block);
            report_block_received(block);
        }
    }

    void rtcp::report_block_received(report_block& block)
    {
        members[block.ssrc];
    }

    void rtcp::sdes_received(rtcp_header& header, rtcp_packet& pkt)
    {
    }

    void rtcp::bye_received(rtcp_header& header, rtcp_packet& pkt)
    {
        for (int i = 0; i < header.RC; i++)
        {
            auto ssrc = pkt.read_ssrc();
            senders.erase(ssrc);
            members.erase(ssrc);
        }

        // TODO: This is where we could reschedule the timer, as in RFC 
        // 3550 OnReceive() sample.
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
        for (auto s = senders.begin(); s != senders.end(); s++)
        {
            report_block b;
            b.ssrc = s->first;
            b.fraction_lost = 0;
            b.cumulative_lost = 0;
            b.extended_seq_received = s->second.max_seq | s->second.cycles;
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

    void rtcp::init_seq(sender* s, uint16_t seq)
    {
        s->base_seq = seq;
        s->max_seq = seq;
        s->bad_seq = RTP_SEQ_MOD + 1;   /* so seq == bad_seq is false */
        s->cycles = 0;
        s->received = 0;
        s->received_prior = 0;
        s->expected_prior = 0;
        /* other initialization */
    }

    int rtcp::update_seq(sender* s, uint16_t seq)
    {
        uint16_t udelta = seq - s->max_seq;
        const int MAX_DROPOUT = 3000;
        const int MAX_MISORDER = 100;
        const int MIN_SEQUENTIAL = 2;

        /*
        * Source is not valid until MIN_SEQUENTIAL packets with
        * sequential sequence numbers have been received.
        */
        if (s->probation) {
            /* packet is in sequence */
            if (seq == s->max_seq + 1) {
                s->probation--;
                s->max_seq = seq;
                if (s->probation == 0) {
                    init_seq(s, seq);
                    s->received++;
                    return 1;
                }
            }
            else {
                s->probation = MIN_SEQUENTIAL - 1;
                s->max_seq = seq;
            }
            return 0;
        }
        else if (udelta < MAX_DROPOUT) {
            /* in order, with permissible gap */
            if (seq < s->max_seq) {
                /*
                * Sequence number wrapped - count another 64K cycle.
                */
                s->cycles += RTP_SEQ_MOD;
            }
            s->max_seq = seq;
        }
        else if (udelta <= RTP_SEQ_MOD - MAX_MISORDER) {
            /* the sequence number made a very large jump */
            if (seq == s->bad_seq) {
                /*
                * Two sequential packets -- assume that the other side
                * restarted without telling us so just re-sync
                * (i.e., pretend this was the first packet).
                */
                init_seq(s, seq);
            }
            else {
                s->bad_seq = (seq + 1) & (RTP_SEQ_MOD - 1);
                return 0;
            }
        }
        else {
            /* duplicate or reordered packet */
        }
        s->received++;
        return 1;
    }

    double rtcp::rtcp_interval(int members,
        int senders,
        double rtcp_bw,
        int we_sent,
        double avg_rtcp_size,
        int initial)
    {
        /*
        * Minimum average time between RTCP packets from this site (in
        * seconds).  This time prevents the reports from `clumping' when
        * sessions are small and the law of large numbers isn't helping
        * to smooth out the traffic.  It also keeps the report interval
        * from becoming ridiculously small during transient outages like
        * a network partition.
        */
        double const RTCP_MIN_TIME = 5.;
        /*
        * Fraction of the RTCP bandwidth to be shared among active
        * senders.  (This fraction was chosen so that in a typical
        * session with one or two active senders, the computed report
        * time would be roughly equal to the minimum report time so that
        * we don't unnecessarily slow down receiver reports.)  The
        * receiver fraction must be 1 - the sender fraction.
        */
        double const RTCP_SENDER_BW_FRACTION = 0.25;
        double const RTCP_RCVR_BW_FRACTION = (1 - RTCP_SENDER_BW_FRACTION);
        /* To compensate for "timer reconsideration" converging to a
        * value below the intended average.
        */
        double const COMPENSATION = 2.71828 - 1.5;

        double t;                   /* interval */
        double rtcp_min_time = RTCP_MIN_TIME;
        int n;                      /* no. of members for computation */

        /*
        * Very first call at application start-up uses half the min
        * delay for quicker notification while still allowing some time
        * before reporting for randomization and to learn about other
        * sources so the report interval will converge to the correct
        * interval more quickly.
        */
        if (initial) {
            rtcp_min_time /= 2;
        }
        /*
        * Dedicate a fraction of the RTCP bandwidth to senders unless
        * the number of senders is large enough that their share is
        * more than that fraction.
        */
        n = members;
        if (senders <= members * RTCP_SENDER_BW_FRACTION) {
            if (we_sent) {
                rtcp_bw *= RTCP_SENDER_BW_FRACTION;
                n = senders;
            }
            else {
                rtcp_bw *= RTCP_RCVR_BW_FRACTION;
                n -= senders;
            }
        }

        /*
        * The effective number of sites times the average packet size is
        * the total number of octets sent when each site sends a report.
        * Dividing this by the effective bandwidth gives the time
        * interval over which those packets must be sent in order to
        * meet the bandwidth target, with a minimum enforced.  In that
        * time interval we send one report so this time is also our
        * average time between reports.
        */
        t = avg_rtcp_size * n / rtcp_bw;
        if (t < rtcp_min_time) t = rtcp_min_time;

        /*
        * To avoid traffic bursts from unintended synchronization with
        * other sites, we then pick our actual next report interval as a
        * random number uniformly distributed between 0.5*t and 1.5*t.
        */
        t = t * (drand48() + 0.5);
        t = t / COMPENSATION;
        return t;
    }

    double rtcp::drand48()
    {
        return dist(engine);
    }
}