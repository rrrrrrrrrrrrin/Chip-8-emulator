#include "chip8.h"
#include <cstdio>  // for printf

// For rand()
#define RAND_MAX 255

void Chip8::clear_display()
{
	for (int i = 0; i < 64 * 32; i++) {
		gfx[i] = 0;
	}
}

// Methods
void Chip8::initialize()
{
	pc = 0x200;  // programs start at location 0x200 (512)
	opcode = 0;
	I = 0;
	sp = 0;

	clear_display();

	// Clear memory
	for (int i = 0; i < 4096; i++) {
		memory[i] = 0;
	}

	// Clear registers V0 - VF
	for (int i = 0; i < 16; i++) {
		V[i] = 0;
	}

	// Clear stack
	for (int i = 0; i < 16; i++) {
		stack[i] = 0;
	}

	// Load the fontset (group of sprites representing 0-F stored in memory to 0x50)
	for (int i = 0; i < 80; i++) {
		memory[i] = fontset[i];
	}

	// Reset timers
	delay_timer = 60;
	sound_timer = 60;

	draw_flag = false;
}

void Chip8::loadROM(int file_size, std::vector<char> buffer)
{
	for (int i = 0; i < file_size; i++) {
		memory[i + 512] = buffer[i];  // Start filling the memory at location 0x200
	}
}

void Chip8::draw(char VX, char VY, char N)
{
	// for the sprites to wrap: X % 64 (display - 64x32) or X & 63
	char X = V[VX >> 8] & 63;  // move VX (opcode & 0x0F00) to the last nibble left to right
	char Y = V[VY >> 4] & 31;  // move VY (opcode & 0x00F0) to the last nibble left to right

	V[0xF] = 0;

	for (int heightpx = 0; heightpx < N; heightpx++)
	{
		// Read N bytes starting from I
		unsigned short byte = memory[I + heightpx];

		// Process each byte
		for (int widthpx = 0; widthpx < 8; widthpx++)
		{
			if ((byte & (0x80 >> widthpx)) != 0)  // data & (0x80 >> widthpx) is to parse byte by bits from left to right
			{
				if (gfx[X + widthpx + ((Y + heightpx) * 64)] == 1) {
					V[0xF] = 1;  // VF is 1 if gfx pxs are flipped from set to unset; collision occured
				}
				gfx[X + widthpx + ((Y + heightpx) * 64)] ^= 1;
			}
		}
	}
	draw_flag = true;
}

void Chip8::decodeOpcodes() {
	printf("1 byte: 0x%X\n", memory[pc]);
	printf("2 byte: 0x%X\n", memory[pc + 1]);
	printf("Full opcode: 0x%X\n", opcode);
}

void Chip8::emulateCycle() {
	// Fetch opcode
	opcode = memory[pc] << 8 | memory[pc + 1];  // fetch two bytes from memory and merge into one opcode

	// decodeOpcodes();

	// Decode opcode
	switch (opcode & 0xF000)  // check opcode through leftmost nibble (most significant nibble)
	{
	case 0x0000:
		switch (opcode & 0x000F)
		{
			// 00E0: Clear display
		case 0x0000:
			clear_display();
			pc += 2;  // opcode is 2 bytes. Move program counter two cells in the memory (one cell - one byte)
			break;

			// 00EE: Returns from a subroutine
		case 0x000E:
			pc = stack[15];
			break;

		default:
			printf("Unknown opcode [0x0000]: 0x%X\n", opcode);
		}
		break;

	// 1NNN: Jumps to address NNN
	case 0x1000:
		pc += opcode & 0x0FFF;
		break;

	// 3XNN: Skips the next opcode if VX equals NN
	case 0x3000:
		if ((V[(opcode & 0x0F00) >> 8]) == (opcode & 0x00FF)) {
			pc += 2;
		}
		break;
		
	// 4XNN: Skips the next opcode if VX equals NN
	case 0x4000:
		if ((V[(opcode & 0x0F00) >> 8]) != (opcode & 0x00FF)) {
			pc += 2;
		}
		break;
	
	// 5XY0: Skips the next opcode if VX equals VY
	case 0x5000:
		if (V[(opcode & 0x0F00) >> 8] == V[(opcode & 0x00F0) >> 4]) {
			pc += 2;
		}
		break;

	// 6XNN: Sets VX to NN
	case 0x6000:
		V[(opcode & 0x0F00) >> 8] = opcode & 0x00FF;
		pc += 2;
		break;

	// 7XNN: Adds NN to VX
	case 0x7000:
		V[(opcode & 0x0F00) >> 8] += opcode & 0x00FF;
		pc += 2;
		break;

	case 0x8000:
		switch (opcode & 0x000F)
		{
		// 8XY0: Stores the value of register VY in register VX
		case 0x0000:
			V[opcode & 0x0F00 >> 8] = V[opcode & 0x00F0 >> 4];
			pc += 2;
			break;

		// 8XY1: Set VX = VX OR VY
		case 0x0001:
			unsigned int X = opcode & 0x0F00 >> 8;
			V[X] = V[X] | V[opcode & 0x00F0 >> 4];
			pc += 2;
			break;

		// 8XY2: Set VX = VX AND VY
		case 0x0002:
			unsigned int X = opcode & 0x0F00 >> 8;
			V[X] = V[X] & V[opcode & 0x00F0 >> 4];
			pc += 2;
			break;

		// 8XY3:  Set VX = VX XOR VY
		case 0x0003:
			unsigned int X = opcode & 0x0F00 >> 8;
			V[X] = V[X] ^ V[opcode & 0x00F0 >> 4];
			pc += 2;
			break;

		// 8XY4: Set VX = VX + VY, set VF = carry
		case 0x0004:
			unsigned int X = opcode & 0x0F00 >> 8;
			unsigned short sum = V[X] + V[opcode & 0x00F0 >> 4];
			V[0xF] = 0;

			// If the result is greater than 8 bits (size of char) (> 255), VF is set to 1
			// Only the lowest (rightmost) 8 bits are stored in VX
			if (sum > 255)
			{
				V[0xF] = 1;
			}
			V[X] = sum & 0x00FF;
			pc += 2;
			break;

		// 8XY5: Set Vx = Vx - Vy, set VF to 1 if VX > VY (no borrow in subtraction)
		case 0x0005:
			unsigned int X = opcode & 0x0F00 >> 8;
			unsigned int Y = opcode & 0x00F0 >> 4;
			V[0xF] = 0;

			if (V[X] > V[Y])
			{
				V[0xF] = 1;
			}
			V[X] = V[X] - V[Y];
			pc += 2;
			break;

		// 8XY6: Set VX = VX SHR 1 (VX / 2), set VF to 1 if LSBit is 1 (SHR is shift right bitwise operator >>)
		case 0x0006:
			unsigned int X = opcode & 0x0F00 >> 8;
			unsigned int Y = opcode & 0x00F0 >> 4;
			V[0xF] = 0;

			if ((V[X] & 0x0000000F) == 1) {
				V[0xF] = 1;
			}
			V[X] >>= 1; 
			pc += 2;
			break;

		// 8XY7: Set VX = VY - VX, set VF to 1 if VY > VX (no borrow in subtraction)
		case 0x0007:
			unsigned int X = opcode & 0x0F00 >> 8;
			unsigned int Y = opcode & 0x00F0 >> 4;
			V[0xF] = 0;

			if (V[Y] > V[X])
			{
				V[0xF] = 1;
			}
			V[X] = V[Y] - V[X];
			pc += 2;
			break;

		// 8XYE: Set Vx = Vx SHL 1, set VF to 1 if MSBit is 1
		case 0x000E:
			unsigned int X = opcode & 0x0F00 >> 8;
			unsigned int Y = opcode & 0x00F0 >> 4;
			V[0xF] = 0;

			if ((V[X] & 0xF0000000) == 1) {
				V[0xF] = 1;
			}
			V[X] <<= 1;
			pc += 2;
			break;

		default:
			printf("Unknown opcode [0x8000]: 0x%X\n", opcode);
		}
		break;

	// 9XY0:
	case 0x9000:
		if (V[(opcode & 0x0F00) >> 8] != V[(opcode & 0x00F0) >> 4]) {
			pc += 2;
		}
		break;

	// ANNN: Sets I to the address NNN
	case 0xA000:
		I = opcode & 0x0FFF;
		pc += 2;
		break;

	// BNNN: Jump to location NNN + V0
	case 0xB000:
		pc = opcode & 0x0FFF + V[0];
		break;

	// CXNN: Set VX = random byte (random number from 0 to 255) AND NN
	case 0xC000:
		V[opcode & 0x0F00 >> 8] = rand() & opcode & 0x00FF;
		pc += 2;
		break;

	// DXYN: Draws a sprite at coordiate (VX, VY), width - 8 pxs, height - N pxs
	case 0xD000:
		draw(opcode & 0x0F00, opcode & 0x00F0, opcode & 0x000F);
		pc += 2;
		break;

	case 0xE000:
		switch (opcode & 0x000F)
		{
		// EX9E: Skip next opcode if key with the value of VX is pressed
		case 0x000E:
			// TODO: check if the key is pressed on the keyboard
			if (V[(opcode & 0x0F00) >> 8]) {
				pc += 2;
			}
			break;

		// EXA1: Skip next opcode if key with the value of VX is not pressed
		case 0x0000:
			// TODO:
			if (!V[(opcode & 0x0F00) >> 8]) {
				pc += 2;
			}
			break;

		default:
			printf("Unknown opcode [0xE000]: 0x%X\n", opcode);
		}
		break;

	case 0xF000:
		switch (opcode & 0x000F)
		{
		// FX07: The value of delay_timer is placed into VX
		case 0x0007:
			V[opcode & 0x0F00 >> 8] = delay_timer;
			pc += 2;
			break;

		// FX0A:
		case 0x000A:
			break;

		case 0x0005:
			switch (opcode & 0x00F0)
			{
			// FX15: Set delay_timer = VX
			case 0x0010:
				delay_timer = V[opcode & 0x0F00 >> 8];
				pc += 2;
				break;

			// FX55: Store registers V0 through VX in memory starting at location I
			case 0x0050:
				unsigned int X = (opcode & 0x0F00 >> 8) + 1;
				for (int i = 0; i < X; i++) {
					memory[I + i] = V[i];
				}
				pc += 2;
				break;

			// FX65: 
			case 0x0060:
				unsigned int X = (opcode & 0x0F00 >> 8) + 1;
				for (int i = 0; i < X; i++) {
					V[i] = memory[I + i];
				}
				pc += 2;
				break;

			default:
				printf("Unknown opcode [0x0005]: 0x%X\n", opcode);
			}
			break;

		// FX18: Set sound_timer = VX
		case 0x0008:
			sound_timer = V[opcode & 0x0F00 >> 8];
			pc += 2;
			break;

		// FX1E: Set I = I + Vx
		case 0x000E:
			I = I + V[opcode & 0x0F00 >> 8];
			pc += 2;
			break;

		// FX29: Set I = location of sprite for digit Vx
		case 0x0009:
			// TODO
			pc += 2;
			break;

			// FX33: Store BCD (binary-coded decimal) representation of Vx in memory locations I, I+1, and I+2
		case 0x0003:
			unsigned char VX = V[opcode & 0x0F00 >> 8];
			memory[I] = VX / 100;
			memory[I + 1] = VX % 100 / 10;
			memory[I + 2] = VX % 5;
			pc += 2;
			break;

		default:
			printf("Unknown opcode [0xF000]: 0x%X\n", opcode);
		}
		break;

	// TODO: Add Super Chip-48 instructions?

	default:
		printf("Unknown opcode: 0x%X\n", opcode);
	}

	// Update timers
	if (delay_timer > 0) {
		--delay_timer;
	}

	if (sound_timer > 0) {
		printf("SOUND\n");  // TODO : Make a sound
		--sound_timer;
	}
}

void Chip8::setKeys() {
	// TODO
}