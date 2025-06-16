#include "App.h"

App::App() {
    m_window = std::make_unique<womp::Window>(WIDTH, HEIGHT, "Womp Womp");

    m_renderer = std::make_unique<womp::Renderer>(*m_window);

    m_wompRenderer = std::make_unique<womp::WompRenderer>(m_renderer->getDevice(), *m_renderer);
}

App::~App() {
}

void App::run() {
    while (!m_window->shouldClose()) {
        m_window->pollEvents();

        //
        if (const auto commandBuffer = m_renderer->BeginFrame()) {
        const int frameIndex = m_renderer->getFrameIndex();
        //
        //     //Swapchain render pass
        m_renderer->beginSwapChainRenderPass(commandBuffer);
        m_wompRenderer->render(commandBuffer, frameIndex, m_renderer->getSwapchain());
        m_renderer->endSwapChainRenderPass(commandBuffer);
        //
        m_renderer->endFrame();
        }

        // const auto sleepTime = vov::DeltaTime::GetInstance().SleepDuration();
        // if (sleepTime > std::chrono::nanoseconds(0)) {
        //     std::this_thread::sleep_for(sleepTime);
        // }
    }
    vkDeviceWaitIdle(m_renderer->getVkDevice());
}
