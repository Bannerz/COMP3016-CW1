cmake_minimum_required(VERSION 3.24)
project(COMP3016Game VERSION 0.1.0 LANGUAGES C CXX)

include(FetchContent)

# Build dependencies from source on each platform; no machine-specific SDK paths.
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build static dependencies" FORCE)
set(SDL_SHARED OFF CACHE BOOL "Build shared SDL" FORCE)
set(SDL_STATIC ON CACHE BOOL "Build static SDL" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "Build SDL test library" FORCE)
set(SDL_TESTS OFF CACHE BOOL "Build SDL tests" FORCE)
set(SDL_EXAMPLES OFF CACHE BOOL "Build SDL examples" FORCE)

FetchContent_Declare(SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-3.2.28
    GIT_SHALLOW TRUE
)
FetchContent_Declare(box2d
    GIT_REPOSITORY https://github.com/erincatto/box2d.git
    GIT_TAG v3.1.1
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(SDL3 box2d)

add_executable(game src/main.cpp)
target_compile_features(game PRIVATE cxx_std_20)
set_target_properties(game PROPERTIES CXX_EXTENSIONS OFF)
target_link_libraries(game PRIVATE SDL3::SDL3-static box2d::box2d)
if(MSVC)
    target_compile_options(game PRIVATE /W4 /permissive- /utf-8)
else()
    target_compile_options(game PRIVATE -Wall -Wextra -Wpedantic)
endif()

