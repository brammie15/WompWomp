#ifndef WINDOW_H
#define WINDOW_H

#include <string>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace womp {
    class Window {
    public:
        Window(int width, int height, const std::string& title);
        ~Window();

        Window(const Window&) = delete;
        Window& operator=(const Window&) = delete;

        [[nodiscard]] GLFWwindow* getGLFWwindow() const { return window; }
        [[nodiscard]] int getWidth() const { return width; }
        [[nodiscard]] int getHeight() const { return height; }
        [[nodiscard]] bool shouldClose() const;

        VkSurfaceKHR createVulkanSurface(VkInstance instance) const;

        [[nodiscard]] bool wasResized() const { return framebufferResized; }
        void resetResizeFlag() { framebufferResized = false; }

        void pollEvents();

    private:
        static void framebufferResizeCallback(GLFWwindow* window, int width, int height);

        GLFWwindow* window = nullptr;
        int width = 0;
        int height = 0;
        bool framebufferResized = false;
    };
}


#endif //WINDOW_H
