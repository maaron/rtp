
#include "stdafx.h"

#include "stack.h"

int main()
{
    boost::asio::ip::udp::endpoint rtp, rtcp;
    rtp.port(60000);
    rtcp.port(60001);

    media::stack st(rtp, rtcp);

    st.run();
}