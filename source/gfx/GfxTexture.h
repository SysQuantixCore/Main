//======================================================================
// gfx/GfxTexture.h
//
// TextureManager do modulo gfx.
//
// O jogo carrega as texturas em CGlobalBitmap (GlobalBitmap.cpp), que
// mantem o formato original em disco (.OZJ/.OZT) e o estado de cada
// bitmap. Este modulo cuida apenas do lado OpenGL:
//
//   - criacao com internal format "sized" (exigido pelo core profile);
//   - conversao de textura de canal unico (alpha) para RGBA, preservando
//     exatamente a semantica do antigo GL_ALPHA (RGB = 0);
//   - filtros, wrap e anisotropy.
//
// A compatibilidade com o pipeline fixo e o ponto central: o motor
// legado usava internal formats nao sized (GL_RGB = 3, GL_RGBA = 4),
// que em core profile geram GL_INVALID_ENUM e deixam a textura
// incompleta.
//======================================================================
#pragma once

#include <vector>

#include "gfx/GfxTypes.h"

namespace gfx
{
    class TextureManager
    {
    public:
        // Cria uma textura RGBA a partir de um buffer RGBA de 4 bytes/pixel.
        static GLuint CreateFromRGBA(int width, int height, const unsigned char* rgba,
                                     bool generateMipmaps = false);

        // Cria uma textura a partir de um buffer de ALPHA de 1 byte/pixel.
        //
        // O core profile nao aceita GL_ALPHA como internal format. Para
        // manter o resultado EXATO do legado (a textura devolve
        // (0, 0, 0, cobertura)), convertemos para RGBA com RGB zerado.
        static GLuint CreateFromAlpha(int width, int height, const unsigned char* alpha,
                                      bool generateMipmaps = false);

        // Converte um buffer de alpha (1 byte/pixel) para RGBA com RGB = 0.
        // Equivalente ao que uma textura GL_ALPHA devolveria.
        static std::vector<unsigned char> ExpandAlphaToRGBA(int width, int height, const unsigned char* alpha);

        // Aplica os parametros de amostragem usados pelo motor legado.
        static void ApplySampling(GLuint texture, GLenum minFilter, GLenum magFilter, GLenum wrapMode);

        // Ativa a filtragem anisotropica (se a extensao existir).
        // O motor legado fixa o valor em 4x.
        static void ApplyAnisotropy(GLuint texture, float maxAnisotropy = 4.0f);

        static void Delete(GLuint texture);
    };
}
