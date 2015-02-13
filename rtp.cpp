
#include "stdafx.h"

#include "rtp.h"
#include "rtp_packet.h"

namespace media
{
    rtp::rtp(io_service& io)
        : local(io)
    {
    }

    void rtp::open(const ip::address& iface, int& port)
    {
        boost::system::error_code ec;
        ip::udp::endpoint ep(iface, port);
        local.open(ep.protocol());

        if (port == 0)
        {
            for (int i = 0; i < 10; i++)
            {
                local.bind(ip::udp::endpoint(iface, port), ec);
                port = local.local_endpoint().port();

                if (port & 1) port++;
                else return;
            }

            throw std::exception("Failed to allocated RTP port");
        }
        else local.bind(ip::udp::endpoint(iface, port));
    }

    bool rtp::try_open(const ip::address& iface, int port)
    {
        boost::system::error_code ec;
        ip::udp::endpoint ep(iface, port);
        local.open(ep.protocol());
        local.bind(ep, ec);
        return !ec;
    }

    void rtp::set_remote(const ip::udp::endpoint& ep)
    {
        remote = ep;
    }

    void rtp::stop()
    {
        local.close();
    }

    void rtp::send(rtp_packet& pkt)
    {
        local.send_to(buffer(pkt.data(), pkt.size()), remote);
    }
}