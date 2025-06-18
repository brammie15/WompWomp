#include <womp/Window.h>
#include <stdexcept>

namespace womp {
    Window::Window(int w, int h, const std::string& title): width(w), height(h) {
        if (!glfwInit()) {
            throw std::runtime_error("Failed to initialize GLFW");
        }

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

        window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
        if (!window) {
            throw std::runtime_error("Failed to create GLFW window");
        }

        glfwSetWindowUserPointer(window, this);
        glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
    }

    Window::~Window() {
        if (window)
            glfwDestroyWindow(window);
        glfwTerminate();
    }

    void Window::framebufferResizeCallback(GLFWwindow* win, int width, int height) {
        const auto self = static_cast<Window*>(glfwGetWindowUserPointer(win));
        self->framebufferResized = true;
        self->width = width;
        self->height = height;
    }

    bool Window::shouldClose() const {
        return glfwWindowShouldClose(window);
    }

    void Window::pollEvents() {
        glfwPollEvents();
    }

    VkSurfaceKHR Window::createVulkanSurface(VkInstance instance) const {
        VkSurfaceKHR surface;
        if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create Vulkan surface");
        }
        return surface;
    }
}
