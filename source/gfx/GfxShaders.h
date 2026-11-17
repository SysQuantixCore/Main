//======================================================================
// gfx/GfxShaders.h
//
// Fonte GLSL do backend moderno.
//
// O shader e unico e cobre tanto o mundo 3D quanto a UI 2D: a diferenca
// esta apenas nas uniforms (uProjection / uModelView) e no estado.
//
// Fidelidade ao fixed-function original:
//   - MODULATE  : C = Cs * Cd            (padrao de todas as texturas)
//   - ADD       : C = Cs * Cd + Cd       (usado em ZzzEffectParticle.cpp)
//   - alpha test: implementa o mesmo comparador de glAlphaFunc (GL_GREATER
//                 no motor original) via discard.
//   - fog       : a coordenada de nevoa e a DISTANCIA do olho
//                 (-z em espaco de view), e nao gl_Position.w. Isso importa
//                 porque a UI 2D usa projecao ortografica (w == 1) e
//                 precisa da mesma formula de nevoa do caminho legado.
//
// Cor do vertice: o motor original calcula a iluminacao na CPU
// (LightTransform) e manda a cor por vertice. O shader apenas a consome,
// nao existe gl_Light* / glMaterial*.
//
// Matriz de cor: o fixed-function usaria GL_MODULATE com a cor primaria.
// Aqui a cor ja vem multiplicada no vertex/fragment shader.
//======================================================================
#pragma once

namespace gfx
{
    // Vertex shader: posicao + texcoord + cor por vertice.
    extern const char* const kVertexShaderSource;

    // Fragment shader: amostragem de textura, environment, alpha test e fog.
    extern const char* const kFragmentShaderSource;
}
