#include <SDL3/SDL.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_keycode.h>
#include <SDL3/SDL_rect.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_timer.h>
#include <SDL3/SDL_video.h>
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstdlib>

constexpr int KeyNum = 9;
static const int KeyMap[KeyNum]{SDLK_Q, SDLK_W, SDLK_E, SDLK_A, SDLK_S, SDLK_D, SDLK_Z, SDLK_X, SDLK_C};

int main()
{
	SDL_Window* window = nullptr;
	SDL_Renderer* renderer = nullptr;
	SDL_CreateWindowAndRenderer("test", 800, 600, 0, &window, &renderer);

	bool quit = false;
	SDL_Event event{};
	while (!quit)
	{
		while (SDL_PollEvent(&event) != 0)
		{
			if (event.type == SDL_EVENT_QUIT)
				quit = true;
			else if (event.type == SDL_EVENT_KEY_UP)
			{
				const int* end = KeyMap + KeyNum;
				auto it = std::find(KeyMap, end, event.key.key);
				if (it != end)
					std::printf("key %X\n", *it);
			}
		}

		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		SDL_RenderClear(renderer);

		SDL_SetRenderDrawColor(renderer, 255, 1, 255, 20);
		SDL_RenderDebugText(renderer, 0, 0, "hello");

		for (int y = 0; y < 32; y++)
		{
			for (int x = 0; x < 64; x++)
			{
                float px = 100 + x * 20;
                float py = 100 + y * 20;

                SDL_FRect r{px, py, 20,20};
                SDL_RenderRect(renderer, &r);
			}
		}

		SDL_RenderPresent(renderer);

		SDL_Delay(10);
	}

	return 0;
}
