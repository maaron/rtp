#pragma once

#include <boost\asio.hpp>

namespace media
{
    using namespace boost::asio;
    class rtp_packet;
    class rtcp_packet;

    class rtp
    {
    public:
        rtp();

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
    };
}