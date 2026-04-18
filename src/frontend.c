#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <stdio.h>
#include <math.h>
#include "backend.h"

#define SCALE 20.0f

int keymap(SDL_Keycode keycode) 
{
    switch (keycode) {
    case SDLK_1: return 0x1;
    case SDLK_2: return 0x2;
    case SDLK_3: return 0x3;
    case SDLK_4: return 0xC;
    case SDLK_Q: return 0x4;
    case SDLK_W: return 0x5;
    case SDLK_E: return 0x6;
    case SDLK_R: return 0xD;
    case SDLK_A: return 0x7;
    case SDLK_S: return 0x8;
    case SDLK_D: return 0x9;
    case SDLK_F: return 0xE;
    case SDLK_Z: return 0xA;
    case SDLK_X: return 0x0;
    case SDLK_C: return 0xB;
    case SDLK_V: return 0xF;
    default: return -1;
    }
}

void create_audio_samples(SDL_AudioSpec spec, float frequency, float amplitude, int num_samples, float* beep_samples)
{
    for (int i = 0; i < num_samples; ++i) {
        float time = (float)i / spec.freq;
        beep_samples[i] = (sinf(2.0f * M_PI * frequency * time) > 0) ? amplitude : -amplitude;
    }
}

int main(int argc, char* argv[]) 
{
	if (argc != 2) {
		printf("Invalid number of arguments. Please specify the rom file.");
		return 1;
	}

    // Chip-8 init
	Chip8 chip8;
	chip8_init(&chip8);
    if(chip8_load_rom(&chip8, argv[1]) == 1) {
        printf("Not able to open the file.");
        return 1;
    }

    // Audio init
    if (!SDL_Init(SDL_INIT_AUDIO)) {
        SDL_Log("SDL could not initialize! SDL_Error: %s", SDL_GetError());
        return 1;
    }
    SDL_AudioSpec spec = { SDL_AUDIO_F32, 1, 44100 };
    SDL_AudioStream* stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, NULL, NULL);
    if (!stream) {
        SDL_Log("Audio stream error! SDL_Error: %s", SDL_GetError());
        return -1;
    }
    SDL_ResumeAudioStreamDevice(stream);

    float beep_samples[4410];
    create_audio_samples(spec, 440.0f, 0.2f, sizeof(beep_samples)/sizeof(float), beep_samples);
		
    // Video init
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL could not initialize! SDL_Error: %s", SDL_GetError());
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("Chip-8 emulator", 64*SCALE, 32*SCALE, 0);
    if (!window) {
        SDL_Log("Window could not be created! SDL_Error: %s", SDL_GetError());
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);
    if (!renderer) {
        SDL_Log("Renderer could not be created! SDL_Error: %s", SDL_GetError());
        return 1;
    }
    SDL_SetRenderVSync(renderer, 1);

    // Main loop
	unsigned int lastTime = 0, currentTime;
    int btn;
    bool quit = false;
    SDL_Event e;
    while (!quit) {
        // Event loop
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT) {
                quit = true;
            } else if (e.type == SDL_EVENT_KEY_DOWN) {
                btn = keymap(e.key.key);
                if (btn != -1) chip8.keypad[btn] = 1;
                else if (e.key.key == SDLK_ESCAPE) quit = true;       
            } else if (e.type == SDL_EVENT_KEY_UP) {
                btn = keymap(e.key.key);
                if (btn != -1) chip8.keypad[btn] = 0;
            }
        }

        // Execute twice per frame
        for (int i=0; i<2; i++) chip8_execute(&chip8);
        // Tick timers once per frame
        chip8_tick_timers(&chip8);
        // Beep
        if (chip8.sound_timer > 0) {
            SDL_PutAudioStreamData(stream, beep_samples, sizeof(beep_samples));
        } else {
            SDL_ClearAudioStream(stream);
        }

        // Clear screen
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

		// Draw
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

		for (int i=0; i<64*32; i++) {
			if (chip8.display[i] == 1) {
				SDL_FRect rectFill = { (i%64)*SCALE, (i/64)*SCALE, SCALE, SCALE };
				SDL_RenderFillRect(renderer, &rectFill);
			}
		}
        
        // Present renderer
        SDL_RenderPresent(renderer);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
