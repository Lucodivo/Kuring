cmake_minimum_required (VERSION 3.5)
project (Kuring)

# We need a CMAKE_DIR with some code to find external dependencies
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

set(LIB_DIR "C:/developer/dependencies/libs")
set(INCL_DIR "C:/developer/dependencies/include")
set(EXT_DIR "${CMAKE_SOURCE_DIR}/external")

# Add .lib files
link_directories(
        ${LIB_DIR}
)

# Define the include DIRs
include_directories(
        ${INCL_DIR}
        ${EXT_DIR}
        ${EXT_DIR}/glm
)

# project source files
file(GLOB_RECURSE SOURCE_FILES
        ${CMAKE_SOURCE_DIR}/src/*.c
        ${CMAKE_SOURCE_DIR}/src/*.cpp)
# project header files
file(GLOB_RECURSE HEADER_FILES
        ${CMAKE_SOURCE_DIR}/src/*.h
        ${CMAKE_SOURCE_DIR}/src/*.hpp)

# Dear IMGUI source files
file(GLOB IMGUI_SOURCE_FILES
        ${INCL_DIR}/imgui/*.cpp)
# Dear IMGUI header files
file(GLOB IMGUI_HEADER_FILES
        ${INCL_DIR}/imgui/*.h)

if(MSVC)
    add_compile_options(/W3)
else ()
    add_compile_options(-Wall -Wall -Wpedantic)
endif()

# Define the executable
add_executable(${PROJECT_NAME} ${HEADER_FILES} ${SOURCE_FILES} ${IMGUI_HEADER_FILES} ${IMGUI_SOURCE_FILES})

# GLAD
add_library(glad "${EXT_DIR}/glad/glad.c")

# Put all libraries into a variable
set(LIBS assimp freetype glad glfw3-x64-d vulkan-1)

# Define the link libraries
target_link_libraries(${PROJECT_NAME} ${LIBS})

function(add_shader TARGET SHADER_PATH)
    find_program(GLSLC glslc)

    set(current-shader-path ${SHADER_PATH})
    get_filename_component(shader-file-name ${SHADER_PATH} NAME)
    set(current-output-path ${CMAKE_BINARY_DIR}/shaders/${shader-file-name}.spv)

    # Add a custom command to compile GLSL to SPIR-V.
    get_filename_component(current-output-dir ${current-output-path} DIRECTORY)
    file(MAKE_DIRECTORY ${current-output-dir})

    add_custom_command(
            OUTPUT ${current-output-path}
            COMMAND ${GLSLC} -o ${current-output-path} ${current-shader-path}
            DEPENDS ${current-shader-path}
            IMPLICIT_DEPENDS CXX ${current-shader-path}
            VERBATIM)

    # Make sure our build depends on this output.
    set_source_files_properties(${current-output-path} PROPERTIES GENERATED TRUE)
    target_sources(${TARGET} PRIVATE ${current-output-path})
endfunction(add_shader)

function(compile_all_shaders)
    file(GLOB_RECURSE VertexShaderFilePaths "${CMAKE_CURRENT_SOURCE_DIR}/src/*.vert")
    foreach(shaderFilePath ${VertexShaderFilePaths})
        add_shader(${PROJECT_NAME} ${shaderFilePath})
    endforeach()
    file(GLOB_RECURSE FragmentShaderFilePaths "${CMAKE_CURRENT_SOURCE_DIR}/src/*.frag")
    foreach(shaderFilePath ${FragmentShaderFilePaths})
        add_shader(${PROJECT_NAME} ${shaderFilePath})
    endforeach()
endfunction(compile_all_shaders)

compile_all_shaders()