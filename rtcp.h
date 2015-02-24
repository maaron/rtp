#pragma once

#include "rtp_service.h"
#include <stdint.h>
#include <map>
#include <string>
#include <random>

namespace media
{
    class rtp_packet;
    class rtcp_packet;

    class rtcp
    {
        uint32_t ssrc;
        rtp_service& connection;
 
        bool init;
        bool sending_bye;
        std::string cname;
        uint32_t bytes_sent;
        uint32_t packets_sent;

        std::default_random_engine engine;
        std::uniform_real_distribution<double> dist;
            
        struct sender
        {
            // Field from sample implementation in RFC 3550
            uint16_t max_seq;        /* highest seq. number seen */
            uint32_t cycles;         /* shifted count of seq. number cycles */
            uint32_t base_seq;       /* base seq number */
            uint32_t bad_seq;        /* last 'bad' seq number + 1 */
            uint32_t probation;      /* sequ. packets till source is valid */
            uint32_t received;       /* packets received */
            uint32_t expected_prior; /* packet expected at last interval */
            uint32_t received_prior; /* packet received at last interval */
            uint32_t transit;        /* relative trans time for prev pkt */
            uint32_t jitter;         /* estimated jitter */

            // Additional fields needed
            uint32_t last_sr;
            ntp_time_t timestamp_last_sr;

            sender();
        };

        struct member
        {
            // The sample code in RFC 3550 doesn't seem to indicate any
            // particular information to be tracked for non-senders.  For our
            // requirements, this could contain information about receiver
            // reports, etc., if particular test requirements arise.
        };

        std::map<uint32_t, member> members;
        std::map<uint32_t, sender> senders;

        void timer_expired();

        void sender_report_received(rtcp_header&, rtcp_packet&);
        void receiver_report_received(rtcp_header&, rtcp_packet&);
        void report_block_received(report_block&);
        void sdes_received(rtcp_header&, rtcp_packet&);
        void bye_received(rtcp_header&, rtcp_packet&);

    public:
        rtcp(rtp_service&, uint32_t ssrc, const char* cname);
        ~rtcp();

        void rtp_sent(rtp_packet&);
        void rtp_received(rtp_packet&);
        void rtcp_received(rtcp_packet&);
        void bye();

    private:
        void send_report();
        void send_bye();
        static void init_seq(sender* s, uint16_t seq);
        static int update_seq(sender* s, uint16_t seq);
        double rtcp_interval(int members, int senders, double rtcp_bw, int we_sent, double avg_rtcp_size, int initial);
        double drand48();
    };
}
