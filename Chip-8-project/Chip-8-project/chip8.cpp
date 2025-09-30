#include "chip8.h"
#include <cstdio>  // for printf
#include <cstring>  // for memset
#include <SDL_scancode.h>

void Chip8::clear_display()
{
	std::memset(gfx, 0, sizeof(gfx));
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
	std::memset(memory, 0, sizeof(memory));

	// Clear registers V0 - VF
	std::memset(V, 0, sizeof(V));

	// Clear stack
	std::memset(stack, 0, sizeof(stack));

	// Clear keys state
	std::memset(keys, 0, sizeof(keys));

	// Load the fontset (group of sprites representing 0-F stored in memory to 0x50)
	for (int i = 0; i < 80; i++) {
		memory[i] = fontset[i];
	}

	// Reset timers
	delay_timer = 60;
	sound_timer = 60;

	draw_flag = false;
}

void Chip8::update_timers() {
	if (delay_timer > 0) {
		--delay_timer;
	}

	if (sound_timer > 0) {
		printf("SOUND\n");  // TODO : Make a sound
		--sound_timer;
	}
}

void Chip8::loadROM(int file_size, std::vector<char> buffer)
{
	for (int i = 0; i < file_size; i++) {
		memory[i + 512] = buffer[i];  // Start filling the memory at location 0x200
	}
}

void Chip8::draw(unsigned int X, unsigned int Y, char N)
{
	// for the sprites to wrap: VX % 64 (display - 64x32) or VX & 63
	char VX = V[X] & 63;
	char VY = V[Y] & 31;

	for (int heightpx = 0; heightpx < N; heightpx++)
	{
		// Read N bytes starting from I
		unsigned short byte = memory[I + heightpx];

		// Process each byte
		for (int widthpx = 0; widthpx < 8; widthpx++)
		{
			if ((byte & (0x80 >> widthpx)) != 0)  // data & (0x80 >> widthpx) is to parse byte by bits from left to right
			{
				if (gfx[VX + widthpx + ((VY + heightpx) * 64)] == 1) {
					V[0xF] = 1;  // VF is 1 if gfx pxs are flipped from set to unset; collision occured
				}
				gfx[VX + widthpx + ((VY + heightpx) * 64)] ^= 1;
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

	pc += 2;  // opcode is 2 bytes. Move program counter two cells in the memory (one cell - one byte)

	decodeOpcodes();

	unsigned int X = (opcode & 0x0F00) >> 8; // move VX (opcode & 0x0F00) to the last nibble left to right
	unsigned int Y = (opcode & 0x00F0) >> 4; // move VY (opcode & 0x00F0) to the last nibble left to right

	// Decode opcode
	switch (opcode & 0xF000)  // check opcode through leftmost nibble (most significant nibble)
	{
	case 0x0000:
		switch (opcode & 0x000F)
		{
		// 00E0: Clear display
		case 0x0000:
			clear_display();
			break;

		// 00EE: Returns from a subroutine
		case 0x000E:
			--sp;
			pc = stack[sp];
			break;

		default:
			printf("Unknown opcode [0x0000]: 0x%X\n", opcode);
		}
		break;

	// 1NNN: Jumps to address NNN
	case 0x1000:
		pc = opcode & 0x0FFF;
		break;

	//2NNN: Call subroutine at nnn
	case 0x2000:
		stack[sp] = pc;
		++sp;
		pc = opcode & 0x0FFF;
		break;

	// 3XNN: Skips the next opcode if VX equals NN
	case 0x3000:
		if (V[X] == (opcode & 0x00FF)) {
			pc += 2;
		}
		break;
		
	// 4XNN: Skips the next opcode if VX doesn't equal NN
	case 0x4000:
		if (V[X] != (opcode & 0x00FF)) {
			pc += 2;
		}
		break;
	
	// 5XY0: Skips the next opcode if VX equals VY
	case 0x5000:
		if (V[X] == V[Y]) {
			pc += 2;
		}
		break;

	// 6XNN: Sets VX to NN
	case 0x6000:
		V[X] = opcode & 0x00FF;
		break;

	// 7XNN: Adds NN to VX
	case 0x7000:
		V[X] += opcode & 0x00FF;
		break;

	case 0x8000:
		switch (opcode & 0x000F)
		{
		// 8XY0: Stores the value of register VY in register VX
		case 0x0000:
			V[X] = V[Y];
			break;

		// 8XY1: Set VX = VX OR VY
		case 0x0001:
			V[X] |= V[Y];
			break;

		// 8XY2: Set VX = VX AND VY
		case 0x0002:
			V[X] &= V[Y];
			break;

		// 8XY3:  Set VX = VX XOR VY
		case 0x0003:
			V[X] ^= V[Y];
			break;

		// 8XY4: Set VX = VX + VY, set VF = carry
		case 0x0004:
		{
			unsigned short sum = V[X] + V[Y];
			V[X] = sum & 0x00FF;  // Only the lowest (rightmost) 8 bits are stored in VX (if sum is greater than 8 bits)

			// If the result is greater than 8 bits (size of char) (> 255), VF is set to 1
			if (sum > 255)
			{
				V[0xF] = 1;
			}
			break;
		}

		// 8XY5: Set Vx = Vx - Vy, set VF to 1 if VX > VY (no borrow in subtraction)
		case 0x0005:
			V[X] = V[X] - V[Y];

			if (V[X] > V[Y])
			{
				V[0xF] = 1;
			}
			break;

		// 8XY6: Set VX = VX SHR 1 (VX / 2), set VF to 1 if LSBit is 1 (SHR is shift right bitwise operator >>)
		case 0x0006:
			V[X] >>= 1;

			if ((V[X] & 0x0000000F) == 1) {
				V[0xF] = 1;
			}
			break;

		// 8XY7: Set VX = VY - VX, set VF to 1 if VY > VX (no borrow in subtraction)
		case 0x0007:
			V[X] = V[Y] - V[X];

			if (V[Y] > V[X])
			{
				V[0xF] = 1;
			}
			break;

		// 8XYE: Set VX = VX SHL 1, set VF to 1 if MSBit is 1
		case 0x000E:
			V[X] <<= 1;

			if ((V[X] & 0xF0000000) == 1) {
				V[0xF] = 1;
			}
			break;

		default:
			printf("Unknown opcode [0x8000]: 0x%X\n", opcode);
		}
		break;

	// 9XY0: Skip next instruction if VX != VY
	case 0x9000:
		if (V[X] != V[Y]) {
			pc += 2;
		}
		break;

	// ANNN: Sets I to the address NNN
	case 0xA000:
		I = opcode & 0x0FFF;
		break;

	// BNNN: Jump to location NNN + V0
	case 0xB000:
		pc = opcode & 0x0FFF + V[0];
		break;

	// CXNN: Set VX = random byte (random number from 0 to 255) AND NN
	case 0xC000:
		V[X] = rand() % 256 & opcode & 0x00FF;
		break;

	// DXYN: Draws a sprite at coordinate (VX, VY), width - 8 pxs, height - N pxs
	case 0xD000:
		draw(X, Y, opcode & 0x000F);
		break;

	case 0xE000:
		switch (opcode & 0x000F)
		{
		// EX9E: Skip next opcode if key with the value of VX is pressed
		case 0x000E:
			if (keys[V[X]] == 1) {
				pc += 2;
			}
			break;

		// EXA1: Skip next opcode if key with the value of VX is not pressed
		case 0x0001: 
			if (keys[V[X]] != 1) {
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
			V[X] = delay_timer;
			break;

		// FX0A: Wait for a key press, store the value of the key in VX
		case 0x000A:
		{
			unsigned char key = 'K';
			while (key == 'K')
			{
				for (int i = 0; i <= 0xF; i++) {
					if (keys[i] == 1) {
						key = keys[i];
						V[X] = i;
						break;
					}
					pc -= 2;
					update_timers();
				}
			}
			break;
		}

		case 0x0005:
			switch (opcode & 0x00F0)
			{
			// FX15: Set delay_timer = VX
			case 0x0010:
				delay_timer = V[X];
				break;

			// FX55: Store registers V0 through VX in memory starting at location I
			case 0x0050:
				for (size_t i = 0; i <= X; i++) {
					memory[I + i] = V[i];
				}
				break;

			// FX65: Read registers V0 through Vx from memory starting at location I
			case 0x0060:
				for (size_t i = 0; i <= X; i++) {
					V[i] = memory[I + i];
				}
				break;

			default:
				printf("Unknown opcode [0x0005]: 0x%X\n", opcode);
			}
			break;

		// FX18: Set sound_timer = VX
		case 0x0008:
			sound_timer = V[X];
			break;

		// FX1E: Set I = I + VX
		case 0x000E:
			I = I + V[X];
			break;

		// FX29: Set I = location of sprite (font character) for digit VX
		case 0x0009:
			// VX sprite was written to memory during initialization, bcs fontset was saved in memory starting at location 80. Each sprite consists of 5 bytes (only consider the lowest nibble). Just point I to the right sprite
			I = 80 + (5 * V[X]);
			break;

		// FX33: Store BCD (binary-coded decimal) representation of VX in memory locations I, I+1, and I+2
		case 0x0003:
		{
			unsigned char VX = V[X];
			memory[I] = VX / 100;
			memory[I + 1] = VX % 100 / 10;
			memory[I + 2] = VX % 5;
			break;
		}

		default:
			printf("Unknown opcode [0xF000]: 0x%X\n", opcode);
		}
		break;

	default:
		printf("Unknown opcode: 0x%X\n", opcode);
	}

	update_timers();
}

void Chip8::setKeys(const bool* keysSDL)
{
	/* 
	COSMAC VIP's Chip-8   Customary modern PC's 
	keyboard layout:	  Chip-8 keyboard layout:
	1 2 3 C				  1 2 3 4
	4 5 6 D			      Q W E R
	7 8 9 E               A S D F
	A 0 B F               Z X C V
	*/

	if (keysSDL[SDL_SCANCODE_1]) { keys[1] = 1; } else 
	if (keysSDL[SDL_SCANCODE_2]) { keys[2] = 1; } else
	if (keysSDL[SDL_SCANCODE_3]) { keys[3] = 1; } else
	if (keysSDL[SDL_SCANCODE_4]) { keys[0xC] = 1; } else

	if (keysSDL[SDL_SCANCODE_Q]) { keys[4] = 1; } else
	if (keysSDL[SDL_SCANCODE_W]) { keys[5] = 1; } else
	if (keysSDL[SDL_SCANCODE_E]) { keys[6] = 1; } else
	if (keysSDL[SDL_SCANCODE_R]) { keys[0xD] = 1; } else

	if (keysSDL[SDL_SCANCODE_A]) { keys[7] = 1; } else
	if (keysSDL[SDL_SCANCODE_S]) { keys[8] = 1; } else
	if (keysSDL[SDL_SCANCODE_D]) { keys[9] = 1; } else
	if (keysSDL[SDL_SCANCODE_F]) { keys[0xE] = 1; } else

	if (keysSDL[SDL_SCANCODE_Z]) { keys[0xA] = 1; } else
	if (keysSDL[SDL_SCANCODE_X]) { keys[0] = 1; } else
	if (keysSDL[SDL_SCANCODE_C]) { keys[0xB] = 1; } else
	if (keysSDL[SDL_SCANCODE_V]) { keys[0xF] = 1; }

	else
	{
		printf("Unknown scancode\n");
	}
}