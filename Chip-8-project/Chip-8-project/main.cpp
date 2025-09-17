#include "chip8.h"
#include <cstdio>  // for printf
#include <fstream>
#include <SDL.h>

const int scale = 20;
#define SCREEN_WIDTH 64
#define SCREEN_HEIGHT 32

bool init();  // Start SDL and create a window
void drawScreen();
void close();  // Free resources and close SDL

// Global SDL variables
SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;

Chip8 chip8;

int main(int argc, char* argv[]) 
{
	if (argc != 2) {
		printf("%s%s%s\n", "Usage: ", argv[0], " filename");
		return 1;
	}

	// Initialize the Chip8 system and load the game into the memory
	chip8.initialize();

	// TODO : Read the program (argv) in binary mode, parse to buffer
	char* filename = argv[1];
	printf("%s\n", filename);
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

	if (!init()) 
	{
		printf("Failed to initialize\n");
		return 3;
	}

	drawScreen();

	//// Emulation loop
	//for (;;) 
	//{
	//	// Emulate one cycle
	//	chip8.emulateCycle();

	//	// TODO : If the draw flag is set, update the screen
	//	if (chip8.draw_flag) {
	//		/*if (!gfxUpdate()) 
	//		{
	//			printf("Failed to update the screen\n");
	//			return 4;
	//		}
	//		gfxUpdate();*/
	//	}

	//	// Store key press state (Press and Release)
	//	chip8.setKeys();
	//}

	close();

	return 0;
}
 
bool init() {
	bool success = true;

	// Initialize SDL
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		printf("Couldn't initialize SDL: %s\n", SDL_GetError());
		success = false;
	}

	// Create SDL window
	window = SDL_CreateWindow("Rin's Chip-8 Emu", SCREEN_WIDTH*scale, SCREEN_HEIGHT*scale, 0);
	if (!window)
	{
		printf("Couldn't create a window: %s\n", SDL_GetError());
		SDL_Quit();  // Clean up SDL
		success = false;
	}

	return success;
}

void drawScreen() {
	bool quit = false;
	SDL_Event e;
	while (!quit)
	{
		while ( SDL_PollEvent(&e) )
		{
			if (e.type == SDL_EVENT_QUIT) {
				quit = true;
			}
		}
	}

	// Create SDL renderer
	renderer = SDL_CreateRenderer(window, 0);
	SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 255);
	SDL_RenderClear(renderer);
	SDL_RenderPresent(renderer);
}

void close() {
	// Destroy window
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	window = NULL;

	// Quit SDL subsystems
	SDL_Quit();
}