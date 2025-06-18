
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

set(SOURCE_FILES
    ${SRC_DIR}/main.cpp
    ${SRC_DIR}/App.h  ${SRC_DIR}/App.cpp
)

add_executable(TestBed ${SOURCE_FILES})

target_link_libraries(TestBed PRIVATE WompLib)

target_include_directories(TestBed PRIVATE ${CMAKE_SOURCE_DIR}/WompLib/src)

# Resource Copying
set(RESOURCE_SRC_DIR "${CMAKE_SOURCE_DIR}/resources")
set(RESOURCE_DST_DIR "${CMAKE_BINARY_DIR}/resources")

add_custom_target(CopyResources ALL
        COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different  ${RESOURCE_SRC_DIR} ${RESOURCE_DST_DIR}
        COMMENT "Copying resources folder to build directory"
)
add_dependencies(TestBed CopyResources)

# Set startup project in VS
set_property(DIRECTORY ${CMAKE_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT TestBed)