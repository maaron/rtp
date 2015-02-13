#pragma once

#define LOG(format, ...) log_msg(format, __VA_ARGS__)

namespace media
{
    void log_msg(const char* format, ...);
}