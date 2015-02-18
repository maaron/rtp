#pragma once

#include "log.h"
#include <boost\asio.hpp>

namespace media
{
    class connection
    {
        boost::asio::ip::udp::socket local;
        boost::asio::ip::udp::endpoint remote;
        char buf[2048];

    public:
        connection(const boost::asio::ip::udp::endpoint& local);

        void set_remote(const boost::asio::ip::udp::endpoint& ep);
        void send(void* data, size_t size);

        template <typename callback_t>
        void async_receive(callback_t c)
        {
            void* data = buf;
            local.async_receive_from(buffer(buf), remote, 
                [c, data](const boost::system::error_code& ec, size_t bytes_transferred)
                {
                    if (!ec)
                    {
                        c(data, bytes_transferred);
                    }
                    else
                    {
                        LOG("Socket receive error: %d, %s", ec.value(), ec.message().c_str());
                    }
                });
        }

        void close();
    };

    class connection_pair
    {
        connection pair[2];

    public:
        connection_pair();

        // Opens a pair of UDP sockets on the specified ports.  If both are
        // zero, a pair of adjacent, ephemeral ports are opened.  If only one
        // is zero, it any available ephemeral port will be used.  Otherwise,
        // the ports specified will attempt to be used.  If either port is
        // non-zero and already in use, or no adjacent ephemeral ports could be
        // allocated, and exception is thrown.
        void open(const boost::asio::ip::address& iface, int& port1, int& port2);

        void send_rtp(rtp_packet&);
        void send_rtcp(rtcp_packet&);

        template <typename callback_t>
        void receive_rtp(callback_t cb)
        {
        }

        template <typename callback_t>
        void receive_rtcp(callback_t cb)
        {
        }
    };
}
