#pragma once

#include <boost\asio.hpp>

namespace media
{
    using namespace boost::asio;
    class rtp_packet;
    class rtcp_packet;

    class rtp
    {
        ip::udp::socket local;
        ip::udp::endpoint remote;
        char buf[2048];

    public:
        rtp(io_service& io);

        void open(const ip::address& iface, int& port);
        bool try_open(const ip::address& iface, int port);

        void set_remote(const ip::udp::endpoint& ep);

        void stop();

        template <typename callback_t>
        void async_receive(callback_t c)
        {
            void* data = buf;
            local.async_receive_from(buffer(buf), remote,
                [c, data](const boost::system::error_code& ec, size_t bytes_transferred)
                {
                    rtp_packet pkt(data, bytes_transferred);
                    c(pkt);
                });
        }

        void send(rtp_packet&);
    };
}