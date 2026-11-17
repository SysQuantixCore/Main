//======================================================================
// gfx/GfxTrace.cpp
//======================================================================
#include "stdafx.h"
#include "gfx/GfxTrace.h"

#include <cstdarg>
#include <cstdio>

namespace gfx
{
    namespace
    {
        FILE* g_TraceFile = nullptr;
        bool  g_TraceEnabled = false;

        void EnsureOpen()
        {
            if (g_TraceFile == nullptr)
            {
                g_TraceFile = fopen("gfx_trace.log", "w");
            }
        }
    }

    void TraceEnable(bool enabled)
    {
        g_TraceEnabled = enabled;
    }

    bool TraceIsEnabled()
    {
        return g_TraceEnabled;
    }

    void Trace(const char* format, ...)
    {
        if (!g_TraceEnabled)
        {
            return;
        }

        EnsureOpen();
        if (g_TraceFile == nullptr)
        {
            return;
        }

        va_list args;
        va_start(args, format);
        vfprintf(g_TraceFile, format, args);
        va_end(args);

        fputc('\n', g_TraceFile);
        fflush(g_TraceFile);
    }
}
