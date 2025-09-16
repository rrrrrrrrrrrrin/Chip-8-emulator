#include "chip8.h"
#include <iostream>
#include <fstream>

Chip8 chip8;

int main(int argc, char* argv[]) 
{
	if (argc != 2) {
		printf("%s%s%s\n", "Usage: ", argv[0], " filename");
		return 1;
	}

	// TODO : Set up the display (gfx) and register input callbacks

	// Initialize the Chip8 system and load the game into the memory
	chip8.initialize();

	// TODO : Read the program (argv) in binary mode, parse to buffer
	char* filename = argv[1];
	// printf("%s\n", filename);
	std::ifstream file(argv[1], std::ios_base::binary);
	if (!file.is_open()) {
		printf("Couldn't open game file");
		return 2;
	}
	
	file.seekg(0, std::ios::end);
	int file_size = static_cast<int>(file.tellg());
	file.seekg(0, std::ios::beg);

	std::vector<char> buffer(file_size);
	file.read(buffer.data(), file_size);
	
	file.close();

	chip8.loadGame(file_size, buffer);

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
 