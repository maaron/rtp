
#include "media.h"
#include "rtp_service.h"
#include <boost\bind.hpp>

using namespace boost;
using namespace boost::asio;

namespace media
{
    rtp_service::rtp_service(io_service& io)
        : io_strand(io), c1(io_strand), c2(io_strand), rtp(&c1), rtcp(&c2), rtcp_timer(io), started(false)
    {
    }

    rtp_service::~rtp_service()
    {
        stop();
    }

    void rtp_service::open(const boost::asio::ip::address& iface, int& rtp_port, int& rtcp_port)
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
                    return;
                }
            }
            throw std::exception("Failed to allocate RTP/RTCP port pair");
        }
        else
        {
            c1.open(iface, rtp_port);
            c2.open(iface, rtcp_port);
        }
    }

    void rtp_service::start()
    {
        if (!started)
        {
            dbg("Starting IO for stream");

            ntp_start_time = get_ntp_time();
            rtp_start_time = get_rtp_time(ntp_start_time);
            started = true;
        }
    }

    void rtp_service::stop()
    {
        if (started)
        {
            dbg("Stopping IO");

            io_strand.post(boost::bind(&rtp_service::stop_request_received, this));
            started = false;
        }
    }

    void rtp_service::stop_request_received()
    {
        rtcp_timer.cancel();
        c1.close();
        c2.close();
    }

    uint32_t rtp_service::get_rtp_time(const ntp_time_t& ntp_time)
    {
        return rtp_start_time + 
            (ntp_time.fractional - ntp_start_time.fractional);
    }

    uint32_t rtp_service::get_rtp_start() { return rtp_start_time; }
        
    void rtp_service::set_rtcp_peer(const boost::asio::ip::udp::endpoint& ep) { rtcp->set_peer(ep); }

    void rtp_service::set_rtp_peer(const boost::asio::ip::udp::endpoint& ep) { rtp->set_peer(ep); }

    void rtp_service::send_rtp(rtp_packet& pkt)
    {
        rtp->send(pkt.data(), pkt.size());
    }

    void rtp_service::send_rtcp(rtcp_packet& pkt)
    {
        rtcp->send(pkt.data(), pkt.compound_size());
    }

    void rtp_service::receive_rtp(std::function<void(rtp_packet&)> cb)
    {
        rtp->async_receive([cb](void* data, size_t size)
        {
            rtp_packet pkt(data, size);
            cb(pkt);
        });
    }

    void rtp_service::receive_rtcp(std::function<void(rtcp_packet&)> cb)
    {
        rtcp->async_receive([cb](void* data, size_t size)
        {
            rtcp_packet pkt(data, size);
            cb(pkt);
        });
    }

    void rtp_service::start_timer(int ms, std::function<void()> cb)
    {
        rtcp_timer.expires_from_now(boost::posix_time::milliseconds(ms));
        rtcp_timer.async_wait(io_strand.wrap([cb](const boost::system::error_code& ec)
        {
            if (!ec) cb();
        }));
    }

    void rtp_service::post(std::function<void()> cb)
    {
        io_strand.post(cb);
    }

    std::string rtp_service::to_string()
    {
        return "RTP " + rtp->to_string() + ", RTCP " + rtcp->to_string();
    }
}