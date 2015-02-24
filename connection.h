#pragma once

#include <boost\asio.hpp>

namespace media
{
    // This class encapsulates the details associated with a local/remote UDP pair.
    class connection
    {
    public:
        boost::asio::io_service::strand& io_strand;
        boost::asio::ip::udp::socket local;
        boost::asio::ip::udp::endpoint remote;
        char buf[2048];

        connection(boost::asio::io_service::strand&);

        void open(const boost::asio::ip::address&, int& port);
        bool try_open(const boost::asio::ip::address&, int port);
        void set_peer(const boost::asio::ip::udp::endpoint& ep);
        void send(void* data, size_t size);

        void async_receive(std::function<void(void*, size_t)> cb);

        void close();

        std::string to_string();
    };
}