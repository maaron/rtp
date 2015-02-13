#pragma once

#include <stdint.h>

namespace media
{
    // Gets the current NTP time in host order
    uint64_t get_ntp_time();

    uint32_t rand32();
}