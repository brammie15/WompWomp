#include "App.h"

#include <bits/this_thread_sleep.h>

App::App() {
    m_window = std::make_unique<womp::Window>(WIDTH, HEIGHT, "Womp Womp");

    m_wompRenderer = std::make_unique<womp::WompRenderer>(*m_window);
}

App::~App() {
    m_wompRenderer.reset();
    m_window.reset();
}

void App::run() {

    m_textureHandle = m_wompRenderer->createTexture("resources/kobeSilly.png");

    int counter = 0;

    while (!m_window->shouldClose()) {
        m_window->pollEvents();

        counter++;

        m_wompRenderer->drawTexture(m_textureHandle, WP_Rect{0, 0, 100, 100}, WP_Rect{100, 100, 200, 200}, glm::vec4(1.0f));
        // m_wompRenderer->drawTexture(m_textureHandle, glm::vec2(200, sin(counter / 1000) * 100 + 200), glm::vec2(100, 100), glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));

        m_wompRenderer->render();
        // std::this_thread::sleep_for(std::chrono::milliseconds(16)); // Roughly 60 FPS
    }
    m_wompRenderer->waitIdle();
}
