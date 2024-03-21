#include "ptpch.h"
#include "Proton/Core/Input.h"
#include "Proton/Core/Application.h"

#include <GLFW/glfw3.h>

#ifdef PT_EDITOR
#include "Proton/Editor/EditorLayer.h"
#include "Proton/Core/GameInstance.h"
#include "Proton/Scripting/EntityScript.h"
#endif

namespace proton {

    bool Input::IsKeyPressed(int keyCode, EntityScript* script)
    {
        auto window = (GLFWwindow*)Application::Get().GetWindow().GetNativeWindow();
        auto state = glfwGetKey(window, keyCode);

    #ifdef PT_EDITOR
        GameInstance* focusedInstance = EditorLayer::Get()->GetFocusedGameInstance();
        if (script && focusedInstance && script->GetScene() != focusedInstance->GetActiveScene())
            return false;
    #endif

        return state == GLFW_PRESS || state == GLFW_REPEAT;
    }

    bool Input::IsMouseButtonPressed(const MouseCode button, EntityScript* script)
    {
        auto* window = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
        auto state = glfwGetMouseButton(window, static_cast<int32_t>(button));

    #ifdef PT_EDITOR
        GameInstance* focusedInstance = EditorLayer::Get()->GetFocusedGameInstance();
        if (script && focusedInstance && script->GetScene() != focusedInstance->GetActiveScene())
            return false;
    #endif

        return state == GLFW_PRESS;
    }

    glm::vec2 Input::GetMousePosition()
    {
        auto window = (GLFWwindow*)Application::Get().GetWindow().GetNativeWindow();
        double x, y;
        glfwGetCursorPos(window, &x, &y);
        return { (float)x, (float)y };
    }
}
