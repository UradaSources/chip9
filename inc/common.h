#include <cstdint>

using byte = uint8_t;
using word = uint16_t;
using uint = unsigned int;

// 由chip8实现
void start();
void update();

const char* state_str();

// 由后端实现
void screen(uint x, uint y, bool f); // 更新屏幕

bool get_key(byte key);		// 检查特定键是否触发
bool any_key(byte* key_id); // 检查是否有任何键触发并返回触发的键id

// 通用
bool load_file(const char* filename, byte* buffer, int* len);
void buzzer(int frequency, int duration); // 播放蜂鸣器
void delay_ms(uint32_t ms);				  // 延迟
void delay_ns(uint32_t ns);				  // 延迟
uint64_t uptime_ns();
uint64_t uptime_ms();

constexpr const char FILE_PATH[] = "../data/5-quirks.ch8";