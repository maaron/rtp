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
        uint32_t bytes_sent;
        uint32_t packets_sent;
        std::string bye_reason;
            
        struct rtcp_peer_info
        {
            uint32_t bytes_received;
            uint32_t packets_received;
            uint32_t remote_ssrc;
            uint8_t fraction_lost;
            uint32_t packets_lost;
            uint32_t highest_sequence;
            uint32_t jitter;
            uint32_t last_sr;
            uint32_t delay_last_sr;

            rtcp_peer_info();
        };
        std::map<uint32_t, rtcp_peer_info> peers;

        // SDES items
        std::string cname;

    public:
        rtcp(connection_pair&, uint32_t ssrc, const char* cname);

        void rtp_sent(rtp_packet&);
        void rtp_received(rtp_packet&);
        void rtcp_received(rtcp_packet&);
        void bye();

    private:
        void send_report();
        void send_bye();
    };
}