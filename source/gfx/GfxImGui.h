//======================================================================
// gfx/GfxImGui.h
//
// Ponte entre o ImGui e os dois backends de render.
//
// O ImGui possui dois backends no projeto:
//   - imgui_impl_opengl2 : usa funcoes do fixed-function (glMatrixMode,
//     glOrtho, glVertexPointer...). Funciona apenas em contexto de
//     COMPATIBILIDADE (o backend legado).
//   - imgui_impl_opengl3 : usa VAO/VBO + GLSL 330 core. Funciona apenas
//     em core profile (o backend moderno).
//
// A escolha e feita em tempo de execucao, no mesmo momento em que o
// contexto OpenGL e criado.
//======================================================================
#pragma once

struct ImDrawData;

namespace gfx
{
    class ImGuiBridge
    {
    public:
        // Inicializa o backend do ImGui de acordo com o modo de render.
        // Deve ser chamado DEPOIS da criacao do contexto OpenGL.
        static void Init();

        static void Shutdown();

        static void NewFrame();

        static void RenderDrawData(ImDrawData* drawData);
    };
}
