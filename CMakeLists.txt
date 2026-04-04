# Minimum required version of CMake
cmake_minimum_required(VERSION 3.22)
# Project name and version
project(OpenChaosDefender VERSION 1.0)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

##########################################################################################
# Configurable options
##########################################################################################

# toggle libs to be used with ON and OFF
option(USE_RAYLIB "Use raylib" ON)
option(USE_RAYGUI "Use raygui" OFF)
option(USE_BOX2D "Use Box2D" OFF)
option(USE_NLOHMANN_JSON "Use nlohmann json" ON)


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
        GIT_TAG        master
        GIT_SHALLOW    TRUE
        GIT_PROGRESS   TRUE
    )

    fetchcontent_makeavailable(raylib)

    list(APPEND LINK_LIBRARIES raylib)
    list(APPEND PROJECT_INCLUDE ${raylib_SOURCE_DIR}/src)  # ensure raylib.h is always found
endif()


##########################################################################################
# Raygui Setup
##########################################################################################

if(USE_RAYGUI)
    set(BUILD_RAYGUI_EXAMPLES OFF CACHE BOOL "" FORCE)

    message(STATUS "Fetching raygui...")
    fetchcontent_declare(raygui
        GIT_REPOSITORY https://github.com/raysan5/raygui.git
        GIT_TAG        master
        GIT_SHALLOW    TRUE
        GIT_PROGRESS   TRUE
    )
    fetchcontent_makeavailable(raygui)

    # Adding raygui include paths
    list(APPEND PROJECT_INCLUDE ${raygui_SOURCE_DIR}/src/)
endif()


##########################################################################################
# Box2D Setup
##########################################################################################

if(USE_BOX2D)
    set(BOX2D_BUILD_TESTBED OFF CACHE BOOL "" FORCE) # don't build the supplied examples
    set(BOX2D_BUILD_UNIT_TESTS OFF CACHE BOOL "" FORCE) # don't build the supplied examples

    message(STATUS "Fetching box2d...")
    fetchcontent_declare(box2d
        GIT_REPOSITORY https://github.com/erincatto/box2d.git
        GIT_TAG        main
        GIT_SHALLOW    TRUE
        GIT_PROGRESS   TRUE
    )
    fetchcontent_makeavailable(box2d)

    # add the dependency here to be linked
    list(APPEND LIBRARIES box2d)
endif()


##########################################################################################
# Nlohmann json Setup
##########################################################################################

if(USE_NLOHMANN_JSON)
    set(BUILD_RAYGUI_EXAMPLES OFF CACHE BOOL "" FORCE)

    message(STATUS "Fetching nlohmann json...")
    fetchcontent_declare(nlohmann_json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG        master
        GIT_SHALLOW    TRUE
        GIT_PROGRESS   TRUE
    )
    fetchcontent_makeavailable(nlohmann_json)

    # add the dependency here to be linked
    list(APPEND LINK_LIBRARIES nlohmann_json::nlohmann_json)
endif()


##########################################################################################
# Project executable setup
##########################################################################################

# Specify the output directories for compiled binaries
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Adding our source files
# Define PROJECT_SOURCES as a list of all source files
file(GLOB_RECURSE PROJECT_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/src/*.cpp")

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
    
    # --preload-file assets so the assets folder is bundled into the .wasm and accessible at runtime in the browser.
    set_target_properties(${PROJECT_NAME} PROPERTIES
    SUFFIX ".html"
    LINK_FLAGS "\
        --shell-file ${CMAKE_CURRENT_LIST_DIR}/tools/minshell.html \
        --preload-file ${CMAKE_CURRENT_LIST_DIR}/assets@assets \
        -s USE_GLFW=3 \
        -s ASSERTIONS=1 \
        -s WASM=1 \
        -s GL_ENABLE_GET_PROC_ADDRESS=1 \
        -s ASYNCIFY"
    )
endif ()

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE ${LINK_LIBRARIES})

# Set common compiler options for your target
target_compile_options(${PROJECT_NAME} PRIVATE
    -Wall
    -Wextra      # More warnings
    -Wpedantic   # Strict ISO C++ compliance warnings
    # Add other warning flags like -Wshadow, -Wconversion, etc. as needed
)