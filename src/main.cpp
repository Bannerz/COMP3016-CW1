//Alex Banfield - 2024-06-05 - COMP3016 - Coursework 1

#include <SDL3/SDL.h>
//SDL supplies the entry point for macOS and Windows
#include <SDL3/SDL_main.h>
#include "Game.h"

#include <exception>

int main(int, char*[])
{
    try {
        //start the application and let Game manage its lifetime
        Game game;
        return game.run();
    } catch (const std::exception& error) {
        SDL_Log("Game failed: %s", error.what());
        return 1;
    }
}
