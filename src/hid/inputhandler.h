#pragma once

#include "gsl-lite.hpp"
#include "inputstate.h"

#include <GLFW/glfw3.h>

namespace hid
{
class InputHandler final
{
public:
    explicit InputHandler(const gsl::not_null<GLFWwindow*>& window);

    void update();

    const InputState& getInputState() const
    {
        return m_inputState;
    }

private:
    InputState m_inputState{};

    const gsl::not_null<GLFWwindow*> m_window;
    double m_lastCursorX = 0;
    double m_lastCursorY = 0;

    int m_controllerIndex = -1;
};
} // namespace hid
