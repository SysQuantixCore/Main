//======================================================================
// gfx/GfxContext.h
//
// Criacao do contexto OpenGL.
//
// Dois backends:
//
//  LEGADO (padrao, comportamento atual do jogo)
//      Contexto OpenGL 1.1 de compatibilidade, criado com wglCreateContext
//      sem informar versao. Pipeline fixo. Zero mudanca visual.
//
//  MODERNO (opt-in via Graphics.ini / --renderer=modern)
//      Contexto OpenGL 3.3 core profile (ou superior) criado com
//      wglCreateContextAttribsARB. Pipeline VBO/VAO + GLSL.
//
// FALLBACK
//      Se o driver nao suportar a versao solicitada, se nao houver
//      WGL_ARB_create_context ou se a criacao do contexto core falhar,
//      o modulo volta para o backend legado e registra o motivo. O jogo
//      continua funcionando exatamente como antes.
//
// REQUISITO IMPORTANTE DO CORE PROFILE
//      A partir do OpenGL 3.2 o core profile exige framebuffer com
//      stencil buffer de pelo menos 8 bits. Por isso o pixel format do
//      backend moderno pede cStencilBits = 8 e cAlphaBits = 8, enquanto o
//      backend legado mantem o pixel format original.
//======================================================================
#pragma once

#include <string>

#include <windows.h>

namespace gfx
{
    struct ContextInfo
    {
        bool        Modern = false;
        int         Major = 0;
        int         Minor = 0;
        std::string Vendor;
        std::string Renderer;
        std::string Version;

        // Preenchido quando o backend moderno foi tentado e recusado.
        std::string FallbackReason;
    };

    class GfxContext
    {
    public:
        // Cria o contexto conforme a configuracao carregada.
        // Retorna false apenas se nem o legado nem o moderno puderam ser
        // criados (neste caso o chamador exibe a mensagem de erro).
        static bool Create(HDC dc, HGLRC& outRenderContext, ContextInfo& outInfo, std::string& outError);

        static void Destroy(HGLRC renderContext);

        // Liga/desliga o VSync (WGL_EXT_swap_control).
        static bool SetVSync(HDC dc, bool enabled);

        // Habilita/desabilita MSAA via WGL_ARB_multisample.
        // Retorna false se a extensao nao estiver disponivel.
        static bool SetMultisample(HDC dc, bool enabled);
    };
}
