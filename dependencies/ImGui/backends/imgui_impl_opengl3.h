// dear imgui: Renderer Backend for modern OpenGL 3.3 core
//
// Adaptacao compacta do backend oficial do Dear ImGui
// (backends/imgui_impl_opengl3.cpp, upstream v1.90.9) para este projeto.
//
// Diferencas em relacao ao upstream:
//   - Usa o GLEW (ja linkado no projeto) em vez do loader gl3w proprio.
//     Isso evita adicionar o arquivo imgui_impl_opengl3_loader.h (~1000
//     linhas) e garante que o backend use exatamente as mesmas funcoes GL
//     que o resto do cliente.
//   - Suporta apenas desktop OpenGL 3.3 core (o minimo do modulo gfx/).
//     Nao ha suporte a OpenGL ES 2/3 nem a contexts de compatibilidade.
//
// O backend OpenGL 2 original (imgui_impl_opengl2.cpp) usa funcoes que
// NAO EXISTEM em core profile 3.3:
//     glVertexPointer / glTexCoordPointer / glColorPointer
//     glEnableClientState / glDisableClientState
//     glMatrixMode / glOrtho
// Em core profile essas chamadas provocam GL_INVALID_OPERATION e o
// desenho do ImGui (e a validação do vertex fetch do driver) quebra.
// Por isso o backend moderno do cliente usa este arquivo.
//
// LICENCA: MIT (igual ao Dear ImGui).

#pragma once

#include "imgui.h"

#if !defined(IMGUI_IMPL_OPENGL_LOADER_CUSTOM) && defined(IMGUI_IMPL_OPENGL_ES2)
#error "IMGUI_IMPL_OPENGL_ES2 is not supported by this backend."
#endif

IMGUI_IMPL_API bool     ImGui_ImplOpenGL3_Init(const char* glsl_version);
IMGUI_IMPL_API void     ImGui_ImplOpenGL3_Shutdown();
IMGUI_IMPL_API void     ImGui_ImplOpenGL3_NewFrame();
IMGUI_IMPL_API void     ImGui_ImplOpenGL3_RenderDrawData(ImDrawData* draw_data);
IMGUI_IMPL_API bool     ImGui_ImplOpenGL3_CreateFontsTexture();
IMGUI_IMPL_API void     ImGui_ImplOpenGL3_DestroyFontsTexture();
IMGUI_IMPL_API bool     ImGui_ImplOpenGL3_CreateDeviceObjects();
IMGUI_IMPL_API void     ImGui_ImplOpenGL3_DestroyDeviceObjects();
