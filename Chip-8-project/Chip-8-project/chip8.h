#pragma once
#include <iostream>
#include <vector>
#include <SDL.h>

class Chip8 {
private:
	// CPU components
	unsigned short opcode = 0;  // current opcode

	unsigned char memory[4096] = { 0 };

	unsigned char V[16] = { 0 };  // general purpose registers

	unsigned short I = 0;  // index register
	unsigned short pc = 0;  // program counter

	unsigned short stack[16] = { 0 };
	unsigned short sp = 0;  // stack pointer

	// Built-in font, with sprite data representing the hexadecimal numbers from 0 through F
	// Each font character should be 4 pixels wide and 5 pixels tall
	// These font sprites are drawn just like regular sprites (set the index register I to the sprite’s memory location)
	unsigned char fontset[80] =
	{
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

public:
	unsigned char gfx[64 * 32] = { 0 };  // 64 is screen width, 32 is screen height

	unsigned char delay_timer = 60;
	unsigned char sound_timer = 60;

	void setKey(unsigned int X, int key, SDL_Scancode scancode);
	void setKeys(unsigned int X);

	// For E000 and FX0A opcodes
	unsigned int keypad_last_state[16] = { 0 };
	unsigned int keypad_current_state[16] = { 0 };

	// For DXYN opcode
	bool draw_flag = false;
	void draw(unsigned int X, unsigned int Y, char N);

	void clear_display();
	void initialize();

	void loadROM(size_t SDL_file_size, std::vector<char> SDL_buffer);

	void decodeOpcodes();
	void emulateCycle();  // Fetch, decode, execute opcodes & update timers

	/*
		COSMAC VIP's Chip-8   Customary modern PC's
		keyboard layout:	  Chip-8 keyboard layout:
		1 2 3 C				  1 2 3 4
		4 5 6 D			      Q W E R
		7 8 9 E               A S D F
		A 0 B F               Z X C V
	*/

	// Used in main.cpp when checking keypad input
	SDL_Scancode keys[16] =
	{
		SDL_SCANCODE_X,  // 0
		SDL_SCANCODE_1,  // 1
		SDL_SCANCODE_2,  // 2
		SDL_SCANCODE_3,  // 3
		SDL_SCANCODE_Q,  // 4
		SDL_SCANCODE_W,  // 5
		SDL_SCANCODE_E,  // 6
		SDL_SCANCODE_A,  // 7
		SDL_SCANCODE_S,  // 8
		SDL_SCANCODE_D,  // 9
		SDL_SCANCODE_Z,  // A
		SDL_SCANCODE_C,  // B
		SDL_SCANCODE_4,  // C
		SDL_SCANCODE_R,  // D
		SDL_SCANCODE_F,  // E
		SDL_SCANCODE_V   // F
	};
};
