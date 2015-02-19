
#include "media.h"
#include "connection_pair.h"

using namespace boost;
using namespace boost::asio;

namespace media
{
    connection::connection(io_service& io)
        : local(io)
    {
    }

    void connection::open(const boost::asio::ip::address& iface, int& port)
    {
        ip::udp::endpoint ep(iface, port);
        local.open(ep.protocol());
        local.bind(ep);

        LOG("Opened socket bound to [%s]:%d\n", iface.to_string().c_str(), port);
    }

    bool connection::try_open(const boost::asio::ip::address& iface, int& port)
    {
        system::error_code ec;
        ip::udp::endpoint ep(iface, port);
        local.open(ep.protocol());
        local.bind(ep, ec);

        if (!ec) LOG("Opened socket bound to [%s]:%d\n", iface.to_string().c_str(), port);
        
        return !ec;
    }

    void connection::set_peer(const boost::asio::ip::udp::endpoint& ep) { remote = ep; }

    void connection::send(void* data, size_t size)
    {
        local.send_to(buffer(data, size), remote);
    }

    void connection::close()
    {
        if (local.is_open())
        {
            LOG("Closing socket bound to [%s]:%d\n", 
                local.local_endpoint().address().to_string().c_str(), 
                local.local_endpoint().port());

            local.close();
        }
    }

    connection_pair::connection_pair()
        : c1(io), c2(io), rtp(&c1), rtcp(&c2), rtcp_timer(io)
    {
    }

    connection_pair::~connection_pair()
    {
        stop();
    }

    void connection_pair::open(const boost::asio::ip::address& iface, int& rtp_port, int& rtcp_port)
    {
        if (rtp_port == 0 && rtcp_port == 0)
        {
            int port1 = 0;
            int port2 = 0;

            for (int i = 0; i < 10; i++)
            {
                c1.open(iface, port1);
                port2 = port1 + 1;
                if (c2.try_open(iface, port2))
                {
                    if (port1 % 2 == 0)
                    {
                        rtp_port = port1;
                        rtcp_port = port2;
                    }
                    else
                    {
                        rtp_port = port2;
                        rtcp_port = port1;
                    }
                }
                throw std::exception("Failed to allocate RTP/RTCP port pair");
            }
        }
        else
        {
            c1.open(iface, rtp_port);
            c2.open(iface, rtcp_port);
        }
    }

    void connection_pair::start()
    {
        LOG("Starting IO for stream [%s](%d,%d)\n",
            c1.local.local_endpoint().address().to_string().c_str(),
            c1.local.local_endpoint().port(),
            c2.local.local_endpoint().port());

        ntp_start_time = get_ntp_time();
        rtp_start_time = get_rtp_time(ntp_start_time);

        io_thread = thread(bind(&io_service::run, &io));
    }

    void connection_pair::stop()
    {
        if (io_thread.joinable())
        {
            LOG("Stopping IO for stream [%s](%d,%d)\n",
                c1.local.local_endpoint().address().to_string().c_str(),
                c1.local.local_endpoint().port(),
                c2.local.local_endpoint().port());

            post(bind(&connection_pair::stop_request_received, this));
            io_thread.join();

            LOG("Finished\n");
        }
    }

    void connection_pair::stop_request_received()
    {
        rtcp_timer.cancel();
        c1.close();
        c2.close();
    }

    uint32_t connection_pair::get_rtp_time(const ntp_time_t& ntp_time)
    {
        return rtp_start_time + 
            (ntp_time.fractional - ntp_start_time.fractional);
    }

    uint32_t connection_pair::get_rtp_start() { return rtp_start_time; }
        
    void connection_pair::set_rtcp_peer(const boost::asio::ip::udp::endpoint& ep) { rtcp->set_peer(ep); }

    void connection_pair::set_rtp_peer(const boost::asio::ip::udp::endpoint& ep) { rtp->set_peer(ep); }

    void connection_pair::send_rtp(rtp_packet& pkt)
    {
        rtp->send(pkt.data(), pkt.size());
    }

    void connection_pair::send_rtcp(rtcp_packet& pkt)
    {
        rtcp->send(pkt.data(), pkt.compound_size());
    }
}