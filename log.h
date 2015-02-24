#pragma once

#define dbg(format, ...) dbg_msg("[%s] " format ", %s:%s:%d", this->to_string().c_str(), __VA_ARGS__, __FILE__, __FUNCTION__, __LINE__)

namespace media
{
    void dbg_msg(const char* format, ...);
}