//======================================================================
// gfx/GfxMath.h
//
// Matematica do renderer: matrizes 4x4 column-major (igual OpenGL/GLM),
// pilha de matrizes que substitui as pilhas fixas do GL
// (GL_PROJECTION / GL_MODELVIEW / GL_TEXTURE) e as projecoes.
//
// REGRA DE OURO DA MIGRACAO
// -------------------------
// As projecoes aqui reproduzem *bit-a-bit* as formulas de
// gluPerspective/gluFrustum/gluOrtho2D do driver original. Nao usar
// glm::perspective() diretamente: a formula do GLM NAO e bit-identica a
// do gluPerspective da Microsoft/Mesa, e usar a errada muda o FOV e
// quebra o alinhamento de tela (ver docs/opengl_analysis.md, secao 5).
//======================================================================
#pragma once

#include <cmath>
#include <vector>

#include "gfx/GfxTypes.h"
#include "gfx/GfxGLM.h"

namespace gfx
{
    using Mat4 = glm::mat4;

    // Modos da antiga glMatrixMode.
    enum class MatrixMode
    {
        ModelView  = 0,   // GL_MODELVIEW
        Projection = 1,   // GL_PROJECTION
        Texture    = 2,   // GL_TEXTURE (nao usado pelo jogo, mantido por compatibilidade)
    };

    // Espelha a pilha de matrizes fixa do OpenGL.
    // Profundidade minima da GL e 32; o jogo nunca passa de 3.
    class MatrixStack
    {
    public:
        MatrixStack();

        void Push();
        void Pop();
        void LoadIdentity();
        void Load(const Mat4& m);

        // Multiplica a matriz do topo:  M = M * m
        // (equivalente a glMultMatrix com a matriz transposta da convencao
        //  glLoadMatrixf, que recebe column-major).
        void Multiply(const Mat4& m);

        void Translate(float x, float y, float z);
        void Rotate(float angleDegrees, float x, float y, float z);
        void Scale(float x, float y, float z);

        const Mat4& Top() const { return m_Stack[m_Stack.size() - 1]; }
        Mat4& Top() { return m_Stack[m_Stack.size() - 1]; }

        size_t Depth() const { return m_Stack.size(); }

    private:
        std::vector<Mat4> m_Stack;
    };

    // -----------------------------------------------------------------
    // Projecoes
    // -----------------------------------------------------------------

    // Reproduz gluPerspective(fovy, aspect, zNear, zFar):
    //   f = 1/tan(fovy/2);  A = f/aspect;  B = f;
    //   gluFrustum(-A, A, -B, B, zNear, zFar)
    Mat4 MakePerspective(float fovYDegrees, float aspect, float zNear, float zFar);

    // Reproduz gluFrustum(left, right, bottom, top, zNear, zFar).
    Mat4 MakeFrustum(float left, float right, float bottom, float top, float zNear, float zFar);

    // Reproduz gluOrtho2D(left, right, bottom, top) == gluOrtho(l,r,b,t,-1,1).
    Mat4 MakeOrtho2D(float left, float right, float bottom, float top);

    // Reproduz gluOrtho(left, right, bottom, top, zNear, zFar).
    Mat4 MakeOrtho(float left, float right, float bottom, float top, float zNear, float zFar);

    // -----------------------------------------------------------------
    // Conversoes
    // -----------------------------------------------------------------

    // Converte a matriz column-major do OpenGL (16 floats) para o formato
    // 3x4 row-major usado pelo jogo (float[3][4]).
    //
    // Reproduz exatamente GetOpenGLMatrix() de ZzzOpenglUtil.cpp:
    //     Matrix[i][j] = OpenGLMatrix[j * 4 + i];
    // Como OpenGLMatrix e column-major, OpenGLMatrix[j*4+i] e o elemento
    // (linha i, coluna j) da matriz em notacao row-major. Logo
    // Matrix[i][j] = linha i, coluna j  ->  row-major.
    void ToRowMajor3x4(const Mat4& m, float out[3][4]);

    // Converte um float[3][4] row-major (formato do jogo) para Mat4 column-major.
    Mat4 FromRowMajor3x4(const float in[3][4]);

    // Compara duas matrizes elemento a elemento (GLM 1.0 nao define
    // operator== / operator!= para mat4 de forma confiavel).
    bool MatricesEqual(const Mat4& a, const Mat4& b);
}
