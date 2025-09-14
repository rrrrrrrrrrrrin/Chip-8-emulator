#include "chip8.h"
#include <iostream>

// Methods
void Chip8::initialize() 
{
	pc = 0x200;  // programs start at location 0x200 (512)
	opcode = 0;
	I = 0;
	sp = 0;

	// Clear display 
	for (int i = 0; i < 64 * 32; i++) {
		gfx[i] = 0;
	}

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

void Chip8::loadGame(int buffer_size, char* buffer)
{
	for (int i = 0; i < buffer_size; i++) {
		memory[i + 512] = buffer[i];
	}
}

void Chip8::emulateCycle() {
	// Fetch opcode
	opcode = memory[pc] << 8 | memory[pc + 1];  // fetch two bytes from memory and merge into one opcode

	// Decode opcode
	switch (opcode & 0xF000)  // check opcode via rightmost nibble 
	{  
		
	// ANNN: Sets I to the address NNN
	case 0xA000: 
		I = opcode & 0x000F;
		pc += 2;  // opcode is 2 bytes. Move program counter two cells in the memory (one cell - one byte)
	
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