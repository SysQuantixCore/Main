//======================================================================
// gfx/GfxImGui.cpp
//======================================================================
#include "stdafx.h"
#include "gfx/GfxImGui.h"
#include "gfx/GfxRenderer.h"

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_opengl2.h"
#include "imgui_impl_opengl3.h"

namespace gfx
{
    namespace
    {
        bool g_UseOpenGL3 = false;
    }

    void ImGuiBridge::Init()
    {
        // O contexto ja esta current neste ponto.
        g_UseOpenGL3 = Renderer::Instance().IsModern();

        if (g_UseOpenGL3)
        {
            ImGui_ImplOpenGL3_Init("#version 330 core");
        }
        else
        {
            ImGui_ImplOpenGL2_Init();
        }
    }

    void ImGuiBridge::Shutdown()
    {
        if (g_UseOpenGL3)
        {
            ImGui_ImplOpenGL3_Shutdown();
        }
        else
        {
            ImGui_ImplOpenGL2_Shutdown();
        }
    }

    void ImGuiBridge::NewFrame()
    {
        if (g_UseOpenGL3)
        {
            ImGui_ImplOpenGL3_NewFrame();
        }
        else
        {
            ImGui_ImplOpenGL2_NewFrame();
        }
    }

    void ImGuiBridge::RenderDrawData(ImDrawData* drawData)
    {
        // O ImGui troca programa, textura e estado GL (inclusive VAO proprio).
        // Desvincula o VAO do renderer antes e invalida o cache de programa.
        gfx::Renderer::Instance().FlushBatch();
        gfx::Renderer::Instance().GetBuffer().Unbind();
        if (g_UseOpenGL3)
        {
            ImGui_ImplOpenGL3_RenderDrawData(drawData);
            gfx::Renderer::Instance().GetShader().InvalidateProgramCache();
        }
        else
        {
            ImGui_ImplOpenGL2_RenderDrawData(drawData);
            gfx::Renderer::Instance().GetShader().InvalidateProgramCache();
        }
    }
}
