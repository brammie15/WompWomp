#cmake_minimum_required(VERSION 3.31)
#project(Womp)

set(LIBRARY_NAME WompLib)

set(CMAKE_CXX_STANDARD 20)

SET(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(SHADER_EMBED_DIR "${CMAKE_CURRENT_BINARY_DIR}/WompLib/embedded_shaders")

include_directories(BEFORE ${CMAKE_SOURCE_DIR}/src)

set(SOURCE_FILES
        ${SRC_DIR}/Chalk.h

        ${SRC_DIR}/Core/Window.cpp
        ${SRC_DIR}/Core/WompMath.cpp

        ${SRC_DIR}/Rendering/Device.h ${SRC_DIR}/Rendering/Device.cpp
        ${SRC_DIR}/Rendering/Renderer.cpp
        ${SRC_DIR}/Rendering/Swapchain.h ${SRC_DIR}/Rendering/Swapchain.cpp
        ${SRC_DIR}/Rendering/Pipeline.h ${SRC_DIR}/Rendering/Pipeline.cpp
        ${SRC_DIR}/Rendering/WompRenderer.cpp

        ${SRC_DIR}/Rendering/DebugLabel.h ${SRC_DIR}/Rendering/DebugLabel.cpp

        ${SRC_DIR}/Descriptors/DescriptorPool.h ${SRC_DIR}/Descriptors/DescriptorPool.cpp
        ${SRC_DIR}/Descriptors/DescriptorSetLayout.h ${SRC_DIR}/Descriptors/DescriptorSetLayout.cpp
        ${SRC_DIR}/Descriptors/DescriptorWriter.h ${SRC_DIR}/Descriptors/DescriptorWriter.cpp


        ${SRC_DIR}/Rendering/Resources/Image.h ${SRC_DIR}/Rendering/Resources/Image.cpp
        ${SRC_DIR}/Rendering/Resources/ImageView.h ${SRC_DIR}/Rendering/Resources/ImageView.cpp
        ${SRC_DIR}/Rendering/Resources/Sampler.h ${SRC_DIR}/Rendering/Resources/Sampler.cpp
        ${SRC_DIR}/Rendering/Resources/Buffer.h ${SRC_DIR}/Rendering/Resources/Buffer.cpp

)

add_library(${LIBRARY_NAME} ${SOURCE_FILES})

target_include_directories(${LIBRARY_NAME}
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include/
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src ${SHADER_EMBED_DIR}
)
find_package(Vulkan REQUIRED)
target_link_libraries(${LIBRARY_NAME} PUBLIC Vulkan::Vulkan)

include(${CMAKE_SOURCE_DIR}/Cmake/LinkVkBootstrap.cmake)
LinkVkBootstrap(${LIBRARY_NAME} PUBLIC)

include(${CMAKE_SOURCE_DIR}/Cmake/LinkVMA.cmake)
LinkVma(${LIBRARY_NAME} PRIVATE)

include(${CMAKE_SOURCE_DIR}/Cmake/LinkGLFW.cmake)
LinkGLFW(${LIBRARY_NAME} PUBLIC)

include(${CMAKE_SOURCE_DIR}/Cmake/LinkGLM.cmake)
LinkGLM(${LIBRARY_NAME} PUBLIC)


# === Shader Compilation ===
find_program(GLSL_VALIDATOR glslangValidator REQUIRED)

set(SHADER_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/shaders")
set(SHADER_BIN_DIR "${CMAKE_CURRENT_BINARY_DIR}/compiled_shaders")
file(MAKE_DIRECTORY ${SHADER_BIN_DIR})

file(GLOB_RECURSE GLSL_FILES
        "${SHADER_SRC_DIR}/*.vert"
        "${SHADER_SRC_DIR}/*.frag"
        "${SHADER_SRC_DIR}/*.comp"
)

set(SPIRV_BINARY_FILES "")

foreach(GLSL ${GLSL_FILES})
    get_filename_component(FILE_NAME ${GLSL} NAME)
    set(SPV "${SHADER_BIN_DIR}/${FILE_NAME}.spv")

    add_custom_command(
            OUTPUT ${SPV}
            COMMAND ${GLSL_VALIDATOR} -V -g ${GLSL} -o ${SPV}
            DEPENDS ${GLSL}
            COMMENT "Compiling shader: ${FILE_NAME}"
    )

    list(APPEND SPIRV_BINARY_FILES ${SPV})
endforeach()

add_custom_target(CompileShaders DEPENDS ${SPIRV_BINARY_FILES})
add_dependencies(${LIBRARY_NAME} CompileShaders)

# === Shader Embedding (CMake-Native) ===
set(SHADER_EMBED_DIR "${CMAKE_CURRENT_BINARY_DIR}/embedded_shaders")
file(MAKE_DIRECTORY ${SHADER_EMBED_DIR})

set(EMBEDDED_SHADER_HEADERS "")

foreach(SPV ${SPIRV_BINARY_FILES})
    get_filename_component(FILE_NAME ${SPV} NAME) # e.g., basic.vert.spv
    string(REPLACE "." "_" SAFE_NAME "${FILE_NAME}") # basic_vert_spv
    set(VAR_NAME "${SAFE_NAME}")
    set(HEADER_FILE "${SHADER_EMBED_DIR}/${SAFE_NAME}.h")

    add_custom_command(
            OUTPUT ${HEADER_FILE}
            COMMAND ${CMAKE_COMMAND}
            -DINPUT=${SPV}
            -DOUTPUT=${HEADER_FILE}
            -DVAR_NAME=${VAR_NAME}
            -P "${CMAKE_SOURCE_DIR}/Cmake/EmbedShader.cmake"
            DEPENDS ${SPV}
            COMMENT "Embedding shader: ${FILE_NAME} → ${HEADER_FILE}"
    )

    list(APPEND EMBEDDED_SHADER_HEADERS ${HEADER_FILE})
endforeach()

add_custom_target(EmbedShaders DEPENDS ${EMBEDDED_SHADER_HEADERS})
add_dependencies(${LIBRARY_NAME} EmbedShaders)

target_include_directories(${LIBRARY_NAME} PRIVATE ${SHADER_EMBED_DIR})