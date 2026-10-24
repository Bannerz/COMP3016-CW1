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
#use bundled png/jped loader without extra libraries
set(SDLIMAGE_BACKEND_STB ON CACHE BOOL "" FORCE)
set(SDLIMAGE_BACKEND_IMAGEIO OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_VENDORED OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_SAMPLES OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_TESTS OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_PNG_SAVE OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_JPG_SAVE OFF CACHE BOOL "" FORCE)

foreach(format AVIF JXL TIF WEBP)
    set(SDLIMAGE_${format} OFF CACHE BOOL "" FORCE)
endforeach()

FetchContent_Declare(SDL_image
    GIT_REPOSITORY  https://github.com/libsdl-org/SDL_image.git
    GIT_TAG release-3.2.4
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(SDL3 box2d SDL_image)

add_executable(game src/main.cpp)
target_compile_features(game PRIVATE cxx_std_20)
set_target_properties(game PROPERTIES CXX_EXTENSIONS OFF)
target_link_libraries(game PRIVATE SDL3::SDL3-static box2d::box2d SDL3_image::SDL3_image)
if(MSVC)
    target_compile_options(game PRIVATE /W4 /permissive- /utf-8)
else()
    target_compile_options(game PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_custom_target(game_assets
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/src/assets"
        "$<TARGET_FILE_DIR:game>/assets"
    COMMENT "Copying game assets"
    VERBATIM
)

add_dependencies(game game_assets)
