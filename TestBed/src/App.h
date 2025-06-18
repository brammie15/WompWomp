#ifndef APP_H
#define APP_H
#include <cstdint>
#include <memory>

#include <womp/Womp.h>


class App {
public:
    static constexpr uint32_t WIDTH = 1200;
    static constexpr uint32_t HEIGHT = 1000;

    App();
    ~App();

    App(const App& other) = delete;
    App(App&& other) noexcept = delete;
    App& operator=(const App& other) = delete;
    App& operator=(App&& other) noexcept = delete;

    void run();

private:
    std::unique_ptr<womp::Window> m_window{};
    std::unique_ptr<womp::WompRenderer> m_wompRenderer{};

    womp::TextureHandle m_textureHandle{};
};



#endif //APP_H
