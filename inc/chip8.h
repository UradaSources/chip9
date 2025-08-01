#include <cstdint>

using byte = uint8_t;
using word = uint16_t;

// chip8模拟器对外暴露的接口
namespace chip8
{
	// execute的执行状态
	enum state_t
	{
		// 正常运行中
		STATE_RUNNING,
		// 屏幕像素更新
		STATE_VRAM_UPDATE,
		// 等待按键输入
		// 经典vip模式时, 将等待一个完整的按键按下-松开过程
		STATE_WAIT_KEY,
		STATE_WAIT_KEY_UP,
		// 死循环
		STATE_INFINITE_LOOP,
		// 当前执行的指令未实现或是无效的
		STATE_NOT_IMPL,
		// 栈溢出
		STATE_ERROR_STAKE_FULL,
		// 尝试令空栈弹出
		STATE_ERROR_POP_EMPTY_STAKC,
	};

    const char* state_str(state_t st);

	// 4069字节的内存
	constexpr int MEM_SIZE = 0x1000;

	// 程序起始偏移为0x200
	constexpr int PROG_MEM_OFFSET = 0x200;

	// 64*32的显存, 在内存紧张的机器上使用每个bit代表一个像素时为64*32/8=256字节
	constexpr int SCREEN_WIDTH = 64;
	constexpr int SCREEN_HEIGHT = 32;

	// 栈深度, 决定子程序嵌套深度
	constexpr int STACK_DEEP = 12;

	// 读取显存
	bool read_vram(int x, int y);

	// 读取定时器
	void read_timer(byte* delay_timer, byte* sound_timer);

	// 设置键状态
	void set_key_state(int key_id, bool pressed);

    // 重置运行时状态并装载程序
	void reset(const byte* rom, int rom_len, const byte* font, int _font_mem_offset, int font_len);

	// 从内存获取一条指令
	void fetch(state_t* state);

    // 执行当前获取的指令
	void execute(state_t* state, byte* cached_reg);

	// 更新定时器
	void update_timer();

    // debug信息
    // 包含各寄存器的值等
    const char* debug_info();
} // namespace chip8