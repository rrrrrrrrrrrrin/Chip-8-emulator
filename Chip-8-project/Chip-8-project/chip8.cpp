#include "chip8.h"
#include <iostream>

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
		memory[0] = 0;
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
}

void Chip8::loadGame(int file_size, std::vector<char> buffer)
{
	for (int i = 0; i < file_size; i++) {
		memory[i + 512] = buffer[i];  // Start filling the memory at location 0x200
	}
}

void Chip8::draw(char VX, char VY, char N) {
	char X = V[VX] & 63;
	char Y = V[VY] & 31;
	V[0xF] = 0;
	for (int heightpx = 0; heightpx < N; heightpx++) {
		// Read N bytes starting from I
		unsigned char byte = memory[I + heightpx];
		
		// Process each byte
		for (int widthpx = 0; widthpx < 8; widthpx++) {
			if ((byte & (0x80 >> widthpx)) == 1)  // data & (0x80 >> widthpx) is to parse byte by bits from left to right
			{
				if (gfx[X + Y] == 1) {
					gfx[X + Y] = 0;
					V[0xF] = 1;  // VF is 1 if gfx pxs are flipped from set to unset; collision
				}
				else {
					gfx[X + Y] = 1;
				}
			}
			X++;
		}
		Y++;
	}
}

void Chip8::emulateCycle() {
	// Fetch opcode
	opcode = memory[pc] << 8 | memory[pc + 1];  // fetch two bytes from memory and merge into one opcode
	
	// Decode opcode
	switch (opcode & 0xF000)  // check opcode via rightmost nibble 
	{  
	case 0x0000:
		switch (opcode & 0x000F)
		{
			// 00EE: Returns from a subroutine
		case 0x000E:
			pc = stack[15];
			break;

			// 00E0: Clear display
		case 0x0000:
			clear_display();
			pc += 2;  // opcode is 2 bytes. Move program counter two cells in the memory (one cell - one byte)
			break;

		default:
			printf("Unknown opcode [0x0000]: 0x%X\n", opcode);
		}
	break;

	// ANNN: Sets I to the address NNN
	case 0xA000: 
		I = opcode & 0x0FFF;
		pc += 2;
		break;

	// 1NNN: Jumps to address NNN
	case 0x1000:
		pc += opcode & 0x0FFF;
		break;

	// 6XNN: Sets VX to NN
	case 0x6000:
		V[opcode & 0x0F00] = opcode & 0x00FF;
		pc += 2;
		break;

	// 7XNN: Adds NN to VX
	case 0x7000:
		V[opcode & 0x0F00] += opcode & 0x00FF;
		pc += 2;
		break;

	// DXYN: Draws a sprite at coordiate (VX, VY), width - 8 pxs, height - N pxs
	case 0xD000:
		draw(opcode & 0x0F00, opcode & 0x00F0, opcode & 0x000F);  // for the sprites to wrap: x % 64 (display - 64x32) or x & 63
		pc += 2;
		break;

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