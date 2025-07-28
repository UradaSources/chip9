#include "common.h"

#include <SDL3/SDL.h>
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <winerror.h>
#include <winscard.h>

// 由一个粗糙实现的sdl后端提供支持
// 通过修改common.h中的FILE_PATH来决定运行哪个卡带

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

key_state_t keys_state[KeyNum]{};

SDL_Surface* bg;
SDL_Surface* vram;

SDL_Surface* screen;

bool screen_changed = true;

void clear_screen() { screen_changed = true; }; // { SDL_ClearSurface(vram, 0, 0, 0, 0); }
void screen_pixel(uint x, uint y, bool f)
{
	constexpr uint8_t FG = 20;

	uint8_t c;
	SDL_ReadSurfacePixel(vram, x, y, &c, nullptr, nullptr, nullptr);

	if (c > 0 && !f)
	{
		c = std::min((int)c + 40, 255);
	}
	else
	{
		c = f ? FG : 255;
	}

	SDL_WriteSurfacePixel(vram, x, y, c, c, c, 255 - c);
}

key_state_t get_key(byte key_id)
{
	if (key_id >= KeyNum)
	{
		std::printf("error: required invaild key\n");
		return key_state_t::NONE;
	}
	return keys_state[key_id];
}
bool any_key(key_state_t state, byte* key_id)
{
	for (int i = 0; i < 16; i++)
	{
		if (keys_state[i] == state)
		{
			if (key_id)
				*key_id = i;

			return true;
		}
	}

	if (key_id)
		*key_id = 0;
	return false;
}

void init_surface(SDL_Window* window)
{
	screen = SDL_GetWindowSurface(window);
	bg = SDL_CreateSurface(SCREEN_WIDTH, SCREEN_HEIGHT, SDL_PIXELFORMAT_RGBA32);
	vram = SDL_CreateSurface(SCREEN_WIDTH, SCREEN_HEIGHT, SDL_PIXELFORMAT_RGBA32);

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
	SDL_Rect dst{(int)ox, (int)oy, (int)(vram->w * scale), (int)(vram->h * scale)};
	SDL_BlitSurfaceScaled(bg, nullptr, screen, &dst, SDL_SCALEMODE_NEAREST);
	SDL_BlitSurfaceScaled(vram, nullptr, screen, &dst, SDL_SCALEMODE_NEAREST);
}

void update_display(int fps, float instr_duration, const char* state_str)
{
	static char msg_buf[100]{};

	int end = std::snprintf(msg_buf, sizeof(msg_buf), "fps: %d up: %.4fms, state: %s", fps,
		instr_duration / 1000000.0f, state_str);

	// std::memset(msg_buf + end, ' ', sizeof(msg_buf) - 1);
	// msg_buf[sizeof(msg_buf) - 1] = '\0';

	std::printf("%s\n", msg_buf);
	std::fflush(stdout);
}

int main()
{
	SDL_Window* window = nullptr;
	SDL_Renderer* renderer = nullptr;
	SDL_CreateWindowAndRenderer("other chip8 simulator", 800, 600, 0, &window, &renderer);

	init_surface(window);

	static char file_name_rev[32]{};
	std::scanf("%30s", file_name_rev);
	
	start(file_name_rev);

	// 以每秒700个指令的速度执行
	constexpr uint64_t FrameIntervalNs = 1000000 / 700;

	bool quit = false;

	uint64_t frame_delay = 0;
	uint64_t instr_duration = 0;

	SDL_Event event{};
	while (!quit)
	{
		auto start_time = uptime_ns();

		// 绘制调试信息
		// int fps = (int)(1000000000.0f / frame_delay);
		// update_display(fps, instr_duration, state_str());

		// 清除按键状态
		for (int i = 0; i < KeyNum; i++)
		{
			auto& code = keys_state[i];
			if (code == key_state_t::RELEASE)
				code = key_state_t::NONE;
		}

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
					keys_state[key_code] = key_state_t::RELEASE;
				}
			}
			else if (event.type == SDL_EVENT_KEY_DOWN)
			{
				const int* end = KeyMap + KeyNum;
				auto it = std::find(KeyMap, end, event.key.key);
				if (it != end)
				{
					byte key_code = it - KeyMap;
					// std::printf("key trigger: %X\n", key_code);
					keys_state[key_code] = key_state_t::PRESSED;
				}
			}
		}

		{
			uint64_t _st = uptime_ns();
			update();
			instr_duration = uptime_ns() - _st;
		}

		if (screen_changed)
		{
			draw(10, 100, 100);
			SDL_UpdateWindowSurface(window);
			screen_changed = false;
		}

		SDL_Delay(2);
	}
	return 0;
}