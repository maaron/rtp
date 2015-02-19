#pragma once

#include <stdint.h>

namespace media
{
    struct ntp_time_t
    {
        uint32_t seconds;
        uint32_t fractional;
    };

    // Gets the current NTP time in host order
    ntp_time_t get_ntp_time();

    // Generates a random 32-bit integer.  This function is not thread-safe.
    uint32_t rand32();
}