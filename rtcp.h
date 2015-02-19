#pragma once

#include "connection_pair.h"
#include <stdint.h>
#include <map>
#include <string>

namespace media
{
    class rtp_packet;
    class rtcp_packet;

    class rtcp
    {
        uint32_t ssrc;
        connection_pair& connection;
 
        bool init;
        std::string cname;
        uint32_t bytes_sent;
        uint32_t packets_sent;
            
        struct rtcp_peer_info
        {
            uint32_t bytes_received;
            uint32_t packets_received;
            int highest_sequence;
            int base_sequence;
            int sequence_cycles;
            int bad_sequence;
            int probation;
            int expected_prior;
            int received_prior;
            uint32_t jitter;
            uint32_t last_sr;
            ntp_time_t timestamp_last_sr;

            rtcp_peer_info();
        };
        std::map<uint32_t, rtcp_peer_info> peers;

        void timer_expired();

        void sender_report_received(rtcp_header&, rtcp_packet&);
        void receiver_report_received(rtcp_header&, rtcp_packet&);
        void sdes_received(rtcp_header&, rtcp_packet&);
        void bye_received(rtcp_header&, rtcp_packet&);

    public:
        rtcp(connection_pair&, uint32_t ssrc, const char* cname);
        ~rtcp();

        void rtp_sent(rtp_packet&);
        void rtp_received(rtp_packet&);
        void rtcp_received(rtcp_packet&);
        void bye();

    private:
        void send_report();
        void send_bye();
    };
}