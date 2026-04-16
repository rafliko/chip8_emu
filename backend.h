#ifndef backend_h
#define backend_h

#include <stdint.h>
#include <stdbool.h>

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
    uint8_t keypad[16];
} Chip8;

void execute_instruction(Chip8* chip8, uint16_t opcode);

#endif
