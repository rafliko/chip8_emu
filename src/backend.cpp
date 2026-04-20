#include "backend.h"
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <cstdio>

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

Chip8::Chip8()
{
	srand(time(NULL));

	this->I = 0;
	this->pc = START_ADDR;
	this->sp = 0;
	this->delay_timer = 0;
	this->sound_timer = 0;
	
	memset(this->memory,0,4096);
	memcpy(this->memory, chip8_fontset, 80);
	memset(this->V,0,16);
	memset(this->stack,0,32);
	memset(this->display,0,64*32);
	memset(this->keypad,0,16);
}

void Chip8::tick_timers()
{
	if (this->delay_timer != 0) this->delay_timer--;
	if (this->sound_timer != 0) this->sound_timer--;
}

uint8_t Chip8::get_sound_timer()
{
	return sound_timer;
}

int Chip8::load_rom(char* path)
{
	FILE *file;
	file = fopen(path, "rb");

	if (file == NULL) return 1;

	fseek(file, 0, SEEK_END);
    long rom_size = ftell(file);
    rewind(file);

	fread(&this->memory[START_ADDR], 1, rom_size, file);
	
	fclose(file);

	return 0;
}

void Chip8::execute()
{
	uint16_t opcode = (uint16_t)((this->memory[this->pc] << 8) | this->memory[this->pc+1]);
	this->pc += 2;
	
	uint16_t nnn = (opcode & 0x0FFF);
	uint16_t n = (opcode & 0x000F);
	uint16_t x = (opcode & 0x0F00) >> 8;
	uint16_t y = (opcode & 0x00F0) >> 4;
	uint16_t kk = (opcode & 0x00FF);

	uint16_t o1 = (opcode & 0xF000);
	uint16_t o3 = (opcode & 0x00F0);
	uint16_t o4 = (opcode & 0x000F);

	uint8_t pos_x;
	uint8_t pos_y;
	// take first 4 bits
	switch (o1) {
	case 0x1000: // 1nnn: JMP nnn
		this->pc = nnn;
		break;
	case 0x2000: // 2nnn: CALL nnn
		this->sp++;
		this->stack[this->sp] = this->pc;
		this->pc = nnn;
		break;
	case 0x3000: // 3xkk: SKIP if Vx == kk
		if (this->V[x] == kk) this->pc+=2;
		break;
	case 0x4000: // 4xkk: SKIP if Vx != kk
		if (this->V[x] != kk) this->pc+=2;
		break;
	case 0x6000: // 6xkk: Vx = kk
		this->V[x] = kk;
		break;
	case 0x7000: // 7xkk: Vx += kk
		this->V[x] += kk;
		break;
	case 0xA000: // Annn: I = nnn
		this->I = nnn;
		break;
	case 0xB000: // Bnnn: JMP nnn + V0
		this->pc = nnn+this->V[0];
		break;
	case 0xC000: // Cxkk: Vx = rng & kk
		this->V[x] = (uint8_t)(rand() % 256) & kk;
		break;
	case 0xD000: // Dxyn: DRAW Vx, Vy, n
		this->V[0xF] = 0;

		pos_x = this->V[x];
		pos_y = this->V[y];
		
		for (int row = 0; row < n; row++) {
			uint8_t sprite = this->memory[this->I + row];
			
			for (int col = 0; col < 8; col++) {
				// 0x80 = 0b10000000
				if ((sprite & (0x80 >> col)) != 0) {
					int index = ((pos_x + col) % 64) + ((pos_y + row) % 32) * 64;

					if (this->display[index] == 1) {
						this->V[0xF] = 1;
					}
					
					this->display[index] ^= 1;
				}
			}
		}
		break;
	default:
		break;
	}

	uint8_t tmpvx;
	uint16_t tmpsum;
	// take first and last 4 bits
	switch (o1 | o4) {
	case 0x5000: // 5xy0: SKIP if Vx == Vy
		if (this->V[x] == this->V[y]) this->pc+=2;
		break;
	case 0x8000: // 8xy0: Vx = Vy
		this->V[x] = this->V[y];
		break;
	case 0x8001: // 8xy1: Vx |= Vy
		this->V[x] |= this->V[y];
		this->V[0xF] = 0;
		break;
	case 0x8002: // 8xy2: Vx &= Vy
		this->V[x] &= this->V[y];
		this->V[0xF] = 0;
		break;
	case 0x8003: // 8xy3: Vx ^= Vy
		this->V[x] ^= this->V[y];
		this->V[0xF] = 0;
		break;
	case 0x8004: // 8xy4: Vx += Vy, VF = carry
		tmpsum = (uint16_t)this->V[x] + (uint16_t)this->V[y];
		this->V[x] = (uint8_t)(tmpsum & 0xFF);
		if (tmpsum > 0xFF) {
			this->V[0xF] = 1;
		} else {
			this->V[0xF] = 0;
		}
		break;
	case 0x8005: // 8xy5: Vx -= Vy, VF = NOT borrow
		tmpvx = this->V[x];
		this->V[x] -= this->V[y];
		if (tmpvx >= this->V[y]) {
			this->V[0xF] = 1;
		} else {
			this->V[0xF] = 0;
		}
		break;
	case 0x8006: // 8xy6: Vx >>= 1, VF = lsb
		tmpvx = this->V[x];
		this->V[x] = this->V[y];
		this->V[x] >>= 1;
		this->V[0xF] = tmpvx & 1;
		break;
	case 0x8007: // 8xy7: Vx = Vy - Vx, VF = NOT borrow
		tmpvx = this->V[x];
		this->V[x] = this->V[y] - this->V[x];
		if (this->V[y] >= tmpvx) {
			this->V[0xF] = 1;
		} else {
			this->V[0xF] = 0;
		}
		break;
	case 0x800E: // 8xyE: Vx <<= 1, VF = msb
		tmpvx = this->V[x];
		this->V[x] = this->V[y];
		this->V[x] <<= 1;
		this->V[0xF] = tmpvx >> 7;
		break;
	case 0x9000: // 9xy0: SKIP if Vx != Vy
		if (this->V[x] != this->V[y]) this->pc += 2;
		break;
	default:
		break;
	}

	bool pressed;
	// take first 4 and last 8 bits
	switch (o1 | o3 | o4) {
	case 0xE09E: // Ex9E: SKIP Vx PRESSED
		if (this->keypad[this->V[x]] == 1) {
			this->pc += 2;
		}
		break;
	case 0xE0A1: // ExA1: SKIP Vx NOT PRESSED
		if (this->keypad[this->V[x]] == 0) {
			this->pc += 2;
		}
		break;
	case 0xF007: // Fx07: Vx = delay_timer
		this->V[x] = this->delay_timer;
		break;
	case 0xF00A: // Fx0A: Wait for a key press
		pressed = false;
		for (int i = 0; i < 16; i++) {
			if (this->keypad[i] == 1) {
				pressed = true;
				break;
			}
		}
		if (!pressed) this->pc -= 2;
		break;
	case 0xF015: // Fx15: delay_timer = Vx
		this->delay_timer = this->V[x];
		break;
	case 0xF018: // Fx18: sound_timer = Vx
		this->sound_timer = this->V[x];
		break;
	case 0xF01E: // Fx1E: I += Vx
		this->I += this->V[x];
		break;
	case 0xF029: // Fx29: I = font_addr
		this->I = this->V[x]*5;
		break;
	case 0xF033: // Fx33: BCD of Vx in I, I+1, I+2
		this->memory[this->I] = this->V[x] / 100;
		this->memory[this->I+1] = this->V[x] / 10 % 10;
		this->memory[this->I+2] = this->V[x] % 10;
		break;
	case 0xF055: // Fx55: Store V0 - Vx into I
		for (int i=0; i<=x; i++) {
			this->memory[this->I+i] = this->V[i];
		}
		break;
	case 0xF065: // Fx65: Load I into V0 - Vx
		for (int i=0; i<=x; i++) {
			this->V[i] = this->memory[this->I+i];
		}
		break;
	default:
		break;
	}

	switch (opcode) {
	case 0x00E0: // 00E0: CLS
		for (int i = 0; i<64*32; i++){
			this->display[i] = 0;
		}
		break;
	case 0x00EE: // 00EE: RET
		this->pc = this->stack[this->sp];
		this->sp -= 1;
		break;
	default:
		break;
	}
}
