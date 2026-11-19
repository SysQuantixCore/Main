//======================================================================
// gfx/GfxShaders.cpp
//======================================================================
#include "stdafx.h"
#include "gfx/GfxShaders.h"

namespace gfx
{
    const char* const kVertexShaderSource = R"(#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aTexCoord;
layout(location = 2) in vec4 aColor;

uniform mat4 uProjection;
uniform mat4 uModelView;

out vec2  vTexCoord;
out vec4  vColor;
out float vFogDepth;

void main()
{
    // Eye space: o fixed-function compoe clip = Projection * ModelView * v.
    vec4 eyePos = uModelView * vec4(aPos, 1.0);
    gl_Position = uProjection * eyePos;

    vTexCoord = aTexCoord;
    vColor    = aColor;

    // Distancia ao olho. O GL usa o z (negativo a frente) do espaco de
    // view como coordenada de nevoa. Nao podemos usar gl_Position.w porque
    // a projecao ortografica da UI tem w == 1.
    vFogDepth = -eyePos.z;
}
)";

    // -----------------------------------------------------------------
    // Shader FRAGMENT - versao CLASSICA (fallback, sampler2D + unidade 0).
    // -----------------------------------------------------------------
    const char* const kFragmentShaderSource = R"(#version 330 core

in vec2  vTexCoord;
in vec4  vColor;
in float vFogDepth;

uniform sampler2D uTexture;
uniform int   uUseTexture;
uniform int   uTexEnvMode;
uniform int   uUseAlphaTest;
uniform int   uAlphaFunc;
uniform float uAlphaRef;
uniform int   uUseFog;
uniform vec3  uFogColor;
uniform float uFogStart;
uniform float uFogEnd;

out vec4 FragColor;

void main()
{
    vec4 tex = vec4(1.0, 1.0, 1.0, 1.0);
    if (uUseTexture != 0)
    {
        tex = texture(uTexture, vTexCoord);
    }

    vec4 c;
    if (uTexEnvMode == 1)          // GL_ADD
    {
        c = tex * vColor + vColor;
        c.a = min(c.a, 1.0);
    }
    else if (uTexEnvMode == 2)     // GL_REPLACE
    {
        c = tex;
    }
    else                           // GL_MODULATE
    {
        c = tex * vColor;
    }
    c.a = tex.a * vColor.a;

    if (uUseAlphaTest != 0)
    {
        bool pass = true;
        if      (uAlphaFunc == 0) pass = false;
        else if (uAlphaFunc == 1) pass = (c.a <  uAlphaRef);
        else if (uAlphaFunc == 2) pass = (c.a == uAlphaRef);
        else if (uAlphaFunc == 3) pass = (c.a <= uAlphaRef);
        else if (uAlphaFunc == 4) pass = (c.a >  uAlphaRef);
        else if (uAlphaFunc == 5) pass = (c.a != uAlphaRef);
        else if (uAlphaFunc == 6) pass = (c.a >= uAlphaRef);
        if (!pass) discard;
    }

    if (uUseFog != 0)
    {
        float f = clamp((uFogEnd - vFogDepth) / (uFogEnd - uFogStart), 0.0, 1.0);
        c.rgb = c.rgb * f + uFogColor * (1.0 - f);
    }

    FragColor = c;
}
)";

    // -----------------------------------------------------------------
    // Shader FRAGMENT - versao BINDLESS (GL_ARB_bindless_texture).
    // O handle 64-bit identifica a textura sem glBindTexture por draw.
    // -----------------------------------------------------------------
    const char* const kFragmentShaderBindless = R"(#version 330 core
#extension GL_ARB_bindless_texture : require

in vec2  vTexCoord;
in vec4  vColor;
in float vFogDepth;

// Handle 64-bit: declarado como sampler2D. A extensao ARB_bindless_texture
// permite carregar o handle via glUniformHandleui64ARB diretamente neste
// sampler, sem glBindTexture.
uniform sampler2D uTextureHandle;
uniform int   uUseTexture;      // 0 = textura desligada
uniform int   uTexEnvMode;     // 0 = MODULATE, 1 = ADD, 2 = REPLACE
uniform int   uUseAlphaTest;   // 0 = sem alpha test
uniform int   uAlphaFunc;      // ver gfx::AlphaFunc
uniform float uAlphaRef;
uniform int   uUseFog;
uniform vec3  uFogColor;
uniform float uFogStart;
uniform float uFogEnd;

out vec4 FragColor;

void main()
{
    vec4 tex = vec4(1.0, 1.0, 1.0, 1.0);
    if (uUseTexture != 0)
    {
        tex = texture(uTextureHandle, vTexCoord);
    }

    vec4 c;
    if (uTexEnvMode == 1)          // GL_ADD    : C = Cs*Cd + Cd
    {
        c = tex * vColor + vColor;
        // Color buffer de 8 bits: o fixed-function clampava alpha em [0,1]
        // antes do blending.
        c.a = min(c.a, 1.0);
    }
    else if (uTexEnvMode == 2)     // GL_REPLACE: C = Cs
    {
        c = tex;
    }
    else                           // GL_MODULATE: C = Cs*Cd
    {
        c = tex * vColor;
    }

    // ---------------------------------------------------------------------
    // Canal ALPHA
    // ---------------------------------------------------------------------
    // O alpha do fragmento vem **apenas da textura** (As).
    //
    // O equivalente ao GL_MODULATE do fixed-function seria As * Ad (alpha
    // da cor primaria). Em varios pontos do motor o alpha da cor chega
    // zerado (por exemplo, quando o codigo do jogo usa glColor3fv, que
    // em compatibilidade deixa o alpha corrente em 1.0, mas cujo valor
    // nao e propagado para o cache do modulo gfx da mesma forma), e o
    // pixel inteiro desaparecia (alpha test + blending).
    //
    // Impacto: efeitos que usam alpha na COR PRIMARIA (glColor4f com a<1)
    // passam a usar o alpha da textura. Deve ser reavaliado na Fase 2
    // (sugestao: uniform "uUseColorAlpha" para ligar/desligar).
    c.a = tex.a * vColor.a;

    if (uUseAlphaTest != 0)
    {
        bool pass = true;
        if      (uAlphaFunc == 0) pass = false;             // NEVER
        else if (uAlphaFunc == 1) pass = (c.a <  uAlphaRef); // LESS
        else if (uAlphaFunc == 2) pass = (c.a == uAlphaRef); // EQUAL
        else if (uAlphaFunc == 3) pass = (c.a <= uAlphaRef); // LEQUAL
        else if (uAlphaFunc == 4) pass = (c.a >  uAlphaRef); // GREATER
        else if (uAlphaFunc == 5) pass = (c.a != uAlphaRef); // NOTEQUAL
        else if (uAlphaFunc == 6) pass = (c.a >= uAlphaRef); // GEQUAL
        else                      pass = true;              // ALWAYS

        if (!pass)
        {
            discard;
        }
    }

    if (uUseFog != 0)
    {
        // Mesma formula do GL para GL_LINEAR:
        //   f = (end - z) / (end - start), limitado a [0,1]
        //   C = f*C + (1-f)*Cfog
        float f = clamp((uFogEnd - vFogDepth) / (uFogEnd - uFogStart), 0.0, 1.0);
        c.rgb = c.rgb * f + uFogColor * (1.0 - f);
    }

    FragColor = c;
}
)";}
