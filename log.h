#pragma once

#define LOG(format, ...) log_msg("[%s] " format ", %s:%s:%d\n", this->to_string().c_str(), __VA_ARGS__, __FILE__, __FUNCTION__, __LINE__)

namespace media
{
    void log_msg(const char* format, ...);
}