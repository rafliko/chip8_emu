#ifndef backend_h
#define backend_h

#include <stdint.h>
#include <stdbool.h>

#define START_ADDR 0x200

typedef struct {
    uint8_t memory[4096];
    uint8_t V[16];
    uint16_t I;
    uint16_t pc; // program counter
    uint16_t stack[16];
    uint16_t sp; // stack pointer
    uint8_t delay_timer;
    uint8_t sound_timer;
    bool display[64 * 32];
    bool keypad[16];
} Chip8;

void chip8_init(Chip8* chip8);
void chip8_tick_timers(Chip8 *chip8);
int chip8_load_rom(Chip8* chip8, char* path);
void chip8_execute_instruction(Chip8* chip8);

#endif
