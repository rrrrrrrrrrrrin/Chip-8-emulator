#pragma once
#include <SDL3\SDL.h>

// Global SDL_audio vars
SDL_AudioStream* stream8 = NULL;
Uint8* audio_buf8 = NULL;
Uint32 audio_len8 = 0;

bool loadSound8()
{
	bool success = true;

	SDL_AudioSpec spec;

	// Load the .wav file
	if (!SDL_LoadWAV("assets/sound.wav", &spec, &audio_buf8, &audio_len8))
	{
		printf("Couldn't load .wav file: %s\n", SDL_GetError());
		SDL_QuitSubSystem(SDL_INIT_AUDIO);
		success = false;
	}

	// Create audio stream in the same format as the .wav file
	stream8 = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, NULL, NULL);
	if (!stream8)
	{
		printf("Couldn't create audio stream: %s\n", SDL_GetError());
		SDL_QuitSubSystem(SDL_INIT_AUDIO);
		success = false;
	}

	return success;
}

void playSound8()
{
	// Check if the audio stream needs more data
	if (SDL_GetAudioStreamQueued(stream8) < static_cast<int>(audio_len8))
	{
		// Add data to the audio stream
		SDL_PutAudioStreamData(stream8, audio_buf8, audio_len8);
	}

	// SDL_OpenAudioDeviceStream starts the device paused. Start playback of the audio device associated with the stream
	if (!SDL_ResumeAudioStreamDevice(stream8))
	{
		printf("Couldn't resume audio device: %s\n", SDL_GetError());
		SDL_QuitSubSystem(SDL_INIT_AUDIO);
	}
}

void pauseSound8()
{
	// Pause audio playback 
	if (!SDL_PauseAudioStreamDevice(stream8))
	{
		printf("Couldn't pause audio device: %s\n", SDL_GetError());
		SDL_QuitSubSystem(SDL_INIT_AUDIO);
	}
}