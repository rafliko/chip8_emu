#pragma once

#include <cstdint>
#define START_ADDR 0x200

class Chip8 {
public:
    bool display[64 * 32];
    bool keypad[16];

    Chip8();
    void tick_timers();
    uint8_t get_sound_timer();
    int load_rom(char* path);
    void execute();

private:
    uint8_t memory[4096];
    uint8_t V[16];
    uint16_t I;
    uint16_t pc; // program counter
    uint16_t stack[16];
    uint16_t sp; // stack pointer
    uint8_t delay_timer;
    uint8_t sound_timer;
};