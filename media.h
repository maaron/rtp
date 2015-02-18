#pragma once

#include <stdint.h>

namespace media
{
    // Gets the current NTP time in host order
    uint64_t get_ntp_time();

    // Generates a random 32-bit integer.  This function is not thread-safe.
    uint32_t rand32();
}