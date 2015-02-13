
#include "stdafx.h"

#include "rtp.h"
#include "rtp_packet.h"

namespace media
{
    rtp::rtp(io_service& io, ip::udp::endpoint local_endpoint)
        : local(io, local_endpoint)
    {
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