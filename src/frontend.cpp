#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <cstdio>
#include <cmath>
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlrenderer3.h"

#include "backend.h"

#define SCALE 20.0f
#define MENU_PADDING 20.0f
#define BASE_SPEED 10

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
    char filename[256] = "";

    // Chip-8 init
	Chip8* chip8 = new Chip8();
    if (argc == 2) {
        strcpy(filename, argv[1]);
        if(chip8->load_rom(filename) == 1) {
            printf("Not able to open the file.\n");
        }
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

    SDL_Window* window = SDL_CreateWindow("Chip-8 emulator", 64*SCALE, 32*SCALE+MENU_PADDING, 0);
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

    // Create ImGui context
    ImGui::CreateContext();
    ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer3_Init(renderer);

    // Variables used in the main loop
	unsigned int lastTime = 0, currentTime;
    bool quit = false;
    int emu_speed = 10;
    bool load_rom_dialog = false;
    uint32_t last_tick_time = 0;
    uint32_t delta = 0;
    uint32_t timer_60hz = 0;
    SDL_Color clear_color = { 0, 0, 0, 255 };
    SDL_Color draw_color = { 255, 255, 255, 255 };
    
    // Main loop
    while (!quit) {
        // Event loop
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            int btn;
            ImGui_ImplSDL3_ProcessEvent(&e);
            if (e.type == SDL_EVENT_QUIT) {
                quit = true;
            } 
            else if (e.type == SDL_EVENT_KEY_DOWN) {
                btn = keymap(e.key.key);
                if (btn != -1) chip8->keypad[btn] = 1;
                else if (e.key.key == SDLK_ESCAPE) quit = true; 
                else if (!load_rom_dialog && e.key.key == SDLK_P) emu_speed==0? emu_speed=BASE_SPEED : emu_speed=0;
            } 
            else if (e.type == SDL_EVENT_KEY_UP) {
                btn = keymap(e.key.key);
                if (btn != -1) chip8->keypad[btn] = 0;
            }
        }

        // Calculate deltatime
        uint32_t tick_time = SDL_GetTicks();
        delta = tick_time;
        last_tick_time = tick_time;
        timer_60hz += delta;

        // Tick timers and execute at 60hz
        if (timer_60hz > 1000/60) {
            for (int i=0; i<emu_speed; i++) chip8->execute();
            chip8->tick_timers();
            timer_60hz = 0;
        }
        // Beep
        if (chip8->get_sound_timer() > 0) {
            SDL_PutAudioStreamData(stream, beep_samples, sizeof(beep_samples));
        } else {
            SDL_ClearAudioStream(stream);
        }

        // ImGui frame
        ImGui_ImplSDLRenderer3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        if (ImGui::BeginMainMenuBar()) {

            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("Load ROM")) { 
                    load_rom_dialog = true;
                }

                ImGui::Separator();

                if (ImGui::MenuItem("Exit", "ESC")) {
                    quit = true;
                }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Emulation")) {
                if (ImGui::MenuItem("Reload")) {
                    delete(chip8);
                    chip8 = new Chip8();
                    if (chip8->load_rom(filename) == 1) {
                        printf("Not able to open the file.\n");
                    }
                }
                if (ImGui::MenuItem("Stop")) {
                    delete(chip8);
                    chip8 = new Chip8();
                }
                if (ImGui::BeginMenu("Speed")) {
                    if (ImGui::MenuItem("0.0x (freeze)", "P")) { emu_speed = 0; }
                    if (ImGui::MenuItem("0.5x ")) { emu_speed = BASE_SPEED/2; }
                    if (ImGui::MenuItem("1.0x")) { emu_speed = BASE_SPEED; }
                    if (ImGui::MenuItem("2.0x")) { emu_speed = BASE_SPEED*2; }
                    ImGui::EndMenu();
                }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Settings")) {                
                if (ImGui::BeginMenu("Theme")) {
                    if (ImGui::MenuItem("B/W")) { 
                        clear_color = { 0, 0, 0, 255}; 
                        draw_color = { 255, 255, 255, 255};
                    }
                    if (ImGui::MenuItem("Green")) { 
                        clear_color = { 154, 186, 16, 255}; 
                        draw_color = { 48, 97, 49, 255};
                    }
                    if (ImGui::MenuItem("Yellow")) { 
                        clear_color = { 154, 102, 1, 255}; 
                        draw_color = { 250, 206, 2, 255};
                    }
                    if (ImGui::MenuItem("Ice")) { 
                        clear_color = { 174, 224, 230, 255}; 
                        draw_color = { 0, 0, 128, 255};
                    }
                    ImGui::EndMenu();
                }
                ImGui::EndMenu();
            }

            // Show info
            float width = ImGui::GetWindowWidth();
            ImGui::SameLine(width-170); 
            ImGui::TextDisabled("FPS: %.1f", ImGui::GetIO().Framerate);
            ImGui::TextDisabled("Speed: %.1fx", (float)emu_speed/BASE_SPEED);

            ImGui::EndMainMenuBar();
        }

        // Load rom window
        if (load_rom_dialog) {
            ImGui::Begin("Load ROM", &load_rom_dialog, ImGuiWindowFlags_AlwaysAutoResize);
            if (ImGui::InputText("ROM path", filename, IM_ARRAYSIZE(filename), ImGuiInputTextFlags_EnterReturnsTrue)) {
                delete(chip8);
                chip8 = new Chip8();
                if (chip8->load_rom(filename) == 1) {
                    printf("Not able to open the file.\n");
                }
                load_rom_dialog = false;
            }
            ImGui::End();
        }

        // End ImGui frame
        ImGui::Render();

        // Clear screen
        SDL_SetRenderDrawColor(renderer, clear_color.r, clear_color.g, clear_color.b, clear_color.a);
        SDL_RenderClear(renderer);
        
		// Draw
        SDL_SetRenderDrawColor(renderer, draw_color.r, draw_color.g, draw_color.b, draw_color.a);

        for (int i=0; i<64*32; i++) {
            if (chip8->display[i] == 1) {
                SDL_FRect rectFill = { (i%64)*SCALE, (i/64)*SCALE+MENU_PADDING, SCALE, SCALE };
                SDL_RenderFillRect(renderer, &rectFill);
            }
        }

        // Render ImGui data
        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
        
        // Present renderer
        SDL_RenderPresent(renderer);
    }

    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
