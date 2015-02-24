
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
        port = local.local_endpoint().port();
    }

    bool connection::try_open(const boost::asio::ip::address& iface, int port)
    {
        system::error_code ec;
        ip::udp::endpoint ep(iface, port);
        local.open(ep.protocol());
        local.bind(ep, ec);

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
                    // TODO:  If a socket error occurs, the callback won't be
                    // called and the receive won't be restarted.  This is good
                    // for situations in which the socket is dead for some
                    // reason, so that we don't start spewing error messages
                    // uncontrollably.  However, there may be error conditions
                    // that don't warrant giving up.  These should be identified
                    // here and handled appropriately.
                    dbg_msg("Socket receive error: %d, %s", ec.value(), ec.message().c_str());
                }
            }));
    }

    void connection::close()
    {
        if (local.is_open()) local.close();
    }

    std::string connection::to_string()
    {
        std::ostringstream str;
        
        if (local.is_open())
        {
            if (local.local_endpoint().address().is_v6())
            {
                str << '['
                    << local.local_endpoint().address().to_string()
                    << "]:" << local.local_endpoint().port();
            }
            else
            {
                str << local.local_endpoint().address().to_string() 
                    << ':' << local.local_endpoint().port();
            }
        }
        else str << "(closed)";

        return str.str();
    }
}
