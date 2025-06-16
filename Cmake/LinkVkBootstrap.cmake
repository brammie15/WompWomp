include(FetchContent)

macro(LinkVkBootstrap TARGET ACCESS)
    FetchContent_Declare(
            vkBootstrap
            GIT_REPOSITORY https://github.com/charles-lunarg/vk-bootstrap
            GIT_TAG v1.4.315
    )

    FetchContent_MakeAvailable(vkBootstrap)

    target_link_libraries(${TARGET} ${ACCESS} vk-bootstrap::vk-bootstrap)
endmacro()