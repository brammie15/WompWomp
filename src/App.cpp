#include "App.h"

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

    while (!m_window->shouldClose()) {
        m_window->pollEvents();

        m_wompRenderer->drawTexture(m_textureHandle, glm::vec2(100, 100), glm::vec2(50, 50), 0);

        m_wompRenderer->render();
    }
    m_wompRenderer->waitIdle();
}
