#include "backend.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>
#include <stdio.h>

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

void chip8_init(Chip8 *chip8)
{
	srand(time(NULL));
	memset(chip8->memory,0,4096);
	memcpy(chip8->memory, chip8_fontset, 80);
	memset(chip8->V,0,16);
	chip8->I = 0;
	chip8->pc = START_ADDR;
	memset(chip8->stack,0,32);
	chip8->sp = 0;
	chip8->delay_timer = 0;
	chip8->sound_timer = 0;
	memset(chip8->display,0,64*32);
	memset(chip8->keypad,0,16);
}

void chip8_tick_timers(Chip8 *chip8)
{
	if (chip8->delay_timer != 0) chip8->delay_timer--;
	if (chip8->sound_timer != 0) chip8->sound_timer--;
}

int chip8_load_rom(Chip8* chip8, char* path)
{
	FILE *file;
	file = fopen(path, "rb");

	if (file == NULL) return 1;

	fseek(file, 0, SEEK_END);
    long rom_size = ftell(file);
    rewind(file);

	fread(&chip8->memory[START_ADDR], 1, rom_size, file);
	
	fclose(file);

	return 0;
}

void chip8_execute_instruction(Chip8 *chip8)
{
	uint16_t opcode = (uint16_t)((chip8->memory[chip8->pc] << 8) | chip8->memory[chip8->pc+1]);
	chip8->pc += 2;
	
	uint16_t nnn = (opcode & 0x0FFF);
	uint16_t n = (opcode & 0x000F);
	uint16_t x = (opcode & 0x0F00) >> 8;
	uint16_t y = (opcode & 0x00F0) >> 4;
	uint16_t kk = (opcode & 0x00FF);

	uint16_t o1 = (opcode & 0xF000);
	uint16_t o3 = (opcode & 0x00F0);
	uint16_t o4 = (opcode & 0x000F);
	
	// take first 4 bits
	switch (o1) {
	case 0x1000: // 1nnn: JMP nnn
		chip8->pc = nnn;
		break;
	case 0x2000: // 2nnn: CALL nnn
		chip8->sp++;
		chip8->stack[chip8->sp] = chip8->pc+2;
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
		uint8_t pos_x = chip8->V[x];
		uint8_t pos_y = chip8->V[y];
		uint8_t height = n;

		//printf("pos_x:%d pos_y:%d height:%d\n", pos_x, pos_y, height);
		
		for (int row = 0; row < height; row++) {
			uint8_t sprite = chip8->memory[chip8->I + row];
			
			for (int col = 0; col < 8; col++) {
				// 0x80 = 0b10000000
				if ((sprite & (0x80 >> col)) != 0) {
					int index = ((pos_x + col) % 64) + ((pos_y + row) % 32) * 64;

					if (chip8->display[index] == 1) {
						chip8->V[0xF] = 1;
					}
					
					chip8->display[index] ^= 1;
				}
			}
		}
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

	// take first 4 and last 8 bits
	switch (o1 | o3 | o4) {
	case 0xE09E: // Ex9E: SKIP Vx PRESSED
		if (chip8->keypad[chip8->V[x]] == 1) {
			chip8->pc += 2;
		}
		break;
	case 0xE0A1: // ExA1: SKIP Vx NOT PRESSED
		if (chip8->keypad[chip8->V[x]] == 0) {
			chip8->pc += 2;
		}
		break;
	case 0xF007: // Fx07: Vx = delay_timer
		chip8->V[x] = chip8->delay_timer;
		break;
	case 0xF00A: // Fx0A: Wait for a key press
		bool pressed = false;
		for (int i = 0; i < 16; i++) {
			if (chip8->keypad[i] == 1) {
				pressed = true;
				break;
			}
		}
		if (!pressed) chip8->pc -= 2;
		break;
	case 0xF015: // Fx15: delay_timer = Vx
		chip8->delay_timer = chip8->V[x];
		break;
	case 0xF018: // Fx18: sound_timer = Vx
		chip8->sound_timer = chip8->V[x];
		break;
	case 0xF01E: // Fx1E: I += Vx
		chip8->I += chip8->V[x];
		break;
	case 0xF029: // Fx29: I = font_addr
		chip8->I = chip8->V[x]*5;
		break;
	case 0xF033: // Fx33: BCD of Vx in I, I+1, I+2
		chip8->memory[chip8->I] = chip8->V[x] / 100;
		chip8->memory[chip8->I+1] = chip8->V[x] / 10 % 10;
		chip8->memory[chip8->I+2] = chip8->V[x] % 10;
		break;
	case 0xF055: // Fx55: Store V0 - Vx into I
		for (int i=0; i<=x; i++) {
			chip8->memory[chip8->I+i] = chip8->V[i];
		}
		break;
	case 0xF065: // Fx65: Load I into V0 - Vx
		for (int i=0; i<=x; i++) {
			chip8->V[i] = chip8->memory[chip8->I+i];
		}
		break;
	default:
		break;
	}

	switch (opcode) {
	case 0x00E0: // 00E0: CLS
		for (int i = 0; i<64*32; i++){
			chip8->display[i] = 0;
		}
		break;
	case 0x00EE: // 00EE: RET
		chip8->pc = chip8->stack[chip8->sp];
		chip8->sp -= 1;
		break;
	default:
		break;
	}
	
	// printf("Opcode: %x\n", opcode);
	// printf("PC: %x\n", chip8->pc);
	// printf("I: %x\n", chip8->I);
}
