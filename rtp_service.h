#pragma once

#include "log.h"
#include "rtp_packet.h"
#include "rtcp_packet.h"
#include "connection.h"
#include <algorithm>
#include <boost\asio.hpp>

namespace media
{
    // This class provides all the socket IO and timers needed for an RTP/RTCP
    // endpoint.  It is bound to a boost::asio::io_service object that will be
    // used for IO completion callbacks.  All callbacks will be run through a
    // single boost::asio::io_service::strand to ensure that operations within
    // an instance of this class are synchronous (regardless of the number of
    // threads bound to the io_service object).
    class rtp_service
    {
        boost::asio::io_service::strand io_strand;
        connection c1, c2;
        connection* rtp;
        connection* rtcp;
        boost::asio::deadline_timer rtcp_timer;
        bool started;

        ntp_time_t ntp_start_time;
        uint32_t rtp_start_time;

        void stop_request_received();

    public:
        rtp_service(boost::asio::io_service&);
        ~rtp_service();

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

        void receive_rtp(std::function<void(rtp_packet&)> cb);

        void receive_rtcp(std::function<void(rtcp_packet&)> cb);

        void start_timer(int ms, std::function<void()> cb);

        void post(std::function<void()> cb);

        std::string to_string();
    };
}
