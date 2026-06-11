# Minimum required version of CMake
cmake_minimum_required(VERSION 3.22)
# Project name and version
project(OpenChaosDefender VERSION 1.0)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

##########################################################################################
# Configurable options
##########################################################################################

# toggle libs to be used with ON and OFF
option(USE_RAYLIB "Use raylib" ON)
option(USE_NLOHMANN_JSON "Use nlohmann json" ON)
option(USE_TOMLPLUSPLUS "Use toml++" ON)
option(USE_HARFBUZZ "Use harfbuzz (GPU text rendering)" ON)


##########################################################################################
# Add dependencies with FetchContent
##########################################################################################

set(LINK_LIBRARIES "")  # libraries to link against
set(PROJECT_INCLUDE "")  # additional include directories

# Include FetchContent
include(FetchContent)

##########################################################################################
# Raylib Setup
##########################################################################################

if(USE_RAYLIB)
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE) # don't build the supplied examples
    set(BUILD_GAMES OFF CACHE BOOL "" FORCE) # don't build the supplied example games
    set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
    set(CUSTOMIZE_BUILD OFF CACHE BOOL "" FORCE)

    message(STATUS "Fetching raylib...")
    fetchcontent_declare(raylib
        GIT_REPOSITORY https://github.com/raysan5/raylib.git
        GIT_TAG        6.0
        GIT_SHALLOW    TRUE
        GIT_PROGRESS   TRUE
    )

    fetchcontent_makeavailable(raylib)

    list(APPEND LINK_LIBRARIES raylib)
    list(APPEND PROJECT_INCLUDE ${raylib_SOURCE_DIR}/src)  # ensure raylib.h is always found
endif()


##########################################################################################
# Nlohmann json Setup
##########################################################################################

if(USE_NLOHMANN_JSON)
    message(STATUS "Fetching nlohmann json...")
    fetchcontent_declare(nlohmann_json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG        v3.11.3
        GIT_SHALLOW    TRUE
        GIT_PROGRESS   TRUE
    )
    fetchcontent_makeavailable(nlohmann_json)

    # add the dependency here to be linked
    list(APPEND LINK_LIBRARIES nlohmann_json::nlohmann_json)
endif()


##########################################################################################
# toml++ Setup
##########################################################################################

if(USE_TOMLPLUSPLUS)
    message(STATUS "Fetching toml++...")
    fetchcontent_declare(tomlplusplus
        GIT_REPOSITORY https://github.com/marzer/tomlplusplus.git
        GIT_TAG        v3.4.0
        GIT_SHALLOW    TRUE
        GIT_PROGRESS   TRUE
    )
    fetchcontent_makeavailable(tomlplusplus)

    # add the dependency here to be linked
    list(APPEND LINK_LIBRARIES tomlplusplus::tomlplusplus)
endif()


##########################################################################################
# HarfBuzz Setup (text shaping + GPU Slug rendering via harfbuzz-gpu)
##########################################################################################

if(USE_HARFBUZZ)
    # Only the core shaper and the GPU (Slug) encoder are needed.
    set(HB_BUILD_SUBSET OFF CACHE BOOL "" FORCE)
    set(HB_BUILD_RASTER OFF CACHE BOOL "" FORCE)
    set(HB_BUILD_VECTOR OFF CACHE BOOL "" FORCE)
    set(HB_BUILD_GPU ON CACHE BOOL "" FORCE)
    set(HB_BUILD_UTILS OFF CACHE BOOL "" FORCE)
    set(HB_HAVE_FREETYPE OFF CACHE BOOL "" FORCE)

    message(STATUS "Fetching harfbuzz...")
    fetchcontent_declare(harfbuzz
        URL https://github.com/harfbuzz/harfbuzz/releases/download/14.2.1/harfbuzz-14.2.1.tar.xz
        URL_HASH SHA256=a54a5d8e9380a41fbb762ce367bcbf7704792dfca0d93f1bbca86c5a57902e0e
    )
    fetchcontent_makeavailable(harfbuzz)

    list(APPEND LINK_LIBRARIES harfbuzz harfbuzz-gpu)
    list(APPEND PROJECT_INCLUDE ${harfbuzz_SOURCE_DIR}/src)
endif()


##########################################################################################
# Project executable setup
##########################################################################################

# Specify the output directories for compiled binaries
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Adding our source files
# Define PROJECT_SOURCES as a list of all source files
file(GLOB_RECURSE PROJECT_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/src/*.cpp")

# Fonts live in resources/fonts/ — the single canonical location. That same folder
# is fed to two mechanisms so every platform gets the real typeface:
#   * the embed header-gen below bakes the TTFs into the binary (all platforms), and
#   * the web build's --preload-file resources@resources copies them into the wasm VFS
#     (see the PLATFORM Web block), where the raylib fallback loads them by path.
# Victor Mono for code/numbers, EB Garamond for prose.
set(FONTS_DIR ${CMAKE_CURRENT_LIST_DIR}/resources/fonts)

set(EMBED_VICTOR_SRC ${CMAKE_BINARY_DIR}/generated/victor_mono_ttf.cpp)
add_custom_command(
    OUTPUT ${EMBED_VICTOR_SRC}
    COMMAND ${CMAKE_COMMAND}
        -DINPUT=${FONTS_DIR}/VictorMono.ttf
        -DOUTPUT=${EMBED_VICTOR_SRC}
        -DSYMBOL=gVictorMonoTtf
        -P ${CMAKE_CURRENT_LIST_DIR}/cmake/embed_resource.cmake
    DEPENDS ${FONTS_DIR}/VictorMono.ttf
            ${CMAKE_CURRENT_LIST_DIR}/cmake/embed_resource.cmake
    COMMENT "Embedding VictorMono.ttf"
)
list(APPEND PROJECT_SOURCES ${EMBED_VICTOR_SRC})

set(EMBED_GARAMOND_SRC ${CMAKE_BINARY_DIR}/generated/eb_garamond_ttf.cpp)
add_custom_command(
    OUTPUT ${EMBED_GARAMOND_SRC}
    COMMAND ${CMAKE_COMMAND}
        -DINPUT=${FONTS_DIR}/EBGaramond.ttf
        -DOUTPUT=${EMBED_GARAMOND_SRC}
        -DSYMBOL=gEBGaramondTtf
        -P ${CMAKE_CURRENT_LIST_DIR}/cmake/embed_resource.cmake
    DEPENDS ${FONTS_DIR}/EBGaramond.ttf
            ${CMAKE_CURRENT_LIST_DIR}/cmake/embed_resource.cmake
    COMMENT "Embedding EBGaramond.ttf"
)
list(APPEND PROJECT_SOURCES ${EMBED_GARAMOND_SRC})

# Adding include paths
list(APPEND PROJECT_INCLUDE ${CMAKE_CURRENT_LIST_DIR}/src/)

# Declaring our executable
add_executable(${PROJECT_NAME})
target_sources(${PROJECT_NAME} PRIVATE ${PROJECT_SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE ${PROJECT_INCLUDE})


##########################################################################################
# Project build settings
##########################################################################################

# Build for web instructions
if (${PLATFORM} STREQUAL "Web")
    message(STATUS "Building for Web")
    # Optimize for size on web
    target_compile_options(${PROJECT_NAME} PRIVATE -Os)
 
    # Tell our code it's a web build so Game.cpp can use emscripten_set_main_loop
    target_compile_definitions(${PROJECT_NAME} PRIVATE PLATFORM_WEB)
    
    # Preload the global resources folder (engine/menu assets) and every datapack
    # (gameplay data + assets) into the .wasm VFS so they are reachable at runtime.
    set_target_properties(${PROJECT_NAME} PROPERTIES
    SUFFIX ".html"
    LINK_FLAGS "\
        --shell-file ${CMAKE_CURRENT_LIST_DIR}/tools/minshell.html \
        --preload-file ${CMAKE_CURRENT_LIST_DIR}/resources@resources \
        --preload-file ${CMAKE_CURRENT_LIST_DIR}/datapacks@datapacks \
        --preload-file ${CMAKE_CURRENT_LIST_DIR}/config@config \
        -s USE_GLFW=3 \
        -s ASSERTIONS=1 \
        -s WASM=1 \
        -s GL_ENABLE_GET_PROC_ADDRESS=1 \
        -s ASYNCIFY \
        -s EXPORTED_RUNTIME_METHODS=['HEAPF32']"
    )
endif ()

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE ${LINK_LIBRARIES})

# Standalone demo for the GPU text renderer (desktop only)
if(USE_HARFBUZZ AND USE_RAYLIB AND NOT ${PLATFORM} STREQUAL "Web")
    add_executable(text_demo
        tools/text_demo.cpp
        src/engine/core/text_renderer.cpp
    )
    target_include_directories(text_demo PRIVATE ${PROJECT_INCLUDE})
    target_link_libraries(text_demo PRIVATE ${LINK_LIBRARIES})
endif()

# Set common compiler options for your target
target_compile_options(${PROJECT_NAME} PRIVATE
    -Wall
    -Wextra      # More warnings
    -Wpedantic   # Strict ISO C++ compliance warnings
    # Add other warning flags like -Wshadow, -Wconversion, etc. as needed
    $<$<CONFIG:Debug>:-g -O0>
    $<$<CONFIG:Release>:-O2 -DNDEBUG>
)