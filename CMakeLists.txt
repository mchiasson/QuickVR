cmake_minimum_required(VERSION 3.0)

project(QuickVR LANGUAGES C CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Qt5 REQUIRED COMPONENTS Quick Core Qml Concurrent)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

add_library(quickvrplugin SHARED

    src/imports/${PROJECT_NAME}/qmldir

    src/BaseLight.cpp
    src/BaseLight.h
    src/DebugCallback.cpp
    src/DebugCallback.h
    src/Device.cpp
    src/Device.h
    src/DirectionalLight.cpp
    src/DirectionalLight.h
    src/Node.cpp
    src/Node.h
    src/QuickVR_plugin.cpp
    src/QuickVR_plugin.h
    src/PointLight.cpp
    src/PointLight.h
    src/Shader.cpp
    src/Shader.h
    src/ShaderProgram.cpp
    src/ShaderProgram.h
    src/SkinnedMesh.cpp
    src/SkinnedMesh.h
    src/SpotLight.cpp
    src/SpotLight.h
    src/Texture.cpp
    src/Texture.h
    src/Headset.cpp
    src/Headset.h
    #src/VRRenderer.cpp
    #src/VRRenderer.h
    #src/VRWindow.cpp
    #src/VRWindow.h
)

target_link_libraries(quickvrplugin PRIVATE
    Qt5::Core
    Qt5::Quick
    Qt5::Qml
    Qt5::Concurrent
    assimp
    stb
    glm
)

target_include_directories(quickvrplugin PRIVATE
    src
)

set_target_properties(quickvrplugin
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/imports/${PROJECT_NAME}"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/imports/${PROJECT_NAME}"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/imports/${PROJECT_NAME}"
)

add_custom_command(TARGET quickvrplugin POST_BUILD
    COMMAND
        ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/src/imports/${PROJECT_NAME}/qmldir
        ${CMAKE_BINARY_DIR}/imports/${PROJECT_NAME}/qmldir
)

if(DEFINED ENV{OVR_SDK_ROOT})

    file(TO_CMAKE_PATH "$ENV{OVR_SDK_ROOT}" OVR_SDK_ROOT)

    add_library(OVR STATIC
        ${OVR_SDK_ROOT}/LibOVR/Shim/OVR_CAPIShim.c
        ${OVR_SDK_ROOT}/LibOVR/Shim/OVR_CAPI_Util.cpp
        ${OVR_SDK_ROOT}/LibOVR/Shim/OVR_StereoProjection.cpp
    )

    target_include_directories(OVR PUBLIC
        ${OVR_SDK_ROOT}/LibOVR/Include
    )

    target_compile_definitions(OVR PUBLIC HAVE_LIBOVR)

    target_link_libraries(quickvrplugin PRIVATE OVR)

endif()


add_subdirectory(examples)
add_subdirectory(3rdparty)
