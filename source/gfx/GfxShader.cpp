//======================================================================
// gfx/GfxShader.cpp
//======================================================================
#include "stdafx.h"
#include "gfx/GfxShader.h"
#include "gfx/GfxTrace.h"

namespace gfx
{
    namespace
    {
        // Compila um shader e devolve o id (0 em caso de erro).
        GLuint CompileStage(GLenum type, const char* source, std::string& errorLog)
        {
            const GLuint shader = glCreateShader(type);
            if (shader == 0)
            {
                errorLog += "Falha ao criar o shader object.\r\n";
                return 0;
            }

            glShaderSource(shader, 1, &source, nullptr);
            glCompileShader(shader);

            GLint compiled = GL_FALSE;
            glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);

            GLint logLength = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
            if (logLength > 1)
            {
                std::string log(static_cast<size_t>(logLength), '\0');
                glGetShaderInfoLog(shader, logLength, nullptr, &log[0]);
                errorLog += log;
                errorLog += "\r\n";
            }

            if (compiled != GL_TRUE)
            {
                glDeleteShader(shader);
                return 0;
            }

            return shader;
        }
    }

    Shader::~Shader()
    {
        Release();
    }

    void Shader::Release()
    {
        if (m_Program != 0)
        {
            glDeleteProgram(m_Program);
            m_Program = 0;
        }
        if (m_VertexShader != 0)
        {
            glDeleteShader(m_VertexShader);
            m_VertexShader = 0;
        }
        if (m_FragmentShader != 0)
        {
            glDeleteShader(m_FragmentShader);
            m_FragmentShader = 0;
        }
        m_UniformCache.clear();
    }

    bool Shader::Compile(const char* vertexSource, const char* fragmentSource, std::string& errorLog)
    {
        Release();
        m_ErrorLog.clear();

        m_VertexShader = CompileStage(GL_VERTEX_SHADER, vertexSource, m_ErrorLog);
        if (m_VertexShader == 0)
        {
            errorLog = m_ErrorLog;
            return false;
        }

        m_FragmentShader = CompileStage(GL_FRAGMENT_SHADER, fragmentSource, m_ErrorLog);
        if (m_FragmentShader == 0)
        {
            errorLog = m_ErrorLog;
            glDeleteShader(m_VertexShader);
            m_VertexShader = 0;
            return false;
        }

        m_Program = glCreateProgram();
        glAttachShader(m_Program, m_VertexShader);
        glAttachShader(m_Program, m_FragmentShader);
        glLinkProgram(m_Program);

        GLint linked = GL_FALSE;
        glGetProgramiv(m_Program, GL_LINK_STATUS, &linked);

        GLint logLength = 0;
        glGetProgramiv(m_Program, GL_INFO_LOG_LENGTH, &logLength);
        if (logLength > 1)
        {
            std::string log(static_cast<size_t>(logLength), '\0');
            glGetProgramInfoLog(m_Program, logLength, nullptr, &log[0]);
            m_ErrorLog += log;
            m_ErrorLog += "\r\n";
        }

        if (linked != GL_TRUE)
        {
            errorLog = m_ErrorLog;
            Release();
            return false;
        }

        // Os shaders podem ser descartados depois do link.
        glDetachShader(m_Program, m_VertexShader);
        glDetachShader(m_Program, m_FragmentShader);
        glDeleteShader(m_VertexShader);
        glDeleteShader(m_FragmentShader);
        m_VertexShader = 0;
        m_FragmentShader = 0;

        return true;
    }

    void Shader::Use() const
    {
        if (m_Program != 0)
        {
            glUseProgram(m_Program);
        }
    }

    GLint Shader::GetLocation(const char* name)
    {
        const auto it = m_UniformCache.find(name);
        if (it != m_UniformCache.end())
        {
            return it->second;
        }

        const GLint location = glGetUniformLocation(m_Program, name);
        m_UniformCache.emplace(name, location);

        gfx::Trace("[gfx] uniform '%s' -> location %d", name, (int)location);
        return location;
    }

    void Shader::SetInt(const char* name, GLint value)
    {
        const GLint location = GetLocation(name);
        if (location >= 0)
        {
            glUniform1i(location, value);
        }
    }

    void Shader::SetFloat(const char* name, GLfloat value)
    {
        const GLint location = GetLocation(name);
        if (location >= 0)
        {
            glUniform1f(location, value);
        }
    }

    void Shader::SetVec3(const char* name, GLfloat x, GLfloat y, GLfloat z)
    {
        const GLint location = GetLocation(name);
        if (location >= 0)
        {
            glUniform3f(location, x, y, z);
        }
    }

    void Shader::SetMat4(const char* name, const float* value)
    {
        const GLint location = GetLocation(name);
        if (location >= 0)
        {
            glUniformMatrix4fv(location, 1, GL_FALSE, value);
        }
    }
}
