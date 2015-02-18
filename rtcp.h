#pragma once

#include <boost\asio.hpp>

namespace media
{
    using namespace boost::asio;

    class rtp_packet;
    class rtcp_packet;

    class rtcp
    {
        uint32_t ssrc;
        uint32_t bytes_sent;
        uint32_t packets_sent;
            
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
        rtcp(uint32_t ssrc, const char* cname);

        // Update octet and packet counts
        void rtp_sent(rtp_packet&);
        void rtp_received(rtp_packet&);
        void rtcp_received(rtcp_packet&);

        // Sends an RTCP packet
        void build_packet(uint64_t ntp_time, uint32_t rtp_time, rtcp_packet&);
        
        // Calculates the time interval before sending the next RTCP packet
        int get_send_time();

        void send_bye(const char* reason);
    };
}