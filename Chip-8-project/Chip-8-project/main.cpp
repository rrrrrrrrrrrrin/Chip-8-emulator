#include "chip8.h"
#include <cstdio>  // for printf
#include <fstream>
#include <SDL_audio.h>

bool openROM(int argc, char* argv[]);  // Read file into the buffer

bool loadSound();
bool initSDL();  // Start SDL (video, audio)
void gfxUpdate();
void close();  // Free resources and close SDL

// Original Chip-8's resolution
#define SCREEN_WIDTH 64
#define SCREEN_HEIGHT 32

// Parameters of width and height for SDL window
#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 320

// Global SDL variables
SDL_Window* window = NULL;
SDL_Texture* texture = NULL;
SDL_Renderer* renderer = NULL;

// Global SDL_audio variables
SDL_AudioStream* stream = NULL;
Uint8* audio_buf = NULL;
Uint32 audio_len = 0;

#define DELAY 1  // For delay of each gfx update frame

Chip8 chip8;

int main(int argc, char* argv[])
{
	if (!openROM(argc, argv))
	{
		printf("Failed to open ROM\n");
		return 1;
	}

	if (!initSDL())
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

		if (chip8.sound_flag) {
			// Check if the audio stream needs more data
			if (SDL_GetAudioStreamQueued(stream) < static_cast<int>(audio_len))
			{
				// Add data to the audio stream
				SDL_PutAudioStreamData(stream, audio_buf, audio_len);
			}

			// SDL_OpenAudioDeviceStream starts the device paused. Start playback of the audio device associated with the stream
			if (!SDL_ResumeAudioStreamDevice(stream)) 
			{
				printf("Couldn't resume audio device: %s\n", SDL_GetError());
				SDL_QuitSubSystem(SDL_INIT_AUDIO);
			}

			// Pause audio playback 
			if (!SDL_PauseAudioStreamDevice(stream))
			{
				printf("Couldn't pause audio device: %s\n", SDL_GetError());
				SDL_QuitSubSystem(SDL_INIT_AUDIO);
			}
		}

		SDL_PumpEvents();  // Update the event queue and internal input device state
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

bool loadSound()
{
	bool success = true;

	SDL_AudioSpec spec;

	// Load the .wav file
	if (!SDL_LoadWAV("sound.wav", &spec, &audio_buf, &audio_len))
	{
		printf("Couldn't load .wav file: %s\n", SDL_GetError());
		SDL_QuitSubSystem(SDL_INIT_AUDIO);
		success = false;
	}

	// Create audio stream in the same format as the .wav file
	stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, NULL, NULL);
	if (!stream)
	{
		printf("Couldn't create audio stream: %s\n", SDL_GetError());
		SDL_QuitSubSystem(SDL_INIT_AUDIO);
		success = false;
	}

	return success;
}

bool initSDL()
{
	bool success = true;

	// Initialize SDL
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) == false)
	{
		printf("Couldn't initialize SDL: %s\n", SDL_GetError());
		success = false;
	}

	loadSound();

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

	// Set the texture's scale mode to the nearest neighbor value interpolation method
	bool scale_mode = SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);
	if (!scale_mode) {
		printf("Couldn't set a texture scale mode: %s\n", SDL_GetError());
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

	int pitch = SCREEN_WIDTH * sizeof(unsigned int);  // Pitch: width * bytes per pixel

	// Update SDL texture: take the pixel data from the pixels array and copy it to the texture's video memory
	SDL_UpdateTexture(texture, NULL, pixels, pitch);

	SDL_RenderClear(renderer);

	// Stretch Chip-8's 64x32 texture to fill the WINDOW_WIDTH x WINDOW_HEIGHT window
	SDL_FRect destRect = { 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT };
	SDL_RenderTexture(renderer, texture, NULL, &destRect);

	// Present the renderer on the screen
	SDL_RenderPresent(renderer);

	SDL_Delay(DELAY);

	/*
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
	*/
}

void close()
{
	// Destroy SDL variables 
	SDL_DestroyWindow(window);
	SDL_free(window);
	SDL_DestroyRenderer(renderer);
	SDL_free(renderer);
	SDL_DestroyTexture(texture);
	SDL_free(texture);

	// Destroy SDL_audio variables
	SDL_free(audio_buf);
	SDL_DestroyAudioStream(stream);

	// Quit SDL subsystems
	SDL_Quit();
	SDL_QuitSubSystem(SDL_INIT_AUDIO);
}