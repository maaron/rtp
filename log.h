#pragma once

#define LOG(format, ...) log_msg("[%s] " format ", %s:%d\n", this->to_string().c_str(), __VA_ARGS__, __FILE__, __LINE__)

namespace media
{
    void log_msg(const char* format, ...);
}