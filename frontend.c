#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <stdio.h>
#include "backend.h"

#define SCALE 20.0f

int main(int argc, char* argv[]) {
	if (argc != 2) {
		printf("Invalid number of arguments. Please specify the rom file.");
		return 1;
	}

	Chip8 c8;
	chip8_init(&c8);
	chip8_load_rom(&c8, argv[1]);
		
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL could not initialize! SDL_Error: %s", SDL_GetError());
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("SDL3 Example", 64*SCALE, 32*SCALE, 0);
    if (!window) {
        SDL_Log("Window could not be created! SDL_Error: %s", SDL_GetError());
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);
    if (!renderer) {
        SDL_Log("Renderer could not be created! SDL_Error: %s", SDL_GetError());
        return 1;
    }

	unsigned int lastTime = 0, currentTime;
    bool quit = false;
    SDL_Event e;
    while (!quit) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT) {
                quit = true;
            }
        }

		currentTime = SDL_GetTicks();
		if (currentTime > lastTime + 1000/60) {
			chip8_tick_timers(&c8);
			lastTime = currentTime;
		}

		chip8_execute_instruction(&c8);

        // Clear screen
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

		// Draw
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

		for (int i=0; i<64*32; i++) {
			if (c8.display[i] == 1) {
				SDL_FRect rectFill = { (i%64)*SCALE, (i/64)*SCALE, SCALE, SCALE };
				SDL_RenderFillRect(renderer, &rectFill);
			}
		}
        
        // Present renderer
        SDL_RenderPresent(renderer);
    }

    // 5. Cleanup
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
