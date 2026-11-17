//======================================================================
// gfx/GfxGLM.h
//
// Include seguro do GLM.
//
// O projeto mantem as macros min/max definidas pelo windows.h (o codigo
// legado do jogo as usa, por exemplo em WINHANDLE.cpp: "min(ratioX, ratioY)").
//
// O GLM declara glm::min(genType x, genType y) e glm::max(...). Com as
// macros do windows.h ativas, essas declaracoes viram
//     (((genType x) < (genType y)) ? (genType x) : (genType y));
// e o GLM nao compila.
//
// Solucao: desativar as macros somente durante o include do GLM e
// restaura-las em seguida (MSVC /Yc nao e necessario aqui).
//
// Observacao: antes desta migracao o GLM nunca era realmente compilado
// (CShaderGL.h esta inteiro sob #ifdef SHADER_VERSION_TEST, desligado).
//======================================================================
#pragma once

#pragma push_macro("min")
#pragma push_macro("max")
#undef min
#undef max

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#pragma pop_macro("max")
#pragma pop_macro("min")
