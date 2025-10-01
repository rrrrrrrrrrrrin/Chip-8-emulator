#pragma once
#include <vector>
#include <SDL_scancode.h>

class Chip8 {
private:
	// CPU components
	unsigned short opcode = 0;  // current opcode

	unsigned char memory[4096] = { 0 };

	unsigned char V[16] = { 0 };  // general purpose registers

	unsigned short I = 0;  // index register
	unsigned short pc = 0;  // program counter

	unsigned char delay_timer = 60;
	unsigned char sound_timer = 60;

	unsigned short stack[16] = { 0 };
	unsigned short sp = 0;  // stack pointer

	// Each sprite (font character) is made up of 5 bytes
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

public:
	unsigned char gfx[64 * 32] = { 0 };

	const bool* keysSDL = NULL;

	bool init();

	void setKey(unsigned int X, SDL_Scancode SDL_SCANCODE, unsigned char key);
	void setKeys(unsigned int X);

	// Methods
	void clear_display();
	void initialize();

	void loadROM(int file_size, std::vector<char> buffer);

	bool draw_flag = false;
	void draw(unsigned int X, unsigned int Y, char N);

	void decodeOpcodes();
	void emulateCycle();  // Fetch, decode, execute opcodes & update timers

	void update_timers();
};
