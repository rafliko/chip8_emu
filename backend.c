#include "backend.h"
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

uint8_t chip8_fontset[80] = {
	0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
	0x20, 0x60, 0x20, 0x20, 0x70, // 1
	0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
	0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
	0x90, 0x90, 0xF0, 0x10, 0x10, // 4
	0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
	0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
	0xF0, 0x10, 0x20, 0x40, 0x40, // 7
	0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
	0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
	0xF0, 0x90, 0xF0, 0x90, 0x90, // A
	0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
	0xF0, 0x80, 0x80, 0x80, 0xF0, // C
	0xE0, 0x90, 0x90, 0x90, 0xE0, // D
	0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
	0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

void execute_instruction(Chip8 *chip8, uint16_t opcode)
{
	srand(time(NULL));
	
	uint16_t nnn = (opcode & 0x0FFF);
	uint16_t n = (opcode & 0x000F);
	uint16_t x = (opcode & 0x0F00) >> 8;
	uint16_t y = (opcode & 0x00F0) >> 4;
	uint16_t kk = (opcode & 0x00FF);

	uint16_t o1 = (opcode & 0xF000);
	uint16_t o2 = (opcode & 0x0F00);
	uint16_t o3 = (opcode & 0x00F0);
	uint16_t o4 = (opcode & 0x000F);

	
	// take first 4 bits
	switch (o1) {
	case 0x1000: // 1nnn: JMP nnn
		chip8->pc = nnn;
		break;
	case 0x2000: // 2nnn: CALL nnn
		chip8->sp++;
		*(chip8->sp) = chip8->pc;
		chip8->pc = nnn;
		break;
	case 0x3000: // 3xkk: SKIP if Vx == kk
		if (chip8->V[x] == kk) chip8->pc+=2;
		break;
	case 0x4000: // 4xkk: SKIP if Vx != kk
		if (chip8->V[x] != kk) chip8->pc+=2;
		break;
	case 0x6000: // 6xkk: Vx = kk
		chip8->V[x] = kk;
		break;
	case 0x7000: // 7xkk: Vx += kk
		chip8->V[x] += kk;
		break;
	case 0xA000: // Annn: I = nnn
		chip8->I = nnn;
		break;
	case 0xB000: // Bnnn: JMP nnn + V0
		chip8->pc = nnn+chip8->V[0];
		break;
	case 0xC000: // Cxkk: Vx = rng & kk
		chip8->V[x] = (uint8_t)(rand() % 256) & kk;
		break;
	case 0xD000: // Dxyn: DRAW Vx, Vy, n
		// TODO
		break;
	default:
		break;
	}
	

	// take first and last 4 bits
	switch (o1 | o4) {
	case 0x5000: // 5xy0: SKIP if Vx == Vy
		if (chip8->V[x] == chip8->V[y]) chip8->pc+=2;
		break;
	case 0x8000: // 8xy0: Vx = Vy
		chip8->V[x] = chip8->V[y];
		break;
	case 0x8001: // 8xy1: Vx |= Vy
		chip8->V[x] |= chip8->V[y];
		break;
	case 0x8002: // 8xy2: Vx &= Vy
		chip8->V[x] &= chip8->V[y];
		break;
	case 0x8003: // 8xy3: Vx ^= Vy
		chip8->V[x] ^= chip8->V[y];
		break;
	case 0x8004: // 8xy4: Vx += Vy, VF = carry
		uint16_t sum = (uint16_t)chip8->V[x] + (uint16_t)chip8->V[y];
		if (sum > 0xFF) {
			chip8->V[0xF] = 1;
		} else {
			chip8->V[0xF] = 0;
		}
		chip8->V[x] = (uint8_t)(sum & 0xFF);
		break;
	case 0x8005: // 8xy5: Vx -= Vy, VF = NOT borrow
		if (chip8->V[x] > chip8->V[y]) {
			chip8->V[0xF] = 1;
		} else {
			chip8->V[0xF] = 0;
		}
		chip8->V[x] -= chip8->V[y];
		break;
	case 0x8006: // 8xy6: Vx >>= 1, VF = lsb
		chip8->V[0xF] = chip8->V[x] & 1;
		chip8->V[x] >>= 1;
		break;
	case 0x8007: // 8xy7: Vx = Vy - Vx, VF = NOT borrow
		if (chip8->V[y] > chip8->V[x]) {
			chip8->V[0xF] = 1;
		} else {
			chip8->V[0xF] = 0;
		}
		chip8->V[x] = chip8->V[y] - chip8->V[x];
		break;
	case 0x8008: // 8xy8: Vx <<= 1, VF = msb
		chip8->V[0xF] = chip8->V[x] >> 7;
		chip8->V[x] <<= 1;
		break;
	case 0x9000: // 9xy0: SKIP if Vx != Vy
		if (chip8->V[x] != chip8->V[y]) chip8->pc += 2;
		break;
	default:
		break;
	}

	
	chip8->pc += 2;
}
