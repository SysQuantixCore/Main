//======================================================================
// gfx/GfxShader.h
//
// Compilacao/link de programas GLSL + cache de locals de uniform.
//
// Uniforms expostos pelo shader do jogo (ver GfxShaders.h):
//   uProjection, uModelView  (mat4)
//   uTexture                  (sampler2D)
//   uUseTexture, uTexEnvMode, uUseAlphaTest, uAlphaFunc, uUseFog (int)
//   uAlphaRef, uFogStart, uFogEnd (float)
//   uFogColor                 (vec3)
//======================================================================
#pragma once

#include <string>
#include <unordered_map>

#include "gfx/GfxTypes.h"

namespace gfx
{
    class Shader
    {
    public:
        Shader() = default;
        ~Shader();

        Shader(const Shader&) = delete;
        Shader& operator=(const Shader&) = delete;

        // Compila e linka. Retorna false e preenche errorLog em caso de falha.
        bool Compile(const char* vertexSource, const char* fragmentSource, std::string& errorLog);

        void Release();

        GLuint GetProgram() const { return m_Program; }
        bool   IsValid()     const { return m_Program != 0; }

        void Use() const;

        // Uniformes (retorna -1 se a uniform nao existir / foi otimizada).
        void SetInt(const char* name, GLint value);
        void SetFloat(const char* name, GLfloat value);
        void SetVec3(const char* name, GLfloat x, GLfloat y, GLfloat z);
        void SetMat4(const char* name, const float* value);

        const std::string& GetLastError() const { return m_ErrorLog; }

    private:
        GLint GetLocation(const char* name);

        GLuint m_Program = 0;
        GLuint m_VertexShader = 0;
        GLuint m_FragmentShader = 0;

        std::unordered_map<std::string, GLint> m_UniformCache;
        std::string m_ErrorLog;
    };
}
