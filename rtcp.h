#pragma once

#include <boost\asio.hpp>

namespace media
{
    using namespace boost::asio;

    class rtp_packet;
    class rtcp_packet;

    class rtcp
    {
        io_service& io;
        ip::udp::socket local;
        ip::udp::endpoint remote;
        char buf[2048];
        uint32_t ssrc;

        uint32_t bytes_sent;
        uint32_t packets_sent;
        uint32_t bytes_received;
        uint32_t packets_received;
        uint32_t remote_ssrc;
        uint8_t fraction_lost;
        uint32_t packets_lost;
        uint32_t highest_sequence;
        uint32_t jitter;
        uint32_t last_sr;
        uint32_t delay_last_sr;

    public:
        rtcp(io_service& io, ip::udp::endpoint local_endpoint);

        // Starts an asynchronous receive operation.  Upon completion, the 
        // supplied callback will be called.
        template <typename callback_t>
        void async_receive(callback_t c)
        {
            void* data = buf;
            local.async_receive_from(buffer(buf), remote,
                [c, data](const boost::system::error_code& ec, size_t bytes_transferred)
                {
                    rtcp_packet pkt(data, bytes_transferred);
                    c(pkt);
                });
        }

        // Closes RTCP sockets
        void stop();

        // Update octet and packet counts
        void rtp_sent(rtp_packet&);
        void rtp_received(rtp_packet&);

        // Sends an RTCP packet
        void send_packet(uint64_t ntp_time, uint32_t rtp_time);
        
        // Calculates the time interval before sending the next RTCP packet
        int get_send_time();
    };
}