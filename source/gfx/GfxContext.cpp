//======================================================================
// gfx/GfxContext.cpp
//======================================================================
#include "stdafx.h"
#include "gfx/GfxContext.h"
#include "gfx/GfxConfig.h"
#include "gfx/GfxRenderer.h"
#include "gfx/GfxRawGL.h"
#include "gfx/GfxTrace.h"

#include <vector>

namespace gfx
{
    namespace
    {
        // Tipos WGL usados apenas aqui.
        typedef HGLRC(WINAPI* PFNWGLCREATECONTEXTATTRIBSARBPROC)(HDC, HGLRC, const int*);
        typedef BOOL(WINAPI* PFNWGLSWAPINTERVALEXTPROC)(int);
        typedef BOOL(WINAPI* PFNWGLSETSWAPINTERVALEXTPROC)(int);

        // Tokens de WGL_ARB_create_context. Definidos aqui para nao depender
        // de <wglext.h> (que conflita com o wglew.h do GLEW).
        const int WGL_CONTEXT_MAJOR_VERSION_ARB = 0x2091;
        const int WGL_CONTEXT_MINOR_VERSION_ARB = 0x2092;
        const int WGL_CONTEXT_PROFILE_MASK_ARB  = 0x9126;
        const int WGL_CONTEXT_CORE_PROFILE_BIT_ARB = 0x00000001;
        const int WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB = 0x00000002;

        PFNWGLCREATECONTEXTATTRIBSARBPROC g_wglCreateContextAttribsARB = nullptr;
        PFNWGLSWAPINTERVALEXTPROC         g_wglSwapIntervalEXT = nullptr;
        PFNWGLSETSWAPINTERVALEXTPROC       g_wglSetSwapIntervalEXT = nullptr;

        bool g_WglExtensionsQueried = false;
        std::string g_WglExtensions;

        void QueryWglExtensions()
        {
            if (g_WglExtensionsQueried)
            {
                return;
            }
            g_WglExtensionsQueried = true;

            // WGL_ARB_extensions_string precisa de um contexto current.
            auto getExtensionsStringARB = reinterpret_cast<const char* (WINAPI*)(HDC)>(
                wglGetProcAddress("wglGetExtensionsStringARB"));
            if (getExtensionsStringARB != nullptr)
            {
                const char* extensions = getExtensionsStringARB(wglGetCurrentDC());
                if (extensions != nullptr)
                {
                    g_WglExtensions = extensions;
                }
            }

            g_wglCreateContextAttribsARB = reinterpret_cast<PFNWGLCREATECONTEXTATTRIBSARBPROC>(
                wglGetProcAddress("wglCreateContextAttribsARB"));

            g_wglSwapIntervalEXT = reinterpret_cast<PFNWGLSWAPINTERVALEXTPROC>(
                wglGetProcAddress("wglSwapIntervalEXT"));
            g_wglSetSwapIntervalEXT = reinterpret_cast<PFNWGLSETSWAPINTERVALEXTPROC>(
                wglGetProcAddress("wglSetSwapIntervalEXT"));
        }

        bool HasWglExtension(const char* name)
        {
            if (g_WglExtensions.empty() || name == nullptr)
            {
                return false;
            }
            return g_WglExtensions.find(name) != std::string::npos;
        }

        // Cria um contexto de compatibilidade (o caminho legado original).
        HGLRC CreateLegacyContext(HDC dc, std::string& outError)
        {
            HGLRC rc = wglCreateContext(dc);
            if (rc == nullptr)
            {
                outError = "Falha ao criar o contexto OpenGL de compatibilidade (wglCreateContext).";
            }
            return rc;
        }

        // Tenta criar um contexto na versao pedida.
        HGLRC CreateModernContext(HDC dc, int major, int minor, bool useCoreProfile, std::string& outError)
        {
            if (g_wglCreateContextAttribsARB == nullptr)
            {
                outError = "O driver nao expoe WGL_ARB_create_context.";
                return nullptr;
            }

        // Tenta criar um contexto na versao pedida.
        //
        // Perfil: por padrao usamos COMPATIBILITY profile (3.3 compat).
        //
        // Motivo: a ScaleForm.dll (proprietaria, presente no cliente) e o
        // componente de login/UI e usa OpenGL fixed-function
        // (glMatrixMode/glOrtho/glBegin). Em core profile ela falha e o
        // cliente nao consegue completar o login automatico.
        //
        // O perfil de compatibilidade NAO prejudica o renderer novo: VBOs,
        // VAOs, shaders GLSL 3.3, FBO e instancing funcionam igual. O que
        // fica disponivel e o fixed-function, que o renderer novo nao usa.
        //
        // Quem quiser testar o core profile puro pode marcar
        // "CoreProfile=1" em Graphics.ini (a ScaleForm e desativada nesse
        // modo e o login automatico deixa de funcionar).
        const int attribs[] =
        {
            WGL_CONTEXT_MAJOR_VERSION_ARB, major,
            WGL_CONTEXT_MINOR_VERSION_ARB, minor,
            WGL_CONTEXT_PROFILE_MASK_ARB,  useCoreProfile
                ? WGL_CONTEXT_CORE_PROFILE_BIT_ARB
                : WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB,
            0
        };

            const HGLRC rc = g_wglCreateContextAttribsARB(dc, nullptr, attribs);
            if (rc == nullptr)
            {
                char buffer[160];
                sprintf_s(buffer, sizeof(buffer),
                         "O driver nao aceitou um contexto OpenGL %d.%d core profile.", major, minor);
                outError = buffer;
            }
            return rc;
        }

        void FillContextInfo(ContextInfo& info)
        {
            const GLubyte* vendor = glGetString(GL_VENDOR);
            const GLubyte* renderer = glGetString(GL_RENDERER);
            const GLubyte* version = glGetString(GL_VERSION);

            info.Vendor   = vendor   ? reinterpret_cast<const char*>(vendor)   : "Unknown";
            info.Renderer = renderer ? reinterpret_cast<const char*>(renderer) : "Unknown";
            info.Version  = version  ? reinterpret_cast<const char*>(version)  : "Unknown";

            // GL_MAJOR_VERSION/GL_MINOR_VERSION so existem em OpenGL 3.0+.
            // No backend legado (contexto 1.1) a consulta geraria
            // GL_INVALID_ENUM, entao consultamos apenas quando disponivel.
            if (GLEW_VERSION_3_0)
            {
                GLint major = 0, minor = 0;
                glGetIntegerv(GL_MAJOR_VERSION, &major);
                glGetIntegerv(GL_MINOR_VERSION, &minor);
                info.Major = major;
                info.Minor = minor;
            }
        }
    }

    bool GfxContext::Create(HDC dc, HGLRC& outRenderContext, ContextInfo& outInfo, std::string& outError)
    {
        outRenderContext = nullptr;
        outInfo = ContextInfo();

        const Config& config = Config::Instance();
        const bool wantModern = config.IsModern();

        PIXELFORMATDESCRIPTOR pfd;
        memset(&pfd, 0, sizeof(pfd));
        pfd.nSize = sizeof(pfd);
        pfd.nVersion = 1;
        pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
        pfd.iPixelType = PFD_TYPE_RGBA;
        pfd.cColorBits = 32;
        pfd.cDepthBits = 24;

        if (wantModern)
        {
            // Core profile 3.2+ exige stencil buffer de 8 bits e um
            // framebuffer RGBA completo.
            pfd.cAlphaBits = 8;
            pfd.cStencilBits = 8;
        }

        const GLuint pixelFormat = ChoosePixelFormat(dc, &pfd);
        if (pixelFormat == 0)
        {
            outError = "OpenGL Choose Pixel Format Error.";
            return false;
        }

        PIXELFORMATDESCRIPTOR chosen;
        memset(&chosen, 0, sizeof(chosen));
        if (!DescribePixelFormat(dc, pixelFormat, sizeof(PIXELFORMATDESCRIPTOR), &chosen))
        {
            outError = "OpenGL Describe Pixel Format Error.";
            return false;
        }

        if (!SetPixelFormat(dc, pixelFormat, &chosen))
        {
            outError = "OpenGL Set Pixel Format Error.";
            return false;
        }

        // -----------------------------------------------------------------
        // Caminho legado (comportamento original, preservado)
        // -----------------------------------------------------------------
        if (!wantModern)
        {
            HGLRC rc = CreateLegacyContext(dc, outError);
            if (rc == nullptr)
            {
                return false;
            }
            if (!wglMakeCurrent(dc, rc))
            {
                outError = "OpenGL Make Current Error.";
                wglDeleteContext(rc);
                return false;
            }
            outRenderContext = rc;
            outInfo.Modern = false;
            FillContextInfo(outInfo);
            return true;
        }

        // -----------------------------------------------------------------
        // Caminho moderno: OpenGL 3.3+ core profile
        // -----------------------------------------------------------------

        // 1) Contexto temporario de compatibilidade, apenas para obter os
        //    ponteiros das extensoes WGL (wglCreateContextAttribsARB).
        std::string bootstrapError;
        HGLRC bootstrap = CreateLegacyContext(dc, bootstrapError);
        if (bootstrap != nullptr)
        {
            wglMakeCurrent(dc, bootstrap);
            QueryWglExtensions();
            wglMakeCurrent(NULL, NULL);
            wglDeleteContext(bootstrap);
        }
        else
        {
            outInfo.FallbackReason = bootstrapError;
        }

        if (g_wglCreateContextAttribsARB == nullptr)
        {
            outInfo.FallbackReason = "O driver nao suporta WGL_ARB_create_context (necessario para OpenGL 3.3 core).";
        }
        else if (!HasWglExtension("WGL_ARB_create_context_profile"))
        {
            outInfo.FallbackReason = "O driver nao suporta WGL_ARB_create_context_profile (core profile indisponivel).";
        }

        // 2) Tenta as versoes, da mais alta para a mais baixa.
        struct VersionPair { int Major; int Minor; };
        std::vector<VersionPair> candidates;
        switch (config.GetVersion())
        {
        case GlVersion::V4_5:
            candidates.push_back({ 4, 5 });
            candidates.push_back({ 4, 1 });
            candidates.push_back({ 3, 3 });
            break;
        case GlVersion::V4_1:
            candidates.push_back({ 4, 1 });
            candidates.push_back({ 3, 3 });
            break;
        case GlVersion::V3_3:
        default:
            candidates.push_back({ 3, 3 });
            break;
        }

        HGLRC modern = nullptr;
        std::string modernError;
        for (const VersionPair& candidate : candidates)
        {
            std::string attemptError;
            modern = CreateModernContext(dc, candidate.Major, candidate.Minor, config.GetCoreProfile(), attemptError);
            if (modern != nullptr)
            {
                break;
            }
            modernError = attemptError;
        }

        if (modern == nullptr)
        {
            outInfo.FallbackReason = modernError.empty() ? outInfo.FallbackReason : modernError;

            // 3) Fallback: contexto legado no mesmo DC.
            HGLRC legacy = CreateLegacyContext(dc, outError);
            if (legacy == nullptr)
            {
                return false;
            }
            if (!wglMakeCurrent(dc, legacy))
            {
                outError = "OpenGL Make Current Error.";
                wglDeleteContext(legacy);
                return false;
            }
            outRenderContext = legacy;
            outInfo.Modern = false;
            FillContextInfo(outInfo);
            return true;
        }

        if (!wglMakeCurrent(dc, modern))
        {
            outError = "OpenGL Make Current Error (contexto moderno).";
            wglDeleteContext(modern);
            return false;
        }

        // 4) O GLEW precisa rodar DEPOIS do contexto final estar current.
        const GLenum glewStatus = glewInit();
        if (glewStatus != GLEW_OK)
        {
            outInfo.FallbackReason = "Falha ao carregar as extensoes OpenGL (GLEW): " +
                                     std::string(reinterpret_cast<const char*>(glewGetErrorString(glewStatus)));
        }

        FillContextInfo(outInfo);

        // 5) Valida a versao minima (3.3 core).
        if (outInfo.Major < 3 || (outInfo.Major == 3 && outInfo.Minor < 3))
        {
            outInfo.FallbackReason = "O driver reportou OpenGL " + std::to_string(outInfo.Major) + "." +
                                     std::to_string(outInfo.Minor) + ", inferior ao minimo 3.3.";
        }
        else if (!GLEW_VERSION_3_3)
        {
            outInfo.FallbackReason = "As funcoes de OpenGL 3.3 nao estao disponiveis neste driver.";
        }

        if (!outInfo.FallbackReason.empty())
        {
            // Fallback: apaga o contexto core e recria o legado.
            wglMakeCurrent(NULL, NULL);
            wglDeleteContext(modern);

            HGLRC legacy = CreateLegacyContext(dc, outError);
            if (legacy == nullptr)
            {
                return false;
            }
            if (!wglMakeCurrent(dc, legacy))
            {
                outError = "OpenGL Make Current Error.";
                wglDeleteContext(legacy);
                return false;
            }
            outRenderContext = legacy;
            outInfo.Modern = false;
            FillContextInfo(outInfo);
            return true;
        }

        // 6) Inicializa o renderer moderno (shaders + buffers).
        std::string rendererError;
        if (!Renderer::Instance().InitializeModern(rendererError))
        {
            outInfo.FallbackReason = rendererError;

            wglMakeCurrent(NULL, NULL);
            wglDeleteContext(modern);

            HGLRC legacy = CreateLegacyContext(dc, outError);
            if (legacy == nullptr)
            {
                return false;
            }
            if (!wglMakeCurrent(dc, legacy))
            {
                outError = "OpenGL Make Current Error.";
                wglDeleteContext(legacy);
                return false;
            }
            outRenderContext = legacy;
            outInfo.Modern = false;
            FillContextInfo(outInfo);
            return true;
        }

        outRenderContext = modern;
        outInfo.Modern = true;

        Trace("[gfx] contexto moderno criado: %s | %s", outInfo.Version.c_str(), outInfo.Renderer.c_str());
        return true;
    }

    void GfxContext::Destroy(HGLRC renderContext)
    {
        if (renderContext == nullptr)
        {
            return;
        }
        if (Renderer::Instance().IsModern())
        {
            Renderer::Instance().Shutdown();
        }
        wglMakeCurrent(NULL, NULL);
        wglDeleteContext(renderContext);
    }

    bool GfxContext::SetVSync(HDC dc, bool enabled)
    {
        (void)dc;
        QueryWglExtensions();

        if (g_wglSwapIntervalEXT != nullptr)
        {
            return g_wglSwapIntervalEXT(enabled ? 1 : 0) == TRUE;
        }
        if (g_wglSetSwapIntervalEXT != nullptr)
        {
            return g_wglSetSwapIntervalEXT(enabled ? 1 : 0) == TRUE;
        }
        return false;
    }

    bool GfxContext::SetMultisample(HDC dc, bool enabled)
    {
        (void)dc;
        QueryWglExtensions();

        if (!HasWglExtension("WGL_ARB_multisample"))
        {
            return false;
        }
        if (enabled)
        {
            gfx::raw::fnEnable(GL_MULTISAMPLE);
        }
        else
        {
            gfx::raw::fnDisable(GL_MULTISAMPLE);
        }
        return true;
    }
}
