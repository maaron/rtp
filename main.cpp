
#include "stdafx.h"

#include "media.h"
#include "source.h"

#include <stdio.h>

int main()
{
    using namespace boost::asio;
    
    media::source src;
    int rtp_port = 60000;
    int rtcp_port = 60001;

    int rtp_port2 = 60002;
    int rtcp_port2 = 60003;

    src.open_stream(boost::asio::ip::address_v6::any(), "cname1", rtp_port, rtcp_port);
    src.open_stream(boost::asio::ip::address_v6::any(), "cname2", rtp_port2, rtcp_port2);

    src.start();
    
    /* - For debugging RTCP
    io_service io;
    media::rtcp tester(io, 0xfefefefe, "my_cname");
    int tester_port = 50001;
    tester.open(ip::address_v4::any(), tester_port);
    tester.set_remote(ip::udp::endpoint(ip::address::from_string("192.168.1.2"), 60001));
    tester.send_packet(media::get_ntp_time(), 0x1234);
    Sleep(100);
    tester.send_packet(media::get_ntp_time(), 0x431);
    */

    printf("Press any key to stop media\n");
    getc(stdin);

    src.stop();
}