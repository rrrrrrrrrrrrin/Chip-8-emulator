#include "chip8.h"
#include <cstdio>  // for printf
#include <fstream>
#include <SDL_audio.h>
#include <SDL_image.h>

bool openROM(int argc, char* argv[]);  // Read file into the buffer
bool openROMSDL();

bool initSDL();  // Start SDL (video, audio)

bool loadSound();
void playSound();

void updateRenderer(SDL_Texture* new_texture);

bool loadIntro();
void displayIntro();

bool initSDLtexture();

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
SDL_Texture* intro = NULL;
SDL_Texture* texture = NULL;
SDL_Renderer* renderer = NULL;

// Global SDL_image vars
int alpha = 255;
#define DELAY 200  // 1000 milliseconds is 1 second

// Global SDL_audio vars
SDL_AudioStream* stream = NULL;
Uint8* audio_buf = NULL;
Uint32 audio_len = 0;

Chip8 chip8;

// SDL_dialog vars and functions in the global space
SDL_IOStream* SDL_file;

// Set up callback used by file dialog functions
static const SDL_DialogFileFilter filters[] = 
{
	{ "CH8 files", "ch8" }
};

static void SDLCALL callback(void* userdata, const char* const* filelist, int filter)
{
	if (!filelist) {
		SDL_Log("An error occured: %s", SDL_GetError());
		return;
	}
	else if (!*filelist) {
		SDL_Log("The user did not select any file.");
		SDL_Log("Most likely, the dialog was canceled.");
		return;
	}

	while (*filelist) {
		SDL_Log("Full path to selected file: '%s'", *filelist);

		SDL_file = SDL_IOFromFile(*filelist, "rb");  // Open ROM for reading in binary mode
		if (SDL_file == NULL)
		{
			printf("Couldn't open SDL file %s%s\n", *filelist, SDL_GetError());
		}

		filelist++;
		return;
	}
}

bool openROMSDL()
{
	// Initialize the Chip8 system and load the game into the memory
	chip8.initialize();

	// Parse ROM to buffer
	SDL_SeekIO(SDL_file, 0, SDL_IO_SEEK_END);
	size_t SDL_file_size = static_cast<size_t>(SDL_TellIO(SDL_file));
	SDL_SeekIO(SDL_file, 0, SDL_IO_SEEK_SET);

	std::vector<char> SDL_buffer(SDL_file_size);
	size_t SDL_read = SDL_ReadIO(SDL_file, SDL_buffer.data(), SDL_file_size);  // .data() for std::vector returns a pointer to memory array used by vector

	if (SDL_GetIOStatus(SDL_file) != SDL_IO_STATUS_EOF)
	{
		printf("The SDL_file data stream is completely read if zero: %Iu", SDL_read);  // Iu is for size_t
		printf("Couldn't read data into SDL_buffer %s\n", SDL_GetError());
		return false;
	}

	if (!SDL_CloseIO(SDL_file))
	{
		printf("Couldn't read the file %s\n", SDL_GetError());
		return false;
	}

	chip8.loadROM(SDL_file_size, SDL_buffer);

	return true;
}

int main(int argc, char* argv[])
{
	if (!initSDL())
	{
		printf("Failed to initialize\n");
		return 1;
	}

	if (!loadIntro())
	{
		printf("Failed to load intro\n");
		return 2;
	}

	displayIntro();

	if (!initSDLtexture())
	{
		printf("Failed to load texture\n");
		return 3;
	}

	bool quit = false;
	bool back = false;
	bool playGame = false;
	bool emulationStart = false;

	// Application is running
	while (true)
	{
		// Process the event queue once every frame BEFORE updating the game's state
		// (while SDL_PollEvent loop closes before the emulation of the cycle and the bool values can be checked outside of the event loop)
		bool quit = false;
		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			if (event.type == SDL_EVENT_QUIT) {
				quit = true;
			}

			// TO-DO: set up a menu (CHIP-8 Emulator Load file with L (New image if the file is loaded into the memory,
			// same (another file can be reloaded) plus) Play game with G (starts an emulation loop) )
			if ((event.type == SDL_EVENT_KEY_DOWN) || back)
			{
				if (event.key.scancode == SDL_SCANCODE_L)
				{
					// Displays a dialog that lets the user select a file on their filesystem
					SDL_ShowOpenFileDialog(callback, NULL, window, filters, SDL_arraysize(filters), NULL, false);

					// TO-DO: Load ROM into chip8 memory (in callback?) and display a new image Play game with G

					/*if (!openROM(argc, argv))
					{
						printf("Failed to open ROM\n");
						return 4;
					}*/

					playGame = true;
					back = false;
				}
			}

			if (event.type == SDL_EVENT_KEY_DOWN && playGame)
			{
				if (event.key.scancode == SDL_SCANCODE_G)
				{
					openROMSDL();

					emulationStart = true;
					playGame = false;
				}
			}

			if (event.type == SDL_EVENT_KEY_DOWN)
			{
				if (event.key.scancode == SDL_SCANCODE_ESCAPE)
				{
					back = true; 
				}
			}
		}

		if (quit) { break; }  // Stop application loop

		// Stop emulation by initializing chip8 object and return to the menu 
		if (back)
		{
			emulationStart = false;
			chip8.initialize();
		}

		if (emulationStart)
		{
			// Emulate one cycle
			chip8.emulateCycle();

			// Update the screen if the draw_flag is true
			if (chip8.draw_flag) {
				gfxUpdate();
			}

			if (chip8.sound_flag) {
				playSound();
			}

			SDL_PumpEvents();  // Update the event queue and internal input device state
		}
	}

	close();

	return 0;
}

bool openROM(int argc, char* argv[])
{
	if (argc != 2) 
	{
		printf("%s%s%s\n", "Usage: ", argv[0], " filename");
		return false;
	}

	// Initialize the Chip8 system and load the game into the memory
	chip8.initialize();

	// Read the program (argv) in binary mode, parse to buffer
	char* filename = argv[1];
	printf("%s\n", filename);
	std::ifstream file(filename, std::ios_base::binary);

	if (!file.is_open()) 
	{
		printf("Couldn't open file %s", filename);
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

bool initSDL()
{
	bool success = true;

	// Initialize SDL
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) == false)
	{
		printf("Couldn't initialize SDL: %s\n", SDL_GetError());
		success = false;
	}

	if (!loadSound()) { success = false; }

	// Create SDL window
	window = SDL_CreateWindow("Rin's Chip-8 Emulator", WINDOW_WIDTH, WINDOW_HEIGHT, NULL);
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

	return success;
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

void playSound()
{
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

void updateRenderer(SDL_Texture* new_texture)
{
	SDL_RenderClear(renderer);
	SDL_RenderTexture(renderer, new_texture, NULL, NULL);
	SDL_RenderPresent(renderer);
}

bool loadIntro()
{
	// Load an image into a texture
	intro = IMG_LoadTexture(renderer, "intro.jpg");
	if (intro == NULL)
	{
		printf("Couldn't load an intro: %s\n", SDL_GetError());
		SDL_Quit();
		return false;
	}

	if (!SDL_SetTextureAlphaMod(intro, static_cast<Uint8>(alpha)))
	{
		printf("Couldn't set an alpha value to intro: %s\n", SDL_GetError());
		SDL_Quit();
		return false;
	}

	if (!SDL_SetTextureBlendMode(intro, SDL_BLENDMODE_BLEND))
	{
		printf("Couldn't set blend mode to intro: %s\n", SDL_GetError());
		SDL_Quit();
		return false;
	}

	return true;
}

void displayIntro()
{
	while (true)
	{
		SDL_SetTextureAlphaMod(intro, static_cast<int>(alpha));

		// Update renderer with a new texture
		updateRenderer(intro);

		if (alpha == 255) { SDL_Delay(DELAY); }

		alpha -= 17;

		SDL_Delay(DELAY);

		if (alpha <= 0) { break; }
	}

	SDL_DestroyTexture(intro);
	intro = NULL;
}

bool initSDLtexture()
{
	bool success = true;

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

	// Update renderer with a new texture
	updateRenderer(texture);

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
}

void close()
{
	// Destroy SDL variables
	SDL_DestroyTexture(texture);
	texture = NULL;
	SDL_DestroyRenderer(renderer);
	renderer = NULL; 
	SDL_DestroyWindow(window);
	window = NULL;

	// Destroy SDL_audio variables
	audio_buf = NULL;
	SDL_DestroyAudioStream(stream);

	// Quit SDL subsystems
	SDL_Quit();
	SDL_QuitSubSystem(SDL_INIT_AUDIO);
}