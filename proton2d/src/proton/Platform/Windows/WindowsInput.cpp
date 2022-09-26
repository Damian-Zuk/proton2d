#include "pch.h"
#include "proton/Platform/Windows/WindowsInput.h"
#include "proton/Core/Application.h"

#include <GLFW/glfw3.h>

namespace proton {

    bool WindowsInput::IsKeyPressed_Implementation(int keyCode)
    {
        auto window = (GLFWwindow*)Application::Get().GetWindow().GetNativeWindow();
        auto state = glfwGetKey(window, keyCode);
        return state == GLFW_PRESS || state == GLFW_REPEAT;
    }

    std::pair<float, float> WindowsInput::GetMousePosition_Implementation()
    {
        auto window = (GLFWwindow*)Application::Get().GetWindow().GetNativeWindow();
        double x, y;
        glfwGetCursorPos(window, &x, &y);
        return { (float)x, (float)y };
    }
}

