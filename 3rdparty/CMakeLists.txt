
option(ASSIMP_BUILD_ASSIMP_TOOLS "" OFF)
option(ASSIMP_BUILD_TESTS "" OFF)
option(BUILD_SHARED_LIBS "" OFF)
add_subdirectory(assimp)

add_custom_target(CopyModels ALL
    COMMAND
        ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/assimp/test/models
        ${PROJECT_BINARY_DIR}/models
    COMMAND
        ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/assimp/test/models-nonbsd
        ${PROJECT_BINARY_DIR}/models-nonbsd
)

add_library(stb INTERFACE)
target_include_directories(stb INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/stb)

add_library(glm INTERFACE)
target_include_directories(glm INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/glm)

add_library(StructureOfArrays INTERFACE)
target_include_directories(StructureOfArrays INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/StructureOfArrays/include)
