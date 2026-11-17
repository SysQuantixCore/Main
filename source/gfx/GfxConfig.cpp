//======================================================================
// gfx/GfxConfig.cpp
//======================================================================
#include "stdafx.h"
#include "gfx/GfxConfig.h"
#include "gfx/GfxTrace.h"

#include <cstdlib>
#include <cstring>
#include <string>

namespace gfx
{
    namespace
    {
        bool EqualsNoCase(const char* a, const char* b)
        {
            return a && b && (_stricmp(a, b) == 0);
        }

        // Caminho absoluto do Graphics.ini: sempre ao lado do executavel.
        // Usar um caminho RELATIVO depende do diretorio de trabalho do
        // processo, que pode nao ser a pasta do jogo (o cliente e
        // executado com privilegio de administrador em alguns casos e
        // pode mudar o CWD) - nesse caso o .ini era silenciosamente
        // ignorado e o cliente caia no backend legado.
        const std::string& IniPath()
        {
            static std::string path;
            if (path.empty())
            {
                char module[MAX_PATH] = { 0 };
                if (GetModuleFileNameA(nullptr, module, sizeof(module)) != 0)
                {
                    char* lastSlash = strrchr(module, '\\');
                    if (lastSlash != nullptr)
                    {
                        *(lastSlash + 1) = '\0';
                        path = std::string(module) + "Graphics.ini";
                    }
                }
                if (path.empty())
                {
                    path = "Graphics.ini";
                }
            }
            return path;
        }

        // Le um valor de Graphics.ini. Devolve false quando a chave nao existe.
        bool ReadIniString(const char* section, const char* key, const char* fallback, std::string& out)
        {
            char buffer[64] = { 0 };
            GetPrivateProfileStringA(section, key, fallback, buffer, sizeof(buffer), IniPath().c_str());
            out = buffer;
            return buffer[0] != '\0';
        }

        bool ReadIniBool(const char* section, const char* key, bool fallback, bool& out)
        {
            std::string value;
            if (!ReadIniString(section, key, "", value) || value.empty())
            {
                out = fallback;
                return false;
            }
            if (EqualsNoCase(value.c_str(), "1") || EqualsNoCase(value.c_str(), "true") ||
                EqualsNoCase(value.c_str(), "yes") || EqualsNoCase(value.c_str(), "on"))
            {
                out = true;
                return true;
            }
            if (EqualsNoCase(value.c_str(), "0") || EqualsNoCase(value.c_str(), "false") ||
                EqualsNoCase(value.c_str(), "no") || EqualsNoCase(value.c_str(), "off"))
            {
                out = false;
                return true;
            }
            out = fallback;
            return false;
        }

        // Extrai "--chave=valor" da linha de comando.
        bool FindCommandLineArg(const char* name, std::string& outValue, bool& outHasValue)
        {
            int    argc = 0;
            LPWSTR* argv = ::CommandLineToArgvW(::GetCommandLineW(), &argc);
            if (!argv)
            {
                return false;
            }

            bool  found = false;
            bool  hasValue = false;
            std::string value;

            const std::string prefix = std::string("--") + name;
            for (int i = 1; i < argc; ++i)
            {
                char narrow[256] = { 0 };
                WideCharToMultiByte(CP_ACP, 0, argv[i], -1, narrow, sizeof(narrow) - 1, nullptr, nullptr);

                const std::string arg(narrow);
                if (arg.size() > prefix.size() &&
                    arg.compare(0, prefix.size(), prefix) == 0 &&
                    arg[prefix.size()] == '=')
                {
                    value = arg.substr(prefix.size() + 1);
                    hasValue = true;
                    found = true;
                    break;
                }
                if (arg == prefix)
                {
                    found = true;
                    hasValue = false;
                    break;
                }
            }

            LocalFree(argv);
            if (found)
            {
                outValue = value;
                outHasValue = hasValue;
            }
            return found;
        }

        bool CommandLineFlag(const char* name)
        {
            std::string value;
            bool hasValue = false;
            if (!FindCommandLineArg(name, value, hasValue))
            {
                return false;
            }
            if (!hasValue)
            {
                return true;
            }
            return !(EqualsNoCase(value.c_str(), "0") || EqualsNoCase(value.c_str(), "false") ||
                     EqualsNoCase(value.c_str(), "off"));
        }
    }

    Config& Config::Instance()
    {
        static Config instance;
        return instance;
    }

    void Config::Load()
    {
        if (m_Loaded)
        {
            return;
        }
        m_Loaded = true;

        LoadIni();
        ApplyCommandLine();

        // O trace so e ligado se o .ini (ou a linha de comando) pedir.
        gfx::TraceEnable(m_Verbose);

        gfx::Trace("[gfx] Config: backend=%s | versao=%s vsync=%d widescreen=%d coreprofile=%d",
            GetBackendName(), GetVersionName(), (int)m_VSync, (int)m_Widescreen,
            (int)m_CoreProfile);
    }

    void Config::LoadIni()
    {
        std::string value;

        // [Graphics] Renderer = legacy | modern | auto
        if (ReadIniString("Graphics", "Renderer", "", value))
        {
            if (EqualsNoCase(value.c_str(), "modern") || EqualsNoCase(value.c_str(), "moderno"))
            {
                m_Backend = Backend::Modern;
            }
            else if (EqualsNoCase(value.c_str(), "legacy") || EqualsNoCase(value.c_str(), "legado"))
            {
                m_Backend = Backend::Legacy;
            }
            else if (EqualsNoCase(value.c_str(), "auto"))
            {
                // "auto": tenta o moderno e cai para o legado se o driver
                // nao suportar. O fallback real acontece em GfxContext.
                m_Backend = Backend::Modern;
            }
            else
            {
                // Valor desconhecido: assume "auto" (moderno com
                // fallback) e avisa, para nao passar despercebido.
                m_Backend = Backend::Modern;
                OutputDebugStringA("[gfx] Graphics.ini: valor invalido em Renderer. Use legacy, modern ou auto.\n");
            }
        }
        else
        {
            // Sem Graphics.ini -> modo legado (comportamento atual).
            m_Backend = Backend::Legacy;
        }

        // [Graphics] RequestedVersion = 3.3 | 4.1 | 4.5
        if (ReadIniString("Graphics", "RequestedVersion", "", value))
        {
            if (EqualsNoCase(value.c_str(), "4.5"))
            {
                m_Version = GlVersion::V4_5;
            }
            else if (EqualsNoCase(value.c_str(), "4.1"))
            {
                m_Version = GlVersion::V4_1;
            }
            else if (EqualsNoCase(value.c_str(), "3.3"))
            {
                m_Version = GlVersion::V3_3;
            }
            else
            {
                m_Version = GlVersion::V3_3;
            }
        }

        ReadIniBool("Graphics", "VSync", false, m_VSync);
        ReadIniBool("Graphics", "Widescreen", false, m_Widescreen);
        ReadIniBool("Graphics", "Verbose", false, m_Verbose);
        ReadIniBool("Graphics", "CoreProfile", false, m_CoreProfile);
    }

    void Config::ApplyCommandLine()
    {
        std::string value;
        bool hasValue = false;

        if (FindCommandLineArg("renderer", value, hasValue) && hasValue)
        {
            if (EqualsNoCase(value.c_str(), "modern"))
            {
                m_Backend = Backend::Modern;
            }
            else if (EqualsNoCase(value.c_str(), "legacy"))
            {
                m_Backend = Backend::Legacy;
            }
            else if (EqualsNoCase(value.c_str(), "auto"))
            {
                m_Backend = Backend::Modern;
            }
        }

        if (FindCommandLineArg("glver", value, hasValue) && hasValue)
        {
            if (EqualsNoCase(value.c_str(), "4.5"))
            {
                m_Version = GlVersion::V4_5;
            }
            else if (EqualsNoCase(value.c_str(), "4.1"))
            {
                m_Version = GlVersion::V4_1;
            }
            else
            {
                m_Version = GlVersion::V3_3;
            }
        }

        if (FindCommandLineArg("vsync", value, hasValue) && hasValue)
        {
            m_VSync = !(EqualsNoCase(value.c_str(), "0") || EqualsNoCase(value.c_str(), "false") ||
                        EqualsNoCase(value.c_str(), "off"));
        }

        if (FindCommandLineArg("widescreen", value, hasValue) && hasValue)
        {
            m_Widescreen = !(EqualsNoCase(value.c_str(), "0") || EqualsNoCase(value.c_str(), "false") ||
                             EqualsNoCase(value.c_str(), "off"));
        }

        if (CommandLineFlag("gfx-verbose"))
        {
            m_Verbose = true;
        }
    }

    const char* Config::GetBackendName() const
    {
        switch (m_Backend)
        {
        case Backend::Modern: return "modern (GLSL + VBO/VAO)";
        case Backend::Legacy: return "legacy (fixed-function)";
        default:              return "unknown";
        }
    }

    const char* Config::GetVersionName() const
    {
        switch (m_Version)
        {
        case GlVersion::V4_5: return "4.5";
        case GlVersion::V4_1: return "4.1";
        case GlVersion::V3_3: return "3.3";
        default:              return "auto";
        }
    }
}
