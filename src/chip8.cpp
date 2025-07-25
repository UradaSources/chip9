// 2025/7/23 13:57
// https://tobiasvl.github.io/blog/write-a-chip-8-emulator/
#include <cstdint>

using byte = uint8_t;
using word = uint16_t;

// 默认的字体数据和偏移
constexpr int DEFAULT_FONT_MEM_OFFSET = 0x050;
constexpr byte DEFAULT_FONT_DAT[] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
    0x20, 0x60, 0x20, 0x20, 0x70, // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
    0x90, 0x90, 0xF0, 0x10, 0x10, // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
    0xF0, 0x10, 0x20, 0x40, 0x40, // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90, // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
    0xF0, 0x80, 0x80, 0x80, 0xF0, // C
    0xE0, 0x90, 0x90, 0x90, 0xE0, // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
    0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

// 精灵的固定宽度
// 高度由绘制命令指定
constexpr int FIXED_SPRITE_WIDTH = 8;

// 字符精灵的固定尺寸4x5
constexpr int FIXED_FONT_WIDTH = 4;
constexpr int FIXED_FONT_HEIGHT = 5;

// 4069字节的内存
constexpr int PROG_MEM_OFFSET = 0x200;
static byte ram[0xFFF]{};

// 64*32的显存, 每位代表一个像素
// 64*32/8=256字节
constexpr int SCREEN_WIDTH = 64;
constexpr int SCREEN_HEIGHT = 32;
static byte vram[SCREEN_WIDTH * SCREEN_HEIGHT / 8]{};

// 16个通用寄存器
static byte reg[16]{};

// 地址寄存器
static word I = 0;

static word IR = 0;
static word PC = 0;

// 堆栈
constexpr int STACK_DEEP = 16;
static word stack[16]{};

// 栈顶指针
static byte SP = 0;

// 计时器
static byte dt = 0;
static byte st = 0;

// 16进制键盘布局
// 1	2	3	C
// 4	5	6	D
// 7	8	9	E
// A	0	B	F
static bool keys[16]{};

// 字体精灵的储存位置
int font_mem_offset = 0;

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <windows.h>

#define assertm(expr, msg)                                                     \
  if (!(expr)) {                                                               \
    std::printf("Assertion failed: %s\nFile: %s, Line: %d\n", msg, __FILE__,   \
                __LINE__);                                                     \
    exit(EXIT_FAILURE);                                                        \
  }

bool load_rom(const char *filename, byte *buffer, int *len) {
  std::ifstream file(filename, std::ios::binary | std::ios::ate);

  if (!file.is_open()) {
    std::printf("Error: Could not open file %s: %s\n", filename,
                strerror(errno));
    return false;
  }

  size_t file_size = file.tellg();
  file.seekg(0, std::ios::beg);

  if (file_size > static_cast<size_t>(*len)) {
    std::printf("Error: Buffer too small (%d bytes), need %zu bytes\n", *len,
                file_size);
    return false;
  }

  file.read(reinterpret_cast<char *>(buffer), file_size);

  if (!file) {
    std::printf("Error: Only %zd bytes could be read\n", file.gcount());
    return false;
  }

  *len = file.gcount();
  return true;
}

// 有任何键被按下时返回true并通过key_id返回
bool any_key(byte *key_id) {
  for (int i = 0; i < sizeof(keys); i++) {
    if (keys[i]) {
      *key_id = i;
      return true;
    }
  }
  return false;
}

// 从字节数组中读取特定位
bool read_bit(const byte *dat, int len, int bit_index) {
  int index = bit_index / 8;
  bit_index = bit_index % 8;
  assertm(dat && index < len, "index out of range");

  return (dat[index] >> (7 - bit_index)) & 1;
}

// 对目标位进行xor
// 冲突时返回true
bool xor_bit(byte *dat, int len, int bit_index, bool rhs) {
  int index = bit_index / 8;
  bit_index = bit_index % 8;
  assertm(dat && index < len, "index out of range");

  // 计算目标位的掩码
  byte mask = rhs ? (1 << (7 - bit_index)) : 0;
  bool old = (dat[index] & mask) != 0;

  dat[index] ^= mask;

  return old && rhs;
}

// 在屏幕上绘制精灵
// 将对x,y进行取模, 绘制冲突时设置VF为1
void draw(int x, int y, const byte *sp, int h) {
  // 对坐标进行取模
  x %= SCREEN_WIDTH;
  y %= SCREEN_HEIGHT;

  reg[0xF] = 0;

  for (int row = 0; row < h; row++) {
    int cy = y + row;
    if (cy >= SCREEN_HEIGHT)
      break;

    for (int col = 0; col < 8; col++) {
      int cx = x + col;
      if (cx >= SCREEN_WIDTH)
        break;

      bool old = read_bit(vram, sizeof(vram), cy * SCREEN_WIDTH + cx);

      // 在当前行里读取
      bool src = read_bit(sp + row, 1, col);
      reg[0xF] |= xor_bit(vram, sizeof(vram), cy * SCREEN_WIDTH + cx, src);

      bool vnew = read_bit(vram, sizeof(vram), cy * SCREEN_WIDTH + cx);

      // std::printf("draw state: sp=%d,vram=%d->%d,vf=%d\n", src, old, vnew,
      // reg[0xF]);
    }
  }
}

// 调试打印
void print_bytes(const byte *dat, int len) {
  for (int i = 0; i < sizeof(vram); i++)
    std::printf("%02X ", vram[i]);
  std::printf("\n");
}
void print_vram() {
  for (int y = 0; y < SCREEN_HEIGHT; ++y) {
    std::printf("%02X ", y);
    for (int x = 0; x < SCREEN_WIDTH; ++x) {
      int pos = y * SCREEN_WIDTH + x;
      bool p = read_bit(vram, sizeof(vram), pos);
      std::cout << (p ? '#' : '.');
    }
    std::cout << '\n';
  }
}

void delay_ms(int ms) { Sleep(ms); }
void buzzer(int frequency, int duration) { Beep(frequency, duration); }
uint32_t uptime() { return (uint32_t)GetTickCount(); }

// 从内存中查找下一条指令
void fetch() {
  byte h = ram[PC++];
  byte l = ram[PC++];
  IR = (word)((h << 8) | l);
}

// execute的执行状态
enum state_t {
  // 正常运行中
  STATE_RUNNING,
  // 屏幕像素更新
  STATE_VRAM_UPDATE,
  // 等待按键输入
  // 经典vip模式时, 将等待一个完整的按键按下-松开过程
  STATE_WAIT_KEY,
  // 死循环
  STATE_INFINITE_LOOP,
  // 当前执行的指令未实现或是无效的
  STATE_NOT_IMPL,
  // 栈溢出
  STATE_ERROR_STAKE_FULL,
  // 尝试令空栈弹出
  STATE_ERROR_POP_EMPTY_STAKC,
};

const char *state_tostr(state_t st) {
  switch (st) {
  case STATE_RUNNING:
    return "STATE_RUNNING";
  case STATE_VRAM_UPDATE:
    return "STATE_VRAM_UPDATE";
  case STATE_WAIT_KEY:
    return "STATE_WAIT_KEY";
  case STATE_INFINITE_LOOP:
    return "STATE_INFINITE_LOOP";
  case STATE_NOT_IMPL:
    return "STATE_NOT_IMPL";
  case STATE_ERROR_STAKE_FULL:
    return "STATE_ERROR_STAKE_FULL";
  case STATE_ERROR_POP_EMPTY_STAKC:
    return "STATE_ERROR_POP_EMPTY_STACK";
  default:
    return "UNKNOWN_STATE";
  }
}

// 执行当前指令
// state指示运行状态, 在执行完毕过程中state可能被更新
// ctx缓存上一个状态时记录的寄存器编号, 可能是无效的
// 在执行完毕后处理异常state, 停止运行或是重置state后再调用execute继续执行
void execute(state_t *state, byte *cached_reg) {
  // 处理0xFX0A的按键阻塞
  // Vx已被记录到cached_reg
  if (*state == STATE_WAIT_KEY) {
    byte key_id = 0;
    if (any_key(&key_id)) {
      reg[*cached_reg] = key_id;
      *state = STATE_RUNNING;
    }
    return;
  }
  // 忽略其他无效状态
  else if (*state != STATE_RUNNING)
    return;

  constexpr word HEAD_MASK = 0xF000;
  switch (IR & HEAD_MASK) {
  case 0: {
    // 00E0: 清屏
    if (IR == 0x00E0) {
      std::memset(vram, 0, sizeof(vram));
      *state = STATE_VRAM_UPDATE;
    }
    // 00EE: 弹出栈顶地址
    else if (IR == 0x00EE) {
      // 检查空栈
      if (SP == 0) {
        *state = STATE_ERROR_POP_EMPTY_STAKC;
        return;
      }

      SP--;
      PC = stack[SP];
    } else {
      *state = STATE_NOT_IMPL;
    }
    return;
  }
  // 1NNN: 无条件跳转到NNN
  case 0x1000: {
    word addr = (word)(IR & 0x0FFF);

    // 额外处理死循环
    if (addr == PC - 2) {
      *state = STATE_INFINITE_LOOP;
      return;
    }

    PC = addr;
    return;
  }
  // 2NNN: 将当前地址压栈并跳转到12位地址NNN
  case 0x2000: {
    // 检查栈溢出
    if (SP >= STACK_DEEP) {
      *state = STATE_ERROR_STAKE_FULL;
      return;
    }

    stack[SP++] = PC;
    word addr = (word)(IR & 0x0FFF);
    PC = addr;
    return;
  }
  // 3XNN: 若Vx==NN, 则跳过下一条指令
  case 0x3000: {
    byte r = (byte)((IR & 0x0F00) >> 8);
    byte val = (byte)(IR & 0x00FF);
    if (reg[r] == val)
      PC += 2;
    return;
  }
  // 4XNN: 若Vx!=NN, 则跳过下一条指令
  case 0x4000: {
    byte r = (byte)((IR & 0x0F00) >> 8);
    byte val = (byte)(IR & 0x00FF);
    if (reg[r] != val)
      PC += 2;
    return;
  }
  // 5XY0: 若Vx==Vy, 则跳过下一条指令
  case 0x5000: {
    byte x = (byte)((IR & 0x0F00) >> 8);
    byte y = (byte)((IR & 0x00F0) >> 4);
    if (reg[x] == reg[y])
      PC += 2;
    return;
  }
  // 6XNN: VX=NNN
  case 0x6000: {
    byte r = (byte)((IR & 0x0F00) >> 8);
    byte val = (byte)(IR & 0x00FF);
    reg[r] = val;
    return;
  }
  // 7XNN: VX+=NNN, 不设置进位标志
  case 0x7000: {
    byte r = (byte)((IR & 0x0F00) >> 8);
    byte val = (byte)(IR & 0x00FF);
    reg[r] += val;
    return;
  }
  // 诸算术与逻辑运算指令
  case 0x8000: {
    word opcode = IR & 0x000F;
    byte x = (byte)((IR & 0x0F00) >> 8);
    byte y = (byte)((IR & 0x00F0) >> 4);

    switch (opcode) {
    // 8XY0: Vx = Vy
    case 0: {
      reg[x] = reg[y];
      return;
    }
    // 8XY1: Vx |= Vy
    case 1: {
      reg[x] |= reg[y];
      return;
    }
    // 8XY2 Vx &= Vy
    case 2: {
      reg[x] &= reg[y];
      return;
    }
    // 8XY3[a] Vx ^= Vy
    case 3: {
      reg[x] ^= reg[y];
      return;
    }
    // 8XY4 Vx += Vy
    case 4: {
      int v = reg[x] + reg[y];
      reg[0xF] = v > 0xFF;
      reg[x] = (byte)v;
      return;
    }
    // 8XY5 Vx -= Vy
    case 5: {
      reg[0xF] = reg[x] >= reg[y];
      reg[x] -= reg[y];
      return;
    }
    // 8XY6[a] Vx >>= 1
    case 6: {
      reg[0xF] = reg[x] & 1;
      reg[x] >>= 1;
      return;
    }
    // 8XY7[a] Vx = Vy - Vx
    case 7: {
      reg[0xF] = reg[y] >= reg[x];
      reg[x] = reg[y] - reg[x];
      return;
    }
    // 8XYE[a] Vx <<= 1
    case 0xE: {
      reg[0xF] = reg[x] & 0x80;
      reg[x] <<= 1;
      return;
    }
    default: {
      *state = STATE_NOT_IMPL;
      return;
    }
    }
  }
  // 9XY0: 若Vx!=Vy, 则跳过下一条指令
  case 0x9000: {
    byte x = (byte)((IR & 0x0F00) >> 8);
    byte y = (byte)((IR & 0x00F0) >> 4);
    if (reg[x] != reg[y])
      PC += 2;
    return;
  }
  // ANNN: I=NNN
  case 0xA000: {
    word addr = (word)(IR & 0x0FFF);
    I = addr;
    return;
  }
  // BNNN: 跳转到地址V0+NNN
  case 0xB000: {
    word addr_offset = (word)(IR & 0x0FFF);
    PC += reg[0] + addr_offset;
    return;
  }
  // CXNN: Vx = rand() & NN
  case 0xC000: {
    byte r = (byte)((IR & 0x0F00) >> 8);
    byte val = (byte)(IR & 0x00FF) & (std::rand() % 256);
    reg[r] = val;
    return;
  }
  // DXYN: 绘制精灵
  case 0xD000: {
    // 精灵数据
    byte *sp_dat = ram + I;

    // 绘制坐标
    byte x_reg = (IR & 0x0F00) >> 8;
    byte y_reg = (IR & 0x00F0) >> 4;

    // 精灵高度
    byte sp_h = IR & 0x000F;

    // 绘制并自动处理VF碰撞标志
    draw(reg[x_reg], reg[y_reg], sp_dat, sp_h);

    *state = STATE_VRAM_UPDATE;

    return;
  }
  case 0xE000: {
    word opcode = IR & 0xF0FF;

    // EX9E: if (keys(Vx)) PC+=2
    if (opcode == 0xE09E) {
      byte r = (IR & 0x0F00) >> 8;
      byte key = reg[r];

      if (key <= 0xF && keys[key])
        PC += 2;
    }
    // EXA1: if (!keys(Vx)) PC+=2
    else if (opcode == 0xE0A1) {
      byte r = (IR & 0x0F00) >> 8;
      byte key = reg[r];

      if (key <= 0xF && !keys[key])
        PC += 2;
    } else {
      *state = STATE_NOT_IMPL;
    }
    return;
  }
  // case 0xF000:
  default: {
    word opcode = IR & 0xF0FF;
    byte r = (IR & 0x0F00) >> 8;

    // FX07: Vx = delay_timer
    if (opcode == 0xF007) {
      reg[r] = dt;
    }
    // FX0A: 阻塞的等待按键按下并储存到Vx
    else if (opcode == 0xF00A) {
      byte key_id = 0;
      if (any_key(&key_id)) {
        reg[*cached_reg] = key_id;
      } else {
        *state = STATE_WAIT_KEY;
        *cached_reg = r;
      }
    }
    // FX15: delay_timer=Vx
    else if (opcode == 0xF015) {
      dt = reg[r];
    }
    // FX18: sound_timer=Vx
    else if (opcode == 0xF018) {
      st = reg[r];
    }
    // FX1E: I+=Vx
    else if (opcode == 0xF01E) {
      I += reg[r];
    }
    // FX29: 将I设置为Vx所储存的字符精灵的地址
    else if (opcode == 0xF029) {
      // 每个字符精灵占用2字节
      I = font_mem_offset + (reg[r] & 0xF) * 4;
    }
    // FX33: 拆解Vx的百,十,个位, 分别储存到I,I+1,I+2
    else if (opcode == 0xF033) {
      byte val = reg[r];

      ram[I] = val % 10;
      ram[I + 1] = (val / 10) % 10;
      ram[I + 2] = (val / 100) % 10;
    }
    // FX55: 将V0-Vx储存到I-I+x
    else if (opcode == 0xF055) {
      for (int i = 0; i <= r; i++)
        ram[I + i] = reg[i];
    }
    // FX65: 从I-I+x读取值并储存到V0-Vx
    else if (opcode == 0xF065) {
      for (int i = 0; i <= r; i++)
        reg[i] = ram[I + i];
    } else {
      *state = STATE_NOT_IMPL;
    }
    return;
  }
  }
}

void reset(const byte *rom, int rom_len, const byte *font, int _font_mem_offset,
           int font_len) {
  assertm(rom && rom_len > 0 && rom_len <= sizeof(ram) - PROG_MEM_OFFSET,
          "rom data invaild");

  IR = 0;
  I = 0;
  PC = PROG_MEM_OFFSET;

  st = 0;
  dt = 0;

  std::memset(reg, 0, sizeof(reg));
  std::memset(keys, 0, sizeof(keys));
  std::memset(vram, 0, sizeof(vram));
  std::memset(ram, 0, sizeof(ram));
  std::memcpy(ram + PROG_MEM_OFFSET, rom, rom_len);

  // 写入字体
  if (!font) {
    font = DEFAULT_FONT_DAT;
    font_len = sizeof(DEFAULT_FONT_DAT);
    _font_mem_offset = DEFAULT_FONT_MEM_OFFSET;
  }

  assertm(_font_mem_offset >= 0 && _font_mem_offset < sizeof(ram) &&
              font_len > 0,
          "font data invaild");

  font_mem_offset = _font_mem_offset;
  std::memcpy(ram + font_mem_offset, font, font_len);
}

int main(int argc, char *argv[]) {

  if (argc < 2) {
    std::printf("usage: %s <file_path>\n", argv[0]);
    return 1; // 返回错误代码
  }

  // 加载镜像文件
  {
    const char *file_path = argv[1];

    byte buffer[512]{};
    int len = sizeof(buffer);

    if (!load_rom(file_path, buffer, &len)) {
      std::printf("rom load faild\n");
      exit(-1);
    }
	
	// 初始化cpu并清空ram/寄存器组
	reset(buffer, len, nullptr, 0, 0);
	
	// 释放缓冲区
  }

  // 运行期间的上下文变量
  state_t state = STATE_RUNNING;
  byte cached_reg = 0;

  bool quit = false;
  uint32_t time = 0;
  
  constexpr int STEP_INTERVAL = 1000 / 700;

  while (!quit) {	 
    word old_pc = PC;

    fetch();
    execute(&state, &cached_reg);

    // 打印指令运行信息
    std::printf("PC=0x%04X,IR=0x%04X,NPC=0x%04X,I=0x%04X reg={", old_pc, IR, PC,
                I);
    for (int i = 0; i < 16; i++) {
      if (reg[i])
        std::printf("%X:0x%04X,", i, reg[i]);
    }
    std::printf("}\n");

    // 处理运行状态
    // 稍后重置状态或是退出
    if (state != STATE_RUNNING || state != STATE_WAIT_KEY) {
      switch (state) {
      case STATE_VRAM_UPDATE: {
        print_vram();
        state = STATE_RUNNING;
        break;
      }
      case STATE_INFINITE_LOOP: {
        quit = true;
        break;
      }
      case STATE_NOT_IMPL:
      case STATE_ERROR_STAKE_FULL:
      case STATE_ERROR_POP_EMPTY_STAKC: {
        std::printf("ERROR: %s\n", state_tostr(state));
        quit = true;
        break;
      }
      default:
        break;
      }
    }
	
	// 
	
	delay_ms(1000/700);
  }

  std::puts("RUN DONE\n");
  return 0;
}
