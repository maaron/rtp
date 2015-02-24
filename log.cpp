
#include <stdarg.h>
#include <stdio.h>
#include <Windows.h>
#include "log.h"

namespace media
{
    void dbg_msg(const char* format, ...)
    {
        char buffer[512];

        va_list args;
        va_start(args, format);

        vsnprintf_s(buffer, _TRUNCATE, format, args);

        va_end(args);

        SYSTEMTIME st;
        ::GetLocalTime(&st);

        printf("[%02d:%02d:%02d.%03d] [%08x] %s\n", 
            st.wHour, st.wMinute, st.wSecond, st.wMilliseconds, 
            ::GetCurrentThreadId(), buffer);
    }
}