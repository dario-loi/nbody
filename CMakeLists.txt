cmake_minimum_required(VERSION 3.11)
set(CMAKE_CXX_STANDARD 20)
project(sandbox)

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

# And optionally
SET(CMAKE_VERBOSE_MAKEFILE ON)

set(EXTERNAL_DIR "${PROJECT_SOURCE_DIR}/external")

set(NBODY_SOURCES
    "${PROJECT_SOURCE_DIR}/nbody.cpp"
)
set(STAPLEGL_DIR "${PROJECT_SOURCE_DIR}/include/staplegl")
set(STAPLEGL_MODULES_DIR "${STAPLEGL_DIR}/modules")

set(STAPLEGL_HEADERS
    ${STAPLEGL_DIR}/staplegl.hpp
    ${STAPLEGL_MODULES_DIR}/framebuffer.hpp
    ${STAPLEGL_MODULES_DIR}/gl_functions.hpp
    ${STAPLEGL_MODULES_DIR}/index_buffer.hpp
    ${STAPLEGL_MODULES_DIR}/renderbuffer.hpp
    ${STAPLEGL_MODULES_DIR}/shader_data_type.hpp
    ${STAPLEGL_MODULES_DIR}/shader.hpp
    ${STAPLEGL_MODULES_DIR}/texture.hpp
    ${STAPLEGL_MODULES_DIR}/uniform_buffer.hpp
    ${STAPLEGL_MODULES_DIR}/utility.hpp
    ${STAPLEGL_MODULES_DIR}/vertex_array.hpp
    ${STAPLEGL_MODULES_DIR}/vertex_buffer.hpp
    ${STAPLEGL_MODULES_DIR}/vertex_buffer_inst.hpp
    ${STAPLEGL_MODULES_DIR}/vertex_buffer_layout.hpp
    ${STAPLEGL_MODULES_DIR}/cubemap.hpp
)

set(GLAD_INCLUDE_DIR "${EXTERNAL_DIR}/glad/include/glad" "${EXTERNAL_DIR}/glad/include/KHR")
set(GLAD_SOURCE ${EXTERNAL_DIR}/glad/src/glad.c)
set(GLAD_HEADERS
    ${EXTERNAL_DIR}/glad/include/glad/glad.h
    ${EXTERNAL_DIR}/glad/include/KHR/khrplatform.h
)

add_library(glad ${GLAD_SOURCE} ${GLAD_HEADERS})

target_include_directories(glad PUBLIC ${GLAD_INCLUDE_DIR})

# now onto the libraries
find_package(OpenGL REQUIRED)

if(NOT OPENGL_FOUND)
    message(ERROR " OPENGL not found!")
else(NOT OPENGL_FOUND)
    message(STATUS " OPENGL found at: ${OPENGL_INCLUDE_DIR} ${OPENGL_LIBRARIES}")
endif(NOT OPENGL_FOUND)

set(LIBS glad ${OPENGL_LIBRARIES})

if(APPLE)
    find_library(COCOA_LIBRARY Cocoa REQUIRED)

    if(NOT COCOA_LIBRARY)
        message(ERROR " COCOA not found!")
    else(NOT COCOA_LIBRARY)
        message(STATUS " COCOA found at: ${COCOA_LIBRARY}")
    endif(NOT COCOA_LIBRARY)

    find_library(IOKIT_LIBRARY IOKit REQUIRED)

    if(NOT IOKIT_LIBRARY)
        message(ERROR " IOKIT not found!")
    else(NOT IOKIT_LIBRARY)
        message(STATUS " IOKIT found at: ${IOKIT_LIBRARY}")
    endif(NOT IOKIT_LIBRARY)

    find_library(COREVIDEO_LIBRARY CoreVideo REQUIRED)

    if(NOT COREVIDEO_LIBRARY)
        message(ERROR " COREVIDEO not found!")
    else(NOT COREVIDEO_LIBRARY)
        message(STATUS " COREVIDEO found at: ${COREVIDEO_LIBRARY}")
    endif(NOT COREVIDEO_LIBRARY)

    if(NOT SANDBOX_PACKAGES_NOT_FOUND)
        list(APPEND LIBS ${COCOA_LIBRARY} ${IOKIT_LIBRARY} ${COREVIDEO_LIBRARY})
    endif(NOT SANDBOX_PACKAGES_NOT_FOUND)
endif(APPLE)

if(UNIX AND NOT APPLE)
    find_package(X11 REQUIRED)

    if(NOT X11_FOUND)
        message(ERROR " X11 not found!")
    else(NOT X11_FOUND)
        message(STATUS " X11 found at: ${X11_INCLUDE_DIR} ${X11_LIBRARIES}")
    endif(NOT X11_FOUND)

    if(NOT CMAKE_DL_LIBS)
        message(ERROR " DL not found!")
    else(NOT CMAKE_DL_LIBS)
        message(STATUS " DL found at: ${CMAKE_DL_LIBS}")
    endif(NOT CMAKE_DL_LIBS)

    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads REQUIRED) # find out what flag is set if Threads is found.

    find_package(XRANDR REQUIRED)

    if(NOT XRANDR_LIBRARY)
        message(ERROR " XRANDR not found!")
    else(NOT XRANDR_LIBRARY)
        message(STATUS " XRANDR found at: ${XRANDR_LIBRARY}")
    endif(NOT XRANDR_LIBRARY)

    list(APPEND LIBS ${X11_LIBRARIES} ${XRANDR_LIBRARY} ${CMAKE_DL_LIBS} Threads::Threads)
endif(UNIX AND NOT APPLE)

# windows just needs the opengl32 library, which should have already been fetched.
find_package(GLFW3 REQUIRED)

if(NOT GLFW3_FOUND)
    message(ERROR " GLFW3 not found!")
else(NOT GLFW3_FOUND)
    message(STATUS " GLFW3 found at: ${GLFW3_INCLUDE_DIR} ${GLFW3_LIBRARY}")
    list(APPEND LIBS ${GLFW3_LIBRARY})
endif(NOT GLFW3_FOUND)

# find TBB for parallelism
find_package(TBB REQUIRED)

if(NOT TBB_FOUND)
    message(ERROR " TBB not found!")
else(NOT TBB_FOUND)
    message(STATUS " TBB found")
endif(NOT TBB_FOUND)

# Copy the glsl shaders to the build directory
# and add the file as a dependency to the target
set(SHADER_DIR "${PROJECT_SOURCE_DIR}/shaders")

set(EXAMPLES_SHADERS
    ${SHADER_DIR}/nbody_shader.glsl
    ${SHADER_DIR}/passthrough_shader.glsl
    ${SHADER_DIR}/smooth_texture.glsl
)

foreach(shader ${EXAMPLES_SHADERS})
    get_filename_component(shader_name ${shader} NAME)
    configure_file(${shader} ${CMAKE_CURRENT_BINARY_DIR}/shaders/${shader_name} COPYONLY)
endforeach(shader)

set(GLM_DIR "${EXTERNAL_DIR}/glm")
set(STB_DIR "${EXTERNAL_DIR}/stb")

add_executable(nbody ${NBODY_SOURCES} ${STAPLEGL_HEADERS} ${GLAD_HEADERS})
target_include_directories(nbody PUBLIC
    ${STAPLEGL_DIR}
    ${STAPLEGL_MODULES_DIR}
    ${GLAD_INCLUDE_DIR}
    ${GLM_DIR}
    ${STB_DIR}
)
target_link_libraries(nbody ${LIBS} TBB::tbb)
target_include_directories(nbody PUBLIC ${GLFW3_INCLUDE_DIR}
    ${OPENGL_INCLUDE_DIR}
)

# add warnings to executable targets
if(MSVC)
    target_compile_options(nbody PRIVATE /W4 /WX)
else()
    target_compile_options(nbody PRIVATE -Wall -Wextra -Wpedantic)
endif()

# enable OMP on UNIX systems
if(UNIX)
    target_compile_options(nbody PRIVATE -fopenmp)
    target_link_libraries(nbody -fopenmp)
    target_compile_options(nbody PRIVATE -march=native)
endif()

# only if in debug configuration
if(CMAKE_BUILD_TYPE MATCHES Debug)
    # define staplegl debug macro
    target_compile_definitions(nbody PRIVATE STAPLEGL_DEBUG)
endif()
