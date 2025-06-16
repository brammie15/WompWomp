cmake_minimum_required(VERSION 3.31)
project(Womp)

set(CMAKE_CXX_STANDARD 20)

SET(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

include_directories(BEFORE ${CMAKE_SOURCE_DIR}/src)

set(SOURCE_FILES
        ${SRC_DIR}/main.cpp
        ${SRC_DIR}/App.h ${SRC_DIR}/App.cpp
        ${SRC_DIR}/Chalk.h

        ${SRC_DIR}/Core/Window.h ${SRC_DIR}/Core/Window.cpp

        ${SRC_DIR}/Rendering/Device.h ${SRC_DIR}/Rendering/Device.cpp
        ${SRC_DIR}/Rendering/Renderer.h ${SRC_DIR}/Rendering/Renderer.cpp
        ${SRC_DIR}/Rendering/Swapchain.h ${SRC_DIR}/Rendering/Swapchain.cpp
        ${SRC_DIR}/Rendering/Pipeline.h ${SRC_DIR}/Rendering/Pipeline.cpp
        ${SRC_DIR}/Rendering/WompRenderer.h ${SRC_DIR}/Rendering/WompRenderer.cpp
        ${SRC_DIR}/Rendering/Buffer.h ${SRC_DIR}/Rendering/Buffer.cpp

        ${SRC_DIR}/Rendering/DebugLabel.h ${SRC_DIR}/Rendering/DebugLabel.cpp

        ${SRC_DIR}/Descriptors/DescriptorPool.h ${SRC_DIR}/Descriptors/DescriptorPool.cpp
        ${SRC_DIR}/Descriptors/DescriptorSetLayout.h ${SRC_DIR}/Descriptors/DescriptorSetLayout.cpp
        ${SRC_DIR}/Descriptors/DescriptorWriter.h ${SRC_DIR}/Descriptors/DescriptorWriter.cpp


        ${SRC_DIR}/Rendering/Resources/Image.h ${SRC_DIR}/Rendering/Resources/Image.cpp
        ${SRC_DIR}/Rendering/Resources/ImageView.h ${SRC_DIR}/Rendering/Resources/ImageView.cpp
        ${SRC_DIR}/Rendering/Resources/Sampler.h ${SRC_DIR}/Rendering/Resources/Sampler.cpp

)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

find_package(Vulkan REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE Vulkan::Vulkan)

include(Cmake/LinkVkBootstrap.cmake)
LinkVkBootstrap(${PROJECT_NAME} PRIVATE)

include(Cmake/LinkVMA.cmake)
LinkVma(${PROJECT_NAME} PRIVATE)

include(Cmake/LinkGLFW.cmake)
LinkGLFW(${PROJECT_NAME} PRIVATE)

include(Cmake/LinkGLM.cmake)
LinkGLM(${PROJECT_NAME} PRIVATE)


find_program(GLSL_VALIDATOR glslangValidator HINTS /usr/bin /usr/local/bin $ENV{VULKAN_SDK}/Bin/ $ENV{VULKAN_SDK}/Bin32/)

# Define shader source and destination directories
set(SHADER_SRC_DIR "${PROJECT_SOURCE_DIR}/shaders")
set(COMPILED_SHADER_DIR "${PROJECT_SOURCE_DIR}/compiled_shaders")
set(OUTPUT_SHADER_DIR "${CMAKE_BINARY_DIR}/shaders")

# Ensure the compiled shader directory exists
file(MAKE_DIRECTORY ${COMPILED_SHADER_DIR})

file(GLOB_RECURSE GLSL_SOURCE_FILES
        "${SHADER_SRC_DIR}/*.frag"
        "${SHADER_SRC_DIR}/*.vert"
        "${SHADER_SRC_DIR}/*.comp"
)

set(SPIRV_BINARY_FILES "")

foreach (GLSL ${GLSL_SOURCE_FILES})
    get_filename_component(FILE_NAME ${GLSL} NAME)
    set(SPIRV "${COMPILED_SHADER_DIR}/${FILE_NAME}.spv")

    message(STATUS "Compiling shader: ${GLSL} -> ${SPIRV}")

    add_custom_command(
            OUTPUT ${SPIRV}
            COMMAND ${GLSL_VALIDATOR} -V -g ${GLSL} -o ${SPIRV}
            DEPENDS ${GLSL}
    )

    list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach ()

add_custom_target(CompileShaders DEPENDS ${SPIRV_BINARY_FILES})

file(MAKE_DIRECTORY ${OUTPUT_SHADER_DIR})

foreach (SPV ${SPIRV_BINARY_FILES})
    get_filename_component(FILE_NAME ${SPV} NAME)
    add_custom_command(
            TARGET CompileShaders POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${SPV} ${OUTPUT_SHADER_DIR}/${FILE_NAME}
            COMMENT "Copying ${FILE_NAME} to output shaders directory"
    )
endforeach ()

add_dependencies(${PROJECT_NAME} CompileShaders)

set(RESOURCE_SRC_DIR "${PROJECT_SOURCE_DIR}/resources")
set(RESOURCE_DST_DIR "${CMAKE_BINARY_DIR}/resources")

add_custom_target(CopyResources ALL
        COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different  ${RESOURCE_SRC_DIR} ${RESOURCE_DST_DIR}
        COMMENT "Copying resources folder to build directory"
)
add_dependencies(${PROJECT_NAME} CopyResources)

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})

