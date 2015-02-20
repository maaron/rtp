
#include "stdafx.h"

#include "media.h"
#include "source.h"

#include <stdio.h>

int main()
{
    using namespace boost::asio;

#if 0
    // For debugging RTCP receives
    media::source src1;
    int src1_rtp = 40000;
    int src1_rtcp = 40001;
    int src2_rtp = 40002;
    int src2_rtcp = 40003;
    src1.open_stream(ip::address_v4::any(), "source1", src1_rtp, src1_rtcp);
    src1.start();

    io_service io;
    ip::udp::socket remote(io, ip::udp::endpoint(ip::address_v4::any(), 40003));
    unsigned char data[] = {
        0x81, 0xc8, 0x00, 0x0c, 0x00, 0x00, 0x4a, 0x00, 
        0xd8, 0x90, 0xd3, 0xbe, 0x28, 0x72, 0xaf, 0xf2, 
        0x00, 0x00, 0x7f, 0x7e, 0x00, 0x00, 0x00, 0x5e, 
        0x00, 0x00, 0x0b, 0xea, 0x87, 0xd5, 0xe2, 0xd2,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa6, 
        0x00, 0x00, 0x00, 0x1a, 0x00, 0x00, 0x00, 0x00, 
        0x00, 0x00, 0x00, 0x00
    };
    remote.send_to(
        buffer(data),
        ip::udp::endpoint(ip::address::from_string("127.0.0.1"), 40001));
#endif

#if 0
    //For debugging RTCP sends
    media::rtp_service pair;
    media::rtcp tester(pair, 0xfefefefe, "my_cname");
    int port1 = 50000;
    int port2 = 50001;
    pair.open(ip::address_v4::any(), port1, port2);
    pair.set_rtcp_peer(ip::udp::endpoint(ip::address::from_string("192.168.1.2"), 60001));
    pair.start();

    printf("Press any key to stop media\n");
    getc(stdin);

    pair.stop();
    printf("Media stopped\n");
#endif

    media::source src;
    int rtp_port = 60000;
    int rtcp_port = 60001;

    int rtp_port2 = 60002;
    int rtcp_port2 = 60003;

    src.open_stream(boost::asio::ip::address::from_string("2001:0:0:1::3"), "cname1", rtp_port, rtcp_port);
    src.open_stream(boost::asio::ip::address::from_string("2001:0:0:1::3"), "cname2", rtp_port2, rtcp_port2);

    src.start();

    printf("Press any key to stop media\n");
    getc(stdin);

    src.stop();
}
