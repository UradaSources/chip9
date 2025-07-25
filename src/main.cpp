#include <SDL3/SDL.h>
#include <SDL3/SDL_render.h>
#include <algorithm>
#include <bitset>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>

constexpr int KeyNum = 9;
static const int KeyMap[KeyNum]{SDLK_Q, SDLK_W, SDLK_E, SDLK_A, SDLK_S, SDLK_D, SDLK_Z, SDLK_X, SDLK_C};

void grid(SDL_Renderer* renderer, uint32_t w, uint32_t h, uint32_t size, uint32_t ox, uint32_t oy)
{
	bool f = true;
	for (int y = 0; y < 32; y++)
	{
		for (int x = 0; x < 64; x++)
		{
			float px = ox + x * size;
			float py = oy + y * size;

			SDL_FRect r{px, py, (float)size, (float)size};

			f = !f;
			uint8_t a = f ? 50 : 80;

			SDL_SetRenderDrawColor(renderer, a, a, a, 255);
			SDL_RenderFillRect(renderer, &r);
		}
		f = !f;
	}
}

#include <bitset>

using byte = uint8_t;
using uint = unsigned int;

bool xor_bit(byte* dat, int len, int bit_index, bool rhs)
{
	int index = bit_index / 8;
	bit_index = bit_index % 8;

	// 计算目标位的掩码
	byte mask = rhs ? (1 << (7 - bit_index)) : 0;
	bool old = (dat[index] & mask) != 0;

	dat[index] ^= mask;

	return old && rhs;
}

// 对位集[dst+bit_offset,dst+bit_offset+bit_len]的bit_len个位进行异或
// 数据来源于位集[src,src+bit_len]
// bool bitset_xor(byte* dst, uint bit_offset, const byte* src, uint bit_len)
// {
// 	if (bit_len == 0)
// 		return false;

// 	// 将dst重新定位到参与运算的起始字节
// 	dst = dst + bit_offset / 8;
// 	// 对起始字节进行操作时的掩码
// 	int dst_write_bit_offset = bit_offset % 8;
// 	byte head_mask = (byte)(0xFF >> dst_write_bit_offset);

// 	// 写入时跨越的字节数
// 	// 这表示将受运算操作影响的字节数
// 	int span_byte_count = (dst_write_bit_offset + bit_len) / 8 + 1;
// 	// byte end_mask = (byte)(0xFF << (8 - bit_len % 8));

// 	// 处理第一个字节
// 	for (int i=0; i< bit_len;)
// 	{
// 		*dst ^= (src[0] >> bit_offset)
// 	}

// 	// 仅处理单字节
// 	if (span_byte_count == 1)
// 	{
// 		byte mask = head_mask | end_mask;
// 		byte orig = *dst; // 记录原始值

// 		// pos为7时代表前面7个字节都用不了了
// 		// len为6时代表至少需要影响6个字节
// 		// 那么7+6
// 		// pos=1 0b0111'1111
// 		// len=6 0b1111'1100
// 	}
// }

const char* bin(unsigned char byte)
{
	static char buf[8]{};
	for (int i = 7; i >= 0; --i)
	{
		buf[7 - i] = '0' + ((byte >> i) & 1);
	}
	return buf;
}

// 将字节数组的每一位整体左移
// 返回最后一个字节溢出的部分
byte array_left_shift(byte* arr, unsigned int len, unsigned int n)
{
	// 用于获取溢出部分的掩码
	byte mask = 0xF >> n;
	
	// 用于缓存溢出的部分
	byte bits1 = 0, bits2 = 0;

	unsigned int i;
	for (i = 0; i < len; i++)
	{
		// 缓存溢出的尾部并写入下一字节的头部
		bits2 = arr[i] & mask;
		arr[i] >>= n;
		arr[i] |= bits1 << (8 - n);
		bits1 = bits2;
	}
	return bits1;
}

void draw(byte* varm, const byte* sp, byte sp_h, byte x, byte y)
{
	
}

void test()
{
	// orig 000110101010000100011011
	// new  0111010000000011
	// new  001000110111010000000011

	byte array[3]{0x1A, 0xA1, 0x1B};
	int len = sizeof(array);

	for (int i = 0; i < len; i++)
	{
		std::printf("%s ", bin(array[i]));
	}
	std::printf("\n");

	// MSB
	unsigned char bits1 = 0, bits2 = 0;
	for (int i = 0; i < len; i++)
	{
		std::printf("[%d]=%s ", i, bin(array[i]));

		// 缓存溢出的头部
		bits2 = array[i] & 0x07;
		std::printf("bits2=%s ", bin(bits2));
		array[i] >>= 3;
		std::printf(">>=3 [%d]=%s ", i, bin(array[i]));

		array[i] |= bits1 << 5;
		std::printf("|=bits1<<5 [%d]=%s ", i, bin(array[i]));
		std::printf("bits1=%s\n", bin(bits1));

		bits1 = bits2;
	}
	array[0] |= bits1 << 5;

	for (int i = 0; i < len; i++)
	{
		std::printf("%s ", bin(array[i]));
	}
	std::printf("\n");
}

int main()
{
	test();
	return 0;

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

		grid(renderer, 64, 32, 8, 100, 100);

		SDL_RenderPresent(renderer);

		SDL_Delay(10);
	}

	return 0;
}