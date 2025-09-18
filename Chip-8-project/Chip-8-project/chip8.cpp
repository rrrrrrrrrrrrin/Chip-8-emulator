#include "chip8.h"
#include <cstdio>  // for printf

void Chip8::clear_display() 
{
	for (int i = 0; i < 64*32; i++) {
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
	// for the sprites to wrap: x % 64 (display - 64x32) or x & 63
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
	switch (opcode & 0xF000)  // check opcode via rightmost nibble 
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
		if ( (V[(opcode & 0x0F00) >> 8]) == (opcode & 0x00FF) ) {
			pc += 2;
		}
		break;

	// 4XNN: Skips the next opcode if VX equals NN
	case 0x4000:
		if ( (V[(opcode & 0x0F00) >> 8]) != (opcode & 0x00FF) ) {
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

	// ANNN: Sets I to the address NNN
	case 0xA000:
		I = opcode & 0x0FFF;
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
