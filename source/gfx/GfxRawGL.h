//======================================================================
// gfx/GfxRawGL.h
//
// Ponteiros para as funcoes OpenGL 1.1 "cruas" (fixed-function).
//
// Sao capturados do opengl32.dll/glu32.dll no inicio do modulo e usados
// pelo backend legado, que precisa chamar a GL original para preservar
// bit-a-bit o comportamento atual do jogo.
//
// Estes ponteiros tambem sao a unica forma de chamar a GL crua depois que
// as macros de redirecionamento (GfxGLAdapter.h) substituem os nomes
// glBegin/glEnd/... por despachantes do modulo gfx.
//
// O prefixo "fn" existe por causa da macro do projeto
//     #define Color4f(r, b, g, o) ((o << 24) + (g << 16) + (b << 8) + r)
// (CBInterface.h) que colidiria com nomes como "Color4f".
//======================================================================
#pragma once

#include <windows.h>
#include <gl/glew.h>

namespace gfx
{
    namespace raw
    {
        // ---------------- Modo imediato ----------------
        extern void (APIENTRY *fnBegin)(GLenum mode);
        extern void (APIENTRY *fnEnd)(void);
        extern void (APIENTRY *fnVertex2f)(GLfloat x, GLfloat y);
        extern void (APIENTRY *fnVertex3f)(GLfloat x, GLfloat y, GLfloat z);
        extern void (APIENTRY *fnVertex3fv)(const GLfloat* v);
        extern void (APIENTRY *fnTexCoord2f)(GLfloat s, GLfloat t);
        extern void (APIENTRY *fnTexCoord2fv)(const GLfloat* v);
        extern void (APIENTRY *fnColor3f)(GLfloat red, GLfloat green, GLfloat blue);
        extern void (APIENTRY *fnColor4f)(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
        extern void (APIENTRY *fnColor3fv)(const GLfloat* v);
        extern void (APIENTRY *fnColor4fv)(const GLfloat* v);
        extern void (APIENTRY *fnColor3ub)(GLubyte red, GLubyte green, GLubyte blue);
        extern void (APIENTRY *fnColor4ub)(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha);
        extern void (APIENTRY *fnNormal3f)(GLfloat nx, GLfloat ny, GLfloat nz);
        extern void (APIENTRY *fnNormal3fv)(const GLfloat* v);

        // ---------------- Client-state arrays ----------------
        extern void (APIENTRY *fnEnableClientState)(GLenum array);
        extern void (APIENTRY *fnDisableClientState)(GLenum array);
        extern void (APIENTRY *fnVertexPointer)(GLint size, GLenum type, GLsizei stride, const void* pointer);
        extern void (APIENTRY *fnTexCoordPointer)(GLint size, GLenum type, GLsizei stride, const void* pointer);
        extern void (APIENTRY *fnColorPointer)(GLint size, GLenum type, GLsizei stride, const void* pointer);
        extern void (APIENTRY *fnDrawArrays)(GLenum mode, GLint first, GLsizei count);

        // ---------------- Pilha de matrizes ----------------
        extern void (APIENTRY *fnMatrixMode)(GLenum mode);
        extern void (APIENTRY *fnPushMatrix)(void);
        extern void (APIENTRY *fnPopMatrix)(void);
        extern void (APIENTRY *fnLoadIdentity)(void);
        extern void (APIENTRY *fnLoadMatrixf)(const GLfloat* m);
        extern void (APIENTRY *fnMultMatrixf)(const GLfloat* m);
        extern void (APIENTRY *fnRotatef)(GLfloat angle, GLfloat x, GLfloat y, GLfloat z);
        extern void (APIENTRY *fnTranslatef)(GLfloat x, GLfloat y, GLfloat z);
        extern void (APIENTRY *fnScalef)(GLfloat x, GLfloat y, GLfloat z);

        // ---------------- Projecao (glu32) ----------------
        extern void (APIENTRY *fnPerspective)(GLdouble fovy, GLdouble aspect, GLdouble zNear, GLdouble zFar);
        extern void (APIENTRY *fnOrtho2D)(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top);

        // ---------------- Estado ----------------
        extern void (APIENTRY *fnEnable)(GLenum cap);
        extern void (APIENTRY *fnDisable)(GLenum cap);
        extern void (APIENTRY *fnAlphaFunc)(GLenum func, GLclampf ref);
        extern void (APIENTRY *fnFogf)(GLenum pname, GLfloat param);
        extern void (APIENTRY *fnFogfv)(GLenum pname, const GLfloat* params);
        extern void (APIENTRY *fnTexEnvi)(GLenum target, GLenum pname, GLint param);
        extern void (APIENTRY *fnTexEnvf)(GLenum target, GLenum pname, GLfloat param);
        extern void (APIENTRY *fnGetFloatv)(GLenum pname, GLfloat* params);
        extern void (APIENTRY *fnGetIntegerv)(GLenum pname, GLint* params);
        extern GLboolean (APIENTRY *fnIsEnabled)(GLenum cap);
        extern void (APIENTRY *fnReadPixels)(GLint x, GLint y, GLsizei width, GLsizei height,
                                             GLenum format, GLenum type, void* pixels);

        // ---------------- Estado cru (para o batching invalidar o lote) ----------------
        extern void (APIENTRY *fnBindTexture)(GLenum target, GLuint texture);
        extern void (APIENTRY *fnBlendFunc)(GLenum sfactor, GLenum dfactor);
        extern void (APIENTRY *fnDepthFunc)(GLenum func);
        extern void (APIENTRY *fnDepthMask)(GLboolean flag);
        extern void (APIENTRY *fnViewport)(GLint x, GLint y, GLsizei width, GLsizei height);
        extern void (APIENTRY *fnScissor)(GLint x, GLint y, GLsizei width, GLsizei height);
        extern void (APIENTRY *fnColorMask)(GLboolean r, GLboolean g, GLboolean b, GLboolean a);
        extern void (APIENTRY *fnFrontFace)(GLenum mode);
        extern void (APIENTRY *fnClear)(GLbitfield mask);
    }
}
