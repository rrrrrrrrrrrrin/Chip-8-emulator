#include "chip8.h"
#include <fstream>

Chip8 chip8;

int main(int argc, char* argv[]) 
{
	// TODO : Set up the display (gfx) and register input callbacks

	// Initialize the Chip8 system and load the game into the memory
	chip8.initialize();

	// TODO : Read the program (argv) in binary mode, parse to buffer
	char* buffer = 0;
	int buffer_size = 0;
	chip8.loadGame(buffer_size, buffer);

	// Emulation loop
	for (;;) 
	{
		// Emulate one cycle
		chip8.emulateCycle();

		// TODO : If the draw flag is set, update the screen


		// Store key press state (Press and Release)
		chip8.setKeys();
	}

	return 0;
}
 