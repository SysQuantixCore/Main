//======================================================================
// gfx/GfxMath.cpp
//======================================================================
#include "stdafx.h"
#include "gfx/GfxMath.h"

namespace gfx
{
    namespace
    {
        constexpr double kPi = 3.14159265358979323846;
    }

    MatrixStack::MatrixStack()
    {
        m_Stack.reserve(8);
        m_Stack.push_back(Mat4(1.0f));
    }

    void MatrixStack::Push()
    {
        // A pilha da GL tem profundidade minima garantida de 32.
        if (m_Stack.size() < 64)
        {
            m_Stack.push_back(m_Stack.back());
        }
    }

    void MatrixStack::Pop()
    {
        // glPopMatrix com pilha de tamanho 1 e erro no driver; protegemos
        // para nao corromper o estado durante a migracao.
        if (m_Stack.size() > 1)
        {
            m_Stack.pop_back();
        }
    }

    void MatrixStack::LoadIdentity()
    {
        m_Stack.back() = Mat4(1.0f);
    }

    void MatrixStack::Load(const Mat4& m)
    {
        m_Stack.back() = m;
    }

    void MatrixStack::Multiply(const Mat4& m)
    {
        // Post-multiplicacao, igual a glMultMatrix.
        m_Stack.back() = m_Stack.back() * m;
    }

    void MatrixStack::Translate(float x, float y, float z)
    {
        // glm::translate faz M = M * T, identico a glTranslatef.
        m_Stack.back() = glm::translate(m_Stack.back(), glm::vec3(x, y, z));
    }

    void MatrixStack::Rotate(float angleDegrees, float x, float y, float z)
    {
        // -----------------------------------------------------------------
        // Rotacao NO ESTILO DA OPENGL (glRotatef), construida a mao.
        // -----------------------------------------------------------------
        // Por que nao usar glm::rotate()?
        //   1. A matriz de rotacao do GLM e a TRANSPOSTA da da OpenGL
        //      (convencao matematica vs. convencao column-major do GL).
        //   2. O glm::rotate() pre-multiplica (R * M) enquanto o glRotatef
        //      pos-multiplica (M = M * R). Isso troca a ordem entre a
        //      rotacao e a translacao e desloca a camera inteira.
        //
        // Sintoma observado (2026-11-15): com o glm::rotate() a cena 3D
        // do login aparecia "de cabeca para baixo" (o mar no alto), o
        // navio sumia e o ceu nao aparecia.
        //
        // A matriz abaixo segue a formula da especificacao do OpenGL
        // (glRotatef), em notacao row-major:
        //   | c+x^2(1-c)  xy(1-c)-zs  xz(1-c)+ys  0 |
        //   | yx(1-c)+zs   c+y^2(1-c)  yz(1-c)-xs  0 |
        //   | zx(1-c)-ys   zy(1-c)+xs  c+z^2(1-c)  0 |
        //   | 0            0            0           1 |
        // -----------------------------------------------------------------
        const float len = std::sqrt(x * x + y * y + z * z);
        if (len <= 0.0f)
        {
            return;
        }
        x /= len;
        y /= len;
        z /= len;

        const float a = static_cast<float>(angleDegrees * kPi / 180.0);
        const float s = std::sin(a);
        const float c = std::cos(a);
        const float omc = 1.0f - c;


        // Matriz column-major: m[coluna][linha].
        // element(linha, coluna) = m[coluna][linha]; por isso a linha 0 da
        // formula da especificacao fica distribuida em m[1][0], m[2][0], etc.
        Mat4 r(1.0f);
        r[0][0] = c + x * x * omc;   r[1][0] = x * y * omc - z * s;   r[2][0] = x * z * omc + y * s;
        r[0][1] = x * y * omc + z * s; r[1][1] = c + y * y * omc;       r[2][1] = y * z * omc - x * s;
        r[0][2] = x * z * omc - y * s; r[1][2] = y * z * omc + x * s;   r[2][2] = c + z * z * omc;

        // M = M * R (pos-multiplicacao, igual ao glRotatef).
        m_Stack.back() = m_Stack.back() * r;
    }

    void MatrixStack::Scale(float x, float y, float z)
    {
        // glm::scale faz M = M * S, identico a glScalef.
        m_Stack.back() = glm::scale(m_Stack.back(), glm::vec3(x, y, z));
    }

    Mat4 MakeFrustum(float left, float right, float bottom, float top, float zNear, float zFar)
    {
        // Reproducao literal de gluFrustum(). O GL armazena a matriz em
        // column-major: m[coluna][linha].
        Mat4 m(1.0f);

        m[0][0] = (2.0f * zNear) / (right - left);
        m[0][1] = 0.0f;
        m[0][2] = 0.0f;
        m[0][3] = 0.0f;

        m[1][0] = 0.0f;
        m[1][1] = (2.0f * zNear) / (top - bottom);
        m[1][2] = 0.0f;
        m[1][3] = 0.0f;

        m[2][0] = (right + left) / (right - left);
        m[2][1] = (top + bottom) / (top - bottom);
        m[2][2] = (zFar + zNear) / (zNear - zFar);

        m[3][0] = 0.0f;
        m[3][1] = 0.0f;
        m[3][2] = -1.0f;  // linha 3, coluna 2  (w = -z_olho)
        m[3][3] = 0.0f;

        // linha 2, coluna 3: o offset de z
        m[2][3] = (2.0f * zFar * zNear) / (zNear - zFar);

        return m;
    }

    Mat4 MakePerspective(float fovYDegrees, float aspect, float zNear, float zFar)
    {
        // -----------------------------------------------------------------
        // Reproducao LITERAL da gluPerspective da MICROSOFT (glu32.dll).
        // -----------------------------------------------------------------
        //
        // ATENCAO: a implementacao da Microsoft NAO delega para
        // gluFrustum. Ela monta a matriz de projecao diretamente:
        //
        //     f = 1 / tan(fovy/2)
        //     A = f / aspect      ->  m00 = A
        //     B = f               ->  m11 = B
        //     m22 = (zFar+zNear)/(zNear-zFar)
        //     m23 = 2*zFar*zNear/(zNear-zFar)
        //     m32 = -1
        //
        // Isso e DIFERENTE da formula "padrao" do OpenGL, que passa por
        // gluFrustum e usa m00 = 2*zNear/(right-left) = zNear/A e
        // m11 = 2*zNear/(top-bottom) = zNear/B. A versao da Microsoft
        // NAO tem o fator zNear, o que torna o FOV efetivo bem menor
        // (com near=20 e fovy=45 graus, o FOV vertical efetivo cai de
        // 45 para ~13,8 graus).
        //
        // Consequencia pratica (validada em 2026-11-15): com a formula
        // "padrao" a cena 3D do login/selecao de personagem aparecia
        // com a camera muito afastada, o ceu nao aparecia e o navio
        // ficava achatado - exatamente o sintoma relatado.
        //
        // Ver docs/migration_notes.md, secao "Projecao de camera".
        const double f = 1.0 / std::tan((static_cast<double>(fovYDegrees) / 2.0) * kPi / 180.0);
        const float A = static_cast<float>(f / static_cast<double>(aspect));
        const float B = static_cast<float>(f);

        // Matriz column-major: m[coluna][linha].
        // CUIDADO: element(linha, coluna) = m[coluna][linha].
        //   element(2,2) -> m[2][2]
        //   element(3,2) -> m[2][3]   (a linha 3, coluna 2: w = -z_olho)
        //   element(2,3) -> m[3][2]   (a linha 2, coluna 3: o offset de z)
        Mat4 m(1.0f);

        m[0][0] = A;      // linha 0, coluna 0
        m[1][1] = B;      // linha 1, coluna 1
        m[2][2] = (zFar + zNear) / (zNear - zFar);
        m[2][3] = -1.0f;  // linha 3, coluna 2  (w = -z_olho)
        m[3][2] = (2.0f * zFar * zNear) / (zNear - zFar);  // linha 2, coluna 3
        m[3][3] = 0.0f;

        return m;
    }

    Mat4 MakeOrtho(float left, float right, float bottom, float top, float zNear, float zFar)
    {
        // Reproducao literal de gluOrtho().
        Mat4 m(1.0f);

        m[0][0] = 2.0f / (right - left);
        m[1][1] = 2.0f / (top - bottom);
        m[2][2] = -2.0f / (zFar - zNear);

        m[3][0] = -(right + left) / (right - left);
        m[3][1] = -(top + bottom) / (top - bottom);
        m[3][2] = -(zFar + zNear) / (zFar - zNear);

        return m;
    }

    Mat4 MakeOrtho2D(float left, float right, float bottom, float top)
    {
        // gluOrtho2D == gluOrtho(left, right, bottom, top, -1.0, 1.0)
        return MakeOrtho(left, right, bottom, top, -1.0f, 1.0f);
    }

    void ToRowMajor3x4(const Mat4& m, float out[3][4])
    {
        // GetOpenGLMatrix(): Matrix[i][j] = OpenGLMatrix[j * 4 + i]
        // OpenGLMatrix e column-major => OpenGLMatrix[j*4+i] = linha i, coluna j.
        // glm::mat4 tambem e column-major => m[j][i] = linha i, coluna j.
        for (int i = 0; i < 3; ++i)
        {
            for (int j = 0; j < 4; ++j)
            {
                out[i][j] = m[j][i];
            }
        }
    }

    Mat4 FromRowMajor3x4(const float in[3][4])
    {
        Mat4 m(1.0f);
        for (int i = 0; i < 3; ++i)
        {
            for (int j = 0; j < 4; ++j)
            {
                m[j][i] = in[i][j];
            }
        }
        return m;
    }

    bool MatricesEqual(const Mat4& a, const Mat4& b)
    {
        const float* pa = glm::value_ptr(a);
        const float* pb = glm::value_ptr(b);
        for (int i = 0; i < 16; ++i)
        {
            if (pa[i] != pb[i])
            {
                return false;
            }
        }
        return true;
    }
}
