#include "common.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_pixels.h>
#include <SDL3/SDL_rect.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_surface.h>
#include <SDL3/SDL_video.h>
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <winscard.h>

// 16进制键盘布局
// 1	2	3	C
// 4	5	6	D
// 7	8	9	E
// A	0	B	F
constexpr int KeyNum = 16;
constexpr int KeyMap[KeyNum]{
	SDLK_X, SDLK_1, SDLK_2, SDLK_3, //
	SDLK_Q, SDLK_W, SDLK_E, SDLK_A, //
	SDLK_S, SDLK_D, SDLK_Z, SDLK_C, //
	SDLK_4, SDLK_R, SDLK_F, SDLK_V, //
};

constexpr int SCREEN_WIDTH = 64;
constexpr int SCREEN_HEIGHT = 32;

// bool vram[SCREEN_WIDTH * SCREEN_HEIGHT]{};

bool keys_state[KeyNum]{};

SDL_Surface* bg;
SDL_Surface* vram;

SDL_Surface* screen;

void clear_screen() { SDL_BlitSurface(bg, nullptr, vram, nullptr); }
void screen_pixel(uint x, uint y)
{
	constexpr uint8_t FG = 20;
	SDL_WriteSurfacePixel(vram, x, y, FG, FG, FG, 255);
}

bool get_key(byte key_id)
{
	if (key_id >= KeyNum)
	{
		std::printf("error: required invaild key\n");
		return false;
	}
	return keys_state[key_id];
}
bool any_key(byte* key_id)
{
	for (int i = 0; i < 16; i++)
	{
		if (keys_state[i])
		{
			*key_id = i;
			return true;
		}
	}
	*key_id = 0;
	return false;
}

void init_surface(SDL_Window* window)
{
	screen = SDL_GetWindowSurface(window);
	bg = SDL_CreateSurface(SCREEN_WIDTH, SCREEN_HEIGHT, SDL_PIXELFORMAT_RGB24);
	vram = SDL_CreateSurface(SCREEN_WIDTH, SCREEN_HEIGHT, SDL_PIXELFORMAT_RGB24);

	constexpr uint8_t BG0 = 135;
	constexpr uint8_t BG1 = 150;

	bool f = true;
	for (int y = 0; y < SCREEN_HEIGHT; y++)
	{
		for (int x = 0; x < SCREEN_WIDTH; x++)
		{
			f = !f;
			uint8_t a = f ? BG0 : BG1;

			SDL_WriteSurfacePixel(bg, x, y, a, a, a, 255);
		}
		f = !f;
	}
}

void draw(float scale, uint32_t ox, uint32_t oy)
{
	// 清空vram
	SDL_Rect dst{(int)ox, (int)oy, (int)(vram->w * scale), (int)(vram->h * scale)};
	SDL_BlitSurfaceScaled(vram, nullptr, screen, &dst, SDL_SCALEMODE_NEAREST);
}

int main()
{
	SDL_Window* window = nullptr;
	SDL_Renderer* renderer = nullptr;
	SDL_CreateWindowAndRenderer("other chip8 simulator", 800, 600, 0, &window, &renderer);

	init_surface(window);

	start();

	constexpr uint64_t FrameIntervalNs = 1000000 / 700;

	bool quit = false;

	uint64_t frame_delay = 0;
	uint64_t real_frame_delay = 0;
	uint64_t upd_delay = 0;
	uint64_t draw_delay = 0;

	SDL_Event event{};
	while (!quit)
	{
		auto start_time = uptime_ns();

		// 清除按键状态
		// std::memset(keys_state, 0, sizeof(keys_state));

		while (SDL_PollEvent(&event) != 0)
		{
			if (event.type == SDL_EVENT_QUIT)
				quit = true;
			else if (event.type == SDL_EVENT_KEY_UP)
			{
				const int* end = KeyMap + KeyNum;
				auto it = std::find(KeyMap, end, event.key.key);
				if (it != end)
				{
					byte key_code = it - KeyMap;
					// std::printf("key trigger: %X\n", key_code);
					keys_state[key_code] = 0;
				}
			}
			else if (event.type == SDL_EVENT_KEY_DOWN)
			{
				const int* end = KeyMap + KeyNum;
				auto it = std::find(KeyMap, end, event.key.key);
				if (it != end)
				{
					byte key_code = it - KeyMap;
					std::printf("key trigger: %X\n", key_code);
					keys_state[key_code] = 1;
				}
			}
		}

		uint64_t up_st = uptime_ns();
		update();
		upd_delay = uptime_ns() - up_st;

		uint64_t draw_st = uptime_ns();

		// SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		// SDL_RenderClear(renderer);

		// 绘制调试信息
		{
			static char msg_buf[100];

			int fps = (int)(1000000000.0f / frame_delay);
			std::snprintf(msg_buf, sizeof(msg_buf), "fps: %d up: %.4fms, dr: %.4fms", fps,
				(upd_delay / 1000000.0f), (draw_delay / 1000000.0f));

			std::puts(msg_buf);

			// SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
			// SDL_RenderDebugText(renderer, 0, 0, msg_buf);
			// SDL_RenderDebugText(renderer, 0, 10, state_str());
		}

		// SDL_RenderPresent(renderer);

		draw(10, 100, 100);
		SDL_UpdateWindowSurface(window);

		draw_delay = uptime_ns() - draw_st;

		real_frame_delay = uptime_ns() - start_time;
		if (real_frame_delay < FrameIntervalNs)
			SDL_DelayNS(FrameIntervalNs - real_frame_delay);
		frame_delay = uptime_ns() - start_time;
	}

	return 0;
}