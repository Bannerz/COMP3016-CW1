//Alex Banfield - 2024-06-05 - COMP3016 - Coursework 1

#include <SDL3/SDL.h>
//SDL supplies the platform-specific entry point for macOS and Windows.
#include <SDL3/SDL_main.h>

//other includes above and helper functions/classes

int main(int, char*[])
{
    //start the video system before creating a window.
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL initialization failed: %s", SDL_GetError());
        return 1;
    }

    //create window and renderer
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    if (!SDL_CreateWindowAndRenderer("COMP3016", 960, 640, SDL_WINDOW_RESIZABLE,
                                    &window, &renderer)) {
        SDL_Log("Window/renderer creation failed: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    //set up code - to fill

    //game objects, assets etc. should be created here, after the window and renderer are created

    //main loop
    bool running = true;
    while (running) {
        //process all pending window, keyboard, and mouse events.
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }

            //input handling code - to fill
        }

        //update code - to fill
        //this runs each frame; give physics a fixed timestep when added.

        //clear the previous frame with a black background
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        //draw shapes, sprites, and text after clearing.

        //display the completed frame in the window.
        SDL_RenderPresent(renderer);

        //pause briefly to avoid a busy loop. This is not a precise frame timer.
        SDL_Delay(16);
    }

    //cleanup code - to fill

    //shut down
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
