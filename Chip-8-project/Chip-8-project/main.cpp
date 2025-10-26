#include "chip8.h"
#include "sound_chip8.h"
#include <cstdio>  // for printf
#include <fstream>
#include <SDL_image.h>

bool openROMSDL();  // Read file into the buffer and load it into chip8's memory

bool initSDL();  // Start SDL (video, audio)
void close();  // Free resources and close SDL

// Manage intro sound
bool loadSound();
void playSound();
void pauseSound();

// Update renderer with a new texture
void updateRenderer(SDL_Texture* new_texture);

// Functions to show intro and menus in application loop
bool loadIntro();
void displayIntro();

bool loadMenuFile();
void displayMenuFile();

bool loadMenuPlay();
void displayMenuPlay();

// Functions to update the screen during emulation
bool initSDLtexture();
void gfxUpdate();

// Original Chip-8's resolution
#define SCREEN_WIDTH 64
#define SCREEN_HEIGHT 32

// Parameters of width and height for SDL window
#define WINDOW_WIDTH 1024
#define WINDOW_HEIGHT 512

// Global SDL variables
SDL_Window* window = NULL;
SDL_Texture* texture = NULL;
SDL_Renderer* renderer = NULL;

SDL_Texture* intro = NULL;
SDL_Texture* menu_file = NULL;
SDL_Texture* menu_play = NULL;

// Global SDL_image vars
int alpha = 255;
#define DELAY 250  // 1000 milliseconds is 1 second

// Global SDL_audio vars
SDL_AudioStream* stream = NULL;
Uint8* audio_buf = NULL;
Uint32 audio_len = 0;

Chip8 chip8;

// SDL_dialog vars and functions in the global space
SDL_IOStream* SDL_file;
bool openedDialog = false;
bool showMenuPlay = false;

// Set up callback used by file dialog functions
static const SDL_DialogFileFilter filters[] = 
{
	{ "CH8 files", "ch8" }
};

static void SDLCALL callback(void* userdata, const char* const* filelist, int filter)
{
	if (!filelist) {
		SDL_Log("An error occured: %s", SDL_GetError());
		openedDialog = false;
		return;
	}
	else if (!*filelist) {
		SDL_Log("The user did not select any file.");
		SDL_Log("Most likely, the dialog was canceled.");
		openedDialog = false;
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

		openedDialog = false;

		// If file was read and loaded successfully, open menu play
		showMenuPlay = true;

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

	// No bytes are read
	if (SDL_read == 0 && SDL_GetIOStatus(SDL_file) != SDL_IO_STATUS_EOF)
	{
		printf("Couldn't read data into SDL_buffer %s\n", SDL_GetError());
		return false;
	}

	if (!SDL_CloseIO(SDL_file))
	{
		printf("Couldn't read the file %s\n", SDL_GetError());
		return false;
	}

	printf("The SDL_file data stream is completely read: %Iu bytes\n", SDL_read);  // Iu is for size_t
	chip8.loadROM(SDL_file_size, SDL_buffer);

	return true;
}

int main()
{
	if (!initSDL())
	{
		printf("Failed to initialize\n");
		return 1;
	}

	// Load textures
	if (!loadIntro())
	{
		printf("Failed to load intro\n");
		return 2;
	}

	if (!loadMenuFile())
	{
		printf("Failed to load menu file\n");
		return 3;
	}

	if (!loadMenuPlay())
	{
		printf("Failed to load menu play\n");
		return 4;
	}

	// displayIntro();

	if (!initSDLtexture())
	{
		printf("Failed to load texture\n");
		return 5;
	}

	loadSound8();

	bool quit = false;
	bool emulationStart = false;
	bool showMenuFile = true;
	bool playGame = false;

	// Application is running
	while (true)
	{
		// Show menu file
		if (showMenuFile)
		{
			displayMenuFile();
		}

		if (showMenuPlay)
		{
			showMenuFile = false;

			// Show menu play
			displayMenuPlay();

			playGame = true;

			showMenuPlay = false;
		}

		// Process the event queue once every frame BEFORE updating the game's state
		// (while SDL_PollEvent loop closes before the emulation of the cycle and the bool values can be checked outside of the event loop)
		bool quit = false;
		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			if (event.type == SDL_EVENT_QUIT) {
				quit = true;
			}

			// Load file with L
			if ((event.type == SDL_EVENT_KEY_DOWN))
			{
				if (!openedDialog && event.key.scancode == SDL_SCANCODE_L && !emulationStart)
				{
					openedDialog = true;  // Don't let the user open another dialog until they close the current one

					// Displays a dialog that lets the user select a file on their filesystem
					SDL_ShowOpenFileDialog(callback, NULL, window, filters, SDL_arraysize(filters), NULL, false);  
					// => bool openedDialog = false, => void showMenuPlay() function is called
				}

				// Play game with P
				if (event.key.scancode == SDL_SCANCODE_P && playGame)
				{
					openROMSDL();

					playGame = false;

					emulationStart = true;
				}
			}

			// Check key presses for FX0A opcode and make a sound until the key is released
			if ((event.type == SDL_EVENT_KEY_DOWN && emulationStart))
			{
				for (int i = 0; i < 16; ++i)
				{
					SDL_Scancode scancode = chip8.keys[i];

					if (event.key.scancode == scancode)
					{
						playSound8();
					}
				}
			}

			if ((event.type == SDL_EVENT_KEY_UP && emulationStart))
			{
				for (int i = 0; i < 16; ++i)
				{
					SDL_Scancode scancode = chip8.keys[i];

					if (event.key.scancode == scancode)
					{
						pauseSound8();
					}
				}
			}

			if (event.type == SDL_EVENT_KEY_DOWN)
			{
				if (event.key.scancode == SDL_SCANCODE_ESCAPE)
				{
					showMenuFile = true; 
				}
			}
		}

		if (quit) { break; }  // Stop application loop

		// Stop emulation by initializing chip8 object and return to the menu (file)
		if (showMenuFile)
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
		}
	}

	close();

	return 0;
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
	if (!loadSound8()) { success = false; }  // from sound_chip8.h

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
	if (!SDL_LoadWAV("intro.wav", &spec, &audio_buf, &audio_len))
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
}

void pauseSound()
{
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
	intro = IMG_LoadTexture(renderer, "intro.png");
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
		playSound();

		SDL_SetTextureAlphaMod(intro, static_cast<int>(alpha));

		// Update renderer with a new texture
		updateRenderer(intro);

		if (alpha == 255) { SDL_Delay(DELAY*2); }

		alpha -= 17;

		SDL_Delay(DELAY);

		if (alpha <= 0) 
		{
			pauseSound();
			break;
		}
	}

	SDL_DestroyTexture(intro);
	intro = NULL;
}

bool loadMenuFile()
{
	// Load an image into a texture
	menu_file = IMG_LoadTexture(renderer, "menu_file.png");
	if (menu_file == NULL)
	{
		printf("Couldn't load a menu_file: %s\n", SDL_GetError());
		SDL_Quit();
		return false;
	}

	return true;
}

void displayMenuFile()
{
	// Update renderer with a new texture
	updateRenderer(menu_file);
}

bool loadMenuPlay()
{
	// Load an image into a texture
	menu_play = IMG_LoadTexture(renderer, "menu_play.png");
	if (menu_play == NULL)
	{
		printf("Couldn't load a menu_play: %s\n", SDL_GetError());
		SDL_Quit();
		return false;
	}

	return true;
}

void displayMenuPlay()
{
	// Update renderer with a new texture
	updateRenderer(menu_play);
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

	// Destroy textures
	SDL_DestroyTexture(menu_file);
	menu_file = NULL;
	SDL_DestroyTexture(texture);
	texture = NULL;
	SDL_DestroyTexture(menu_play);
	menu_play = NULL;

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