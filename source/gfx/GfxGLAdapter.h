//======================================================================
// gfx/GfxGLAdapter.h
//
// Ponte de compatibilidade entre o motor legado (que chama glBegin, glEnd,
// glVertex3fv, glMatrixMode, ...) e o modulo gfx.
//
// COMO FUNCIONA
// -------------
// As macros deste header substituem os nomes das funcoes OpenGL que
// NAO EXISTEM no core profile 3.3 por despachantes em gfx::GL.
//
// Em tempo de execucao cada despachante decide:
//   - Backend legado  : chama a funcao original da OpenGL (gfx::raw::*),
//                        preservando bit-a-bit o comportamento atual.
//   - Backend moderno : encaminha para gfx::Renderer, que implementa o
//                        desenho com VBO/VAO + shader GLSL.
//
// POR QUE OS DESPACHANTES NO ESCOPO GLOBAL
// ----------------------------------------
// O codigo legado usa chamadas com qualificador global (ex.: "::glBegin").
// Uma macro que expandisse para "gfx::GL::Begin" geraria "::gfx::GL::Begin",
// que e invalido. Por isso os despachantes sao declarados no escopo global
// com o prefixo "gfxGL", e as macros expandem para esses nomes.
//
// FUNCOES INTERCEPTADAS (as que sumiram no core profile 3.3)
//   - Modo imediato ....... glBegin/glEnd + glVertex*/glTexCoord*/glColor*
//   - Client-state arrays . glEnableClientState/glVertexPointer/glDrawArrays
//   - Pilha de matrizes .... glMatrixMode/glPushMatrix/glRotatef/...
//   - Projecao ............. gluPerspective/gluOrtho2D
//   - Estado removido ...... GL_ALPHA_TEST, GL_FOG, glAlphaFunc, glFogf(v)
//   - Consultas ............ glGetFloatv (GL_CURRENT_COLOR/MODELVIEW_MATRIX)
//   - Leitura de pixels .... glReadPixels (GL_DEPTH_COMPONENT e GL_RGB)
//
// AS FUNCOES GL 1.1 QUE AINDA EXISTEM NO CORE PROFILE NAO SAO INTERCEPTADAS
// (glBindTexture, glBlendFunc, glDepthMask, glTexImage2D, glViewport,
// glClear, glScissor, glStencilOp, glPolygonMode, ...). O motor legado as
// chama diretamente e elas funcionam igual nos dois backends.
//
// IMPORTANTE: este header DEVE ser incluido depois de <gl/glew.h> e
// <gl/glu.h>. Ele e incluido pelo StdAfx.h, que ja garante essa ordem.
//======================================================================
#pragma once

#include "gfx/GfxTypes.h"
#include "gfx/GfxRawGL.h"

// ---------------------------------------------------------------------
// Despachantes (escopo global, prefixo gfxGL)
// ---------------------------------------------------------------------
void gfxGLBegin(GLenum mode);
void gfxGLEnd();
void gfxGLVertex2f(GLfloat x, GLfloat y);
void gfxGLVertex3f(GLfloat x, GLfloat y, GLfloat z);
void gfxGLVertex3fv(const GLfloat* v);
void gfxGLTexCoord2f(GLfloat s, GLfloat t);
void gfxGLTexCoord2fv(const GLfloat* v);
void gfxGLColor3f(GLfloat red, GLfloat green, GLfloat blue);
void gfxGLColor4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
void gfxGLColor3fv(const GLfloat* v);
void gfxGLColor4fv(const GLfloat* v);
void gfxGLColor3ub(GLubyte red, GLubyte green, GLubyte blue);
void gfxGLColor4ub(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha);
void gfxGLNormal3f(GLfloat nx, GLfloat ny, GLfloat nz);
void gfxGLNormal3fv(const GLfloat* v);

void gfxGLEnableClientState(GLenum array);
void gfxGLDisableClientState(GLenum array);
void gfxGLVertexPointer(GLint size, GLenum type, GLsizei stride, const void* pointer);
void gfxGLTexCoordPointer(GLint size, GLenum type, GLsizei stride, const void* pointer);
void gfxGLColorPointer(GLint size, GLenum type, GLsizei stride, const void* pointer);
void gfxGLDrawArrays(GLenum mode, GLint first, GLsizei count);

void gfxGLMatrixMode(GLenum mode);
void gfxGLPushMatrix();
void gfxGLPopMatrix();
void gfxGLLoadIdentity();
void gfxGLLoadMatrixf(const GLfloat* m);
void gfxGLMultMatrixf(const GLfloat* m);
void gfxGLRotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z);
void gfxGLTranslatef(GLfloat x, GLfloat y, GLfloat z);
void gfxGLScalef(GLfloat x, GLfloat y, GLfloat z);

void gfxGLPerspective(GLdouble fovy, GLdouble aspect, GLdouble zNear, GLdouble zFar);
void gfxGLOrtho2D(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top);

void gfxGLEnable(GLenum cap);
void gfxGLDisable(GLenum cap);
void gfxGLAlphaFunc(GLenum func, GLclampf ref);
void gfxGLFogf(GLenum pname, GLfloat param);
void gfxGLFogfv(GLenum pname, const GLfloat* params);
void gfxGLTexEnvi(GLenum target, GLenum pname, GLint param);
void gfxGLTexEnvf(GLenum target, GLenum pname, GLfloat param);
void gfxGLGetFloatv(GLenum pname, GLfloat* params);
void gfxGLReadPixels(GLint x, GLint y, GLsizei width, GLsizei height,
                      GLenum format, GLenum type, void* pixels);

// ---------------------------------------------------------------------
// Macros de redirecionamento
// ---------------------------------------------------------------------
#define glBegin              gfxGLBegin
#define glEnd                gfxGLEnd
#define glVertex2f           gfxGLVertex2f
#define glVertex3f           gfxGLVertex3f
#define glVertex3fv          gfxGLVertex3fv
#define glTexCoord2f         gfxGLTexCoord2f
#define glTexCoord2fv        gfxGLTexCoord2fv
#define glColor3f            gfxGLColor3f
#define glColor4f            gfxGLColor4f
#define glColor3fv           gfxGLColor3fv
#define glColor4fv           gfxGLColor4fv
#define glColor3ub           gfxGLColor3ub
#define glColor4ub           gfxGLColor4ub
#define glNormal3f           gfxGLNormal3f
#define glNormal3fv          gfxGLNormal3fv

#define glEnableClientState  gfxGLEnableClientState
#define glDisableClientState gfxGLDisableClientState
#define glVertexPointer      gfxGLVertexPointer
#define glTexCoordPointer    gfxGLTexCoordPointer
#define glColorPointer       gfxGLColorPointer
#define glDrawArrays         gfxGLDrawArrays

#define glMatrixMode         gfxGLMatrixMode
#define glPushMatrix         gfxGLPushMatrix
#define glPopMatrix          gfxGLPopMatrix
#define glLoadIdentity       gfxGLLoadIdentity
#define glLoadMatrixf        gfxGLLoadMatrixf
#define glMultMatrixf        gfxGLMultMatrixf
#define glRotatef            gfxGLRotatef
#define glTranslatef         gfxGLTranslatef
#define glScalef             gfxGLScalef

#define gluPerspective       gfxGLPerspective
#define gluOrtho2D           gfxGLOrtho2D

#define glEnable             gfxGLEnable
#define glDisable            gfxGLDisable
#define glAlphaFunc          gfxGLAlphaFunc
#define glFogf               gfxGLFogf
#define glFogfv              gfxGLFogfv
#define glTexEnvi            gfxGLTexEnvi
#define glTexEnvf            gfxGLTexEnvf
#define glGetFloatv          gfxGLGetFloatv
#define glReadPixels         gfxGLReadPixels
