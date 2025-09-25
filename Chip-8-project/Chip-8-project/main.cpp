#include "chip8.h"
#include <cstdio>  // for printf
#include <fstream>
#include <SDL.h>

bool openROM(int argc, char* argv[]);  // Read file into the buffer

bool init();  // Start SDL and create a window
void gfxUpdate();
void close();  // Free resources and close SDL

// Original Chip-8's resolution
#define SCREEN_WIDTH 64
#define SCREEN_HEIGHT 32


// Parameters of width and height for SDL window
#define WINDOW_WIDTH 1080
#define WINDOW_HEIGHT 720

// Global SDL variables
SDL_Window* window = NULL;
SDL_Surface* surface = NULL;  // Surface directly represents Chip-8 display
SDL_Texture* texture = NULL;
SDL_Renderer* renderer = NULL;

Chip8 chip8;

int main(int argc, char* argv[])
{
	if (!openROM(argc, argv))
	{
		printf("Failed to open ROM\n");
		return 1;
	}

	if (!init())
	{
		printf("Failed to initialize\n");
		return 2;
	}

	// Emulation loop
	for (;;)
	{
		// Emulate one cycle
		chip8.emulateCycle();

		// Update the screen if the draw_flag is true
		if (chip8.draw_flag) {
			gfxUpdate();
		}

		// Store key press state (Press and Release)
		chip8.setKeys();
	}

	close();

	return 0;
}

bool openROM(int argc, char* argv[])
{
	bool success = true;

	if (argc != 2) {
		printf("%s%s%s\n", "Usage: ", argv[0], " filename");
		return false;
	}

	// Initialize the Chip8 system and load the game into the memory
	chip8.initialize();

	// Read the program (argv) in binary mode, parse to buffer
	char* filename = argv[1];
	printf("%s\n", filename);
	std::ifstream file(argv[1], std::ios_base::binary);
	if (!file.is_open()) {
		printf("Couldn't open game file");
		return false;
	}

	file.seekg(0, std::ios::end);
	int file_size = static_cast<int>(file.tellg());
	file.seekg(0, std::ios::beg);

	std::vector<char> buffer(file_size);
	file.read(buffer.data(), file_size);

	file.close();

	chip8.loadROM(file_size, buffer);

	return true;
}

bool init()
{
	bool success = true;

	// Initialize SDL
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		printf("Couldn't initialize SDL: %s\n", SDL_GetError());
		success = false;
	}

	// Create SDL window
	window = SDL_CreateWindow("Rin's Chip-8 Emu", WINDOW_WIDTH, WINDOW_HEIGHT, NULL);
	if (!window)
	{
		printf("Couldn't create a window: %s\n", SDL_GetError());
		SDL_Quit();  // Clean up SDL
		success = false;
	}

	// Create SDL renderer
	renderer = SDL_CreateRenderer(window, NULL);
	if (!renderer)
	{
		printf("Couldn't create a renderer: %s\n", SDL_GetError());
		SDL_Quit();
		success = false;
	}

	// Create SDL texture: texture will be updated with contents of surface and then rendered to the screen
	texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING, SCREEN_WIDTH, SCREEN_HEIGHT);
	if (!texture)
	{
		printf("Couldn't create a texture: %s\n", SDL_GetError());
		SDL_Quit();
		success = false;
	}

	return success;
}

void gfxUpdate()
{
	// Buffer for converted pixels
	unsigned int pixels[SCREEN_WIDTH * SCREEN_HEIGHT] = { 0 };
	for (int px = 0; px < SCREEN_WIDTH * SCREEN_HEIGHT; px++) {
		if (chip8.gfx[px] == 1)
		{
			pixels[px] = 0xFFFFFFFF;  // White pixel 
		}
		else
		{
			pixels[px] = 0xFF000000;  // Black pixel
		}
	}

	// Update SDL texture: take the pixel data from the pixels array and copy it to the texture's video memory
	int pitch = SCREEN_WIDTH * sizeof(unsigned int);  // Pitch is width * bytes per pixel
	SDL_UpdateTexture(texture, NULL, pixels, pitch);

	SDL_RenderClear(renderer);

	// Copy the texture to the renderer's buffer
	SDL_RenderTexture(renderer, texture, NULL, NULL);

	// Present the renderer on the screen
	SDL_RenderPresent(renderer);

	// Leave the window on the screen
	bool quit = false;
	SDL_Event event;
	while (!quit)
	{
		while (SDL_PollEvent(&event))
		{
			if (event.type == SDL_EVENT_QUIT) {
				quit = true;
			}
		}
	}
}

void close()
{
	// Destroy SDL variables
	SDL_DestroyWindow(window);
	window = NULL;
	SDL_DestroySurface(surface);
	surface = NULL;
	SDL_DestroyTexture(texture);
	texture = NULL;
	SDL_DestroyRenderer(renderer);
	renderer = NULL;

	// Quit SDL subsystems
	SDL_Quit();
}