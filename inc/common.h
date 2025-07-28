#include <cstdint>

using byte = uint8_t;
using word = uint16_t;
using uint = unsigned int;

enum class key_state_t : byte
{
	NONE,
	PRESSED,
	RELEASE
};

// 由chip8实现
void start(const char* file_path);
void update();

const char* state_str();

// 由后端实现
void clear_screen();
void screen_pixel(uint x, uint y, bool f); // 更新屏幕

key_state_t get_key(byte key);				   // 检查特定键是否按下
bool any_key(key_state_t state, byte* key_id); // 检查是否有任何键按下并返回触发的键id

// 通用
bool load_file(const char* filename, byte* buffer, int* len);
void buzzer(int frequency, int duration); // 播放蜂鸣器
void delay_ms(uint32_t ms);				  // 延迟
void delay_ns(uint32_t ns);				  // 延迟
uint64_t uptime_ns();
uint64_t uptime_ms();

inline bool debug_out() { return false; }
