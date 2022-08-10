#include "pch.h"
#include "WindowsInput.h"
#include "proton/Application.h"

#include <GLFW/glfw3.h>

namespace proton {

    Input* Input::s_Instance = new WindowsInput();

    bool WindowsInput::isKeyPressed_Impl(int keyCode)
    {
        auto window = (GLFWwindow*)Application::get().getWindow().getNativeWindow();
        auto state = glfwGetKey(window, keyCode);
        return state == GLFW_PRESS || state == GLFW_REPEAT;
    }

    std::pair<float, float> WindowsInput::getMousePosition_Impl()
    {
        auto window = (GLFWwindow*)Application::get().getWindow().getNativeWindow();
        double x, y;
        glfwGetCursorPos(window, &x, &y);
        return { (float)x, (float)y };
    }
}

