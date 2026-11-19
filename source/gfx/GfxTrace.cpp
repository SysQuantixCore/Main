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

    // -----------------------------------------------------------------
    // Profiling de performance (so ativo com verbose).
    // -----------------------------------------------------------------
    struct PerfAcc
    {
        double matrixMs = 0.0;
        double stateMs = 0.0;
        unsigned long matrixCalls = 0;
        unsigned long stateCalls = 0;
    };
    static PerfAcc g_PerfAcc;

    double PerfNowMs()
    {
        LARGE_INTEGER t;
        QueryPerformanceCounter(&t);
        static double s_Freq = 0.0;
        if (s_Freq == 0.0)
        {
            LARGE_INTEGER f;
            QueryPerformanceFrequency(&f);
            s_Freq = static_cast<double>(f.QuadPart);
        }
        return static_cast<double>(t.QuadPart) / s_Freq * 1000.0;
    }

    void PerfAddMatrix(double ms)
    {
        if (!g_TraceEnabled) return;
        g_PerfAcc.matrixMs += ms;
        ++g_PerfAcc.matrixCalls;
    }

    void PerfAddState(double ms)
    {
        if (!g_TraceEnabled) return;
        g_PerfAcc.stateMs += ms;
        ++g_PerfAcc.stateCalls;
    }

    void PerfReportAndReset()
    {
        if (g_PerfAcc.matrixCalls == 0 && g_PerfAcc.stateCalls == 0) return;
        Trace("[gfx] PERF/state: matrix=%.1fms (%lu calls, %.2fus/call) state=%.1fms (%lu calls, %.2fus/call)",
              g_PerfAcc.matrixMs, g_PerfAcc.matrixCalls,
              g_PerfAcc.matrixCalls ? g_PerfAcc.matrixMs * 1000.0 / g_PerfAcc.matrixCalls : 0.0,
              g_PerfAcc.stateMs, g_PerfAcc.stateCalls,
              g_PerfAcc.stateCalls ? g_PerfAcc.stateMs * 1000.0 / g_PerfAcc.stateCalls : 0.0);
        g_PerfAcc = PerfAcc();
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
