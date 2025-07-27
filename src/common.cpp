#include "common.h"

#include <SDL3/SDL.h>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <windows.h>

void buzzer(int frequency, int duration) { Beep(frequency, duration); }

uint64_t uptime_ns() { return SDL_GetTicksNS(); }
uint64_t uptime_ms() { return SDL_GetTicks(); }

void delay_ms(uint32_t ms) { SDL_Delay(ms); }
void delay_ns(uint32_t ns) { SDL_Delay(ns); }

bool load_file(const char* filename, byte* buffer, int* len)
{
	std::ifstream file(filename, std::ios::binary | std::ios::ate);

	if (!file.is_open())
	{
		std::printf("Error: Could not open file %s: %s\n", filename, strerror(errno));
		return false;
	}

	size_t file_size = file.tellg();
	file.seekg(0, std::ios::beg);

	if (file_size > static_cast<size_t>(*len))
	{
		std::printf("Error: Buffer too small (%d bytes), need %zu bytes\n", *len, file_size);
		return false;
	}

	file.read(reinterpret_cast<char*>(buffer), file_size);

	if (!file)
	{
		std::printf("Error: Only %zd bytes could be read\n", file.gcount());
		return false;
	}

	*len = file.gcount();
	return true;
}