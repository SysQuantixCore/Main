//======================================================================
// gfx/GfxTrace.h
//
// Trace simples em arquivo texto, usado para diagnosticar o backend
// moderno. Escrito em "gfx_trace.log" na pasta do executavel.
//
// Ativado pela chave "Verbose=1" em Graphics.ini ou pela flag
// --gfx-verbose. Sem isso, Trace() nao faz nada (custo zero).
//======================================================================
#pragma once

namespace gfx
{
    void Trace(const char* format, ...);
    void TraceEnable(bool enabled);
    bool TraceIsEnabled();
}
