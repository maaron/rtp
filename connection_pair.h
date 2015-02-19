#pragma once

#include "log.h"
#include "rtp_packet.h"
#include "rtcp_packet.h"
#include <algorithm>
#include <boost\thread.hpp>
#include <boost\asio.hpp>

namespace media
{
    class connection
    {
    public:
        boost::asio::ip::udp::socket local;
        boost::asio::ip::udp::endpoint remote;
        char buf[2048];

        connection(boost::asio::io_service&);

        void open(const boost::asio::ip::address&, int& port);
        bool try_open(const boost::asio::ip::address&, int& port);
        void set_peer(const boost::asio::ip::udp::endpoint& ep);
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
                    else if (ec != boost::asio::error::operation_aborted)
                    {
                        LOG("Socket receive error: %d, %s\n", ec.value(), ec.message().c_str());
                    }
                });
        }

        void close();
    };

    class connection_pair
    {
        boost::asio::io_service io;
        connection c1, c2;
        connection* rtp;
        connection* rtcp;
        boost::asio::deadline_timer rtcp_timer;
        boost::thread io_thread;

        ntp_time_t ntp_start_time;
        uint32_t rtp_start_time;

        void stop_request_received();

    public:
        connection_pair();
        ~connection_pair();

        // Opens a pair of UDP sockets on the specified ports.  If both are
        // zero, a pair of adjacent, ephemeral ports are opened.  If only one
        // is zero, any available ephemeral port will be used.  Otherwise,
        // the ports specified will attempt to be used.  If either port is
        // non-zero and already in use, or no adjacent ephemeral ports could be
        // allocated, an exception is thrown.
        void open(const boost::asio::ip::address& iface, int& rtp_port, int& rtcp_port);
        void start();
        void stop();

        uint32_t get_rtp_time(const ntp_time_t&);
        uint32_t get_rtp_start();
        
        void set_rtcp_peer(const boost::asio::ip::udp::endpoint&);
        void set_rtp_peer(const boost::asio::ip::udp::endpoint&);

        void send_rtp(rtp_packet&);
        void send_rtcp(rtcp_packet&);

        template <typename callback_t>
        void receive_rtp(callback_t cb)
        {
            rtp->async_receive([cb](void* data, size_t size)
            {
                rtp_packet pkt(data, size);
                cb(pkt);
            });
        }

        template <typename callback_t>
        void receive_rtcp(callback_t cb)
        {
            rtcp->async_receive([cb](void* data, size_t size)
            {
                rtcp_packet pkt(data, size);
                cb(pkt);
            });
        }

        template <typename duration_t, typename callback_t>
        void start_timer(const duration_t& t, callback_t cb)
        {
            rtcp_timer.expires_from_now(t);
            rtcp_timer.async_wait([cb](const boost::system::error_code& ec)
            {
                if (!ec) cb();
            });
        }

        template <typename callback_t>
        void post(callback_t cb)
        {
            io.post(cb);
        }


    };
}
