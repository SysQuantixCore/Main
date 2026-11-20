//======================================================================
// gfx/GfxConfig.h
//
// Configuracao do renderer: escolha de backend (legado / moderno),
// versao de OpenGL alvo, VSync e modo widescreen.
//
// Fontes de configuracao, em ordem de precedencia (maior primeiro):
//   1. Linha de comando:  --renderer=modern|legacy|auto
//                         --glver=3.3|4.1|4.5
//                         --vsync=0|1
//                         --widescreen=0|1
//                         --gfx-verbose
//   2. Graphics.ini (na raiz do executavel)
//
// Objetivo do arquivo e documentado em docs/migration_notes.md.
//======================================================================
#pragma once

#include <string>

namespace gfx
{
    // Qual pipeline de render sera usado.
    enum class Backend
    {
        // Pipeline fixo original (fixed-function). Contexto OpenGL 1.1
        // de compatibilidade. Comportamento atual do jogo, bit-a-bit.
        Legacy = 0,

        // VBOs/VAOs + shaders GLSL. Contexto OpenGL 3.3 core profile
        // (ou superior). Modo de teste/validacao.
        Modern = 1,
    };

    // Versao minima de OpenGL requisitada ao driver para o backend moderno.
    enum class GlVersion
    {
        Auto = 0,
        V3_3 = 33,
        V4_1 = 41,
        V4_5 = 45,
    };

    class Config
    {
    public:
        static Config& Instance();

        // Le Graphics.ini + linha de comando. Idempotente: apenas a
        // primeira chamada tem efeito.
        void Load();

        Backend   GetBackend()    const { return m_Backend; }
        GlVersion GetVersion()    const { return m_Version; }
        bool      GetVSync()      const { return m_VSync; }
        bool      GetWidescreen() const { return m_Widescreen; }
        bool      GetVerbose()    const { return m_Verbose; }
        bool      GetCoreProfile() const { return m_CoreProfile; }
        bool      GetBatch()       const { return m_Batch; }
        bool      GetMapped()      const { return m_Mapped; }
        bool      GetBindless()    const { return m_Bindless; }

        bool IsModern() const { return m_Backend == Backend::Modern; }
        bool IsLegacy() const { return m_Backend == Backend::Legacy; }

        const char* GetBackendName() const;
        const char* GetVersionName() const;

        // Caminho do arquivo .ini efetivamente lido (apenas diagnostico).
        const std::string& GetIniPath() const { return m_IniPath; }

        // Motivo pelo qual o backend moderno foi rejeitado/desejado.
        // Vazio enquanto nada aconteceu.
        const std::string& GetStatusMessage() const { return m_StatusMessage; }
        void SetStatusMessage(const std::string& msg) { m_StatusMessage = msg; }

    private:
        Config() = default;
        Config(const Config&) = delete;
        Config& operator=(const Config&) = delete;

        void LoadIni();
        void ApplyCommandLine();

        Backend   m_Backend    = Backend::Legacy;
        GlVersion m_Version    = GlVersion::V3_3;
        bool      m_VSync      = false;
        bool      m_Widescreen = false;
        bool      m_Verbose    = false;
        bool      m_CoreProfile = false;
        // Mapped VBO DESLIGADO: 6 variantes testadas (ring, reset frame,
        // 4 regioes, fence, fence bloqueante, COHERENT+flush) todas corrompem
        // a imagem neste driver Intel, apesar do ganho de FPS (14->64).
        // Ver docs/analise_estabilidade_moderno.md.
        bool      m_Batch       = false;
        bool      m_Mapped      = false;
        bool      m_Bindless    = false;
        bool      m_Loaded     = false;

        std::string m_IniPath      = "Graphics.ini";
        std::string m_StatusMessage;
    };
}
