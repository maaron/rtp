
#include "connection.h"
#include "log.h"
#include <sstream>

namespace media
{
    using namespace boost;
    using namespace boost::asio;

    connection::connection(io_service::strand& strand)
        : local(strand.get_io_service()), io_strand(strand)
    {
    }

    void connection::open(const boost::asio::ip::address& iface, int& port)
    {
        ip::udp::endpoint ep(iface, port);
        local.open(ep.protocol());
        local.bind(ep);

        LOG("Opened socket bound to [%s]:%d", iface.to_string().c_str(), port);
    }

    bool connection::try_open(const boost::asio::ip::address& iface, int& port)
    {
        system::error_code ec;
        ip::udp::endpoint ep(iface, port);
        local.open(ep.protocol());
        local.bind(ep, ec);

        if (!ec) LOG("Opened socket bound to [%s]:%d", iface.to_string().c_str(), port);
        
        return !ec;
    }

    void connection::set_peer(const boost::asio::ip::udp::endpoint& ep) { remote = ep; }

    void connection::send(void* data, size_t size)
    {
        local.send_to(buffer(data, size), remote);
    }

    void connection::async_receive(std::function<void(void*, size_t)> cb)
    {
        void* data = buf;
        local.async_receive_from(buffer(buf), remote, io_strand.wrap(
            [cb, data, this](const boost::system::error_code& ec, size_t bytes_transferred)
            {
                if (!ec)
                {
                    cb(data, bytes_transferred);
                }
                else if (ec != boost::asio::error::operation_aborted)
                {
                    LOG("Socket receive error: %d, %s", ec.value(), ec.message().c_str());
                }
            }));
    }

    void connection::close()
    {
        if (local.is_open())
        {
            LOG("Closing socket bound to [%s]:%d", 
                local.local_endpoint().address().to_string().c_str(), 
                local.local_endpoint().port());

            local.close();
        }
    }

    std::string connection::to_string()
    {
        std::ostringstream str;
        
        if (local.is_open())
        {
            str << local.local_endpoint().address().to_string() 
                << ": " 
                << local.local_endpoint().port();
        }
        else str << "(closed)";

        str << " --> ";

        str << remote.address().to_string()
            << ": "
            << remote.port();

        return str.str();
    }
}