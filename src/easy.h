#include <stdint.h>
#include <stdio.h>

#define MAX_REGS 30

typedef struct {
  uint64_t section_data;
  uint64_t section_bss;
  uint64_t section_code;
  uint64_t code_size;
  uint64_t bss_size;
  uint64_t data_size;
  uint64_t entry_start_point;
} BinaryHeader;

typedef struct {
  uint8_t mode;
  uint8_t regstart;
  uint64_t interrupt_pos;
  uint64_t disk_s;
  uint64_t disk_e;
  uint64_t keyboard;
  uint64_t mouse;
  uint64_t usb_dev;
} CPU_SL;

typedef struct {
  union {
    uint64_t u64;
    double f64;
    struct {
      uint32_t low32;
      float flow32;
      uint32_t high32;
      float fhigh32;
    };
    struct {
      uint16_t low16;
      uint16_t midlow16;
      uint16_t midhigh16;
      uint16_t high16;
    };
    struct {
      uint8_t b0, b1, b2, b3, b4, b5, b6, b7;
    };
  };
} Register64;

typedef struct {
  Register64 reg[MAX_REGS];
  CPU_SL sl;
} CPU;

typedef enum {
  REG64_FULL = 0,
  REG32_LOW = 1,
  REG32_HIGH = 2,
  REG16_LOW = 3,
  REG16_HIGH = 4,
  REG16_MIDLOW = 5,
  REG16_MIDHIGH = 6,
  REG8_B0 = 7,
  REG8_B1 = 8,
  REG8_B2 = 9,
  REG8_B3 = 10,
  REG8_B4 = 11,
  REG8_B5 = 12,
  REG8_B6 = 13,
  REG8_B7 = 14
} RegAccessType;

typedef struct {
  uint8_t opcode;
  uint16_t src;
  uint16_t dst;
  uint64_t imm64;
} Instruction;

typedef enum { SECTION_CODE, SECTION_DATA, SECTION_BSS } Section;

typedef enum { LABEL_TYPE_CODE, LABEL_TYPE_DATA, LABEL_TYPE_BSS } LabelType;

typedef enum {
  DATA_TYPE_RP,
  DATA_TYPE_RW,
  DATA_TYPE_RB,
  DATA_TYPE_RE,
  DATA_TYPE_RR,
} BssType;

typedef enum {
  DATA_TYPE_ASCII,
  DATA_TYPE_BYTE,
  DATA_TYPE_HWORD,
  DATA_TYPE_WORD,
  DATA_TYPE_DWORD,
} DataType;

typedef struct {
  char name[128];
  int address;
  LabelType type;
} Label;

typedef struct {
  char name[128];
  int addr;
  DataType type;
  union {
    char *ascii;
    int integer;
  };
} Data;

typedef struct {
  int bss_id;
  uint64_t addr;
  BssType type;
  uint64_t size;
} BSSSectionType;

typedef struct {
  char name[128];
  BSSSectionType section;
} BSS;

typedef struct {
  int zero;
  int sign;
  int greater;
} Flags;

typedef enum {
  OPCODE_NOP = 0x00,
  OPCODE_MOV = 0x01,
  OPCODE_ADD = 0x02,
  OPCODE_SUB = 0x03,
  OPCODE_MUL = 0x04,
  OPCODE_DIV = 0x05,
  OPCODE_AND = 0x06,
  OPCODE_OR = 0x07,
  OPCODE_XOR = 0x08,
  OPCODE_NOT = 0x09,
  OPCODE_SHL = 0x0A,
  OPCODE_SHR = 0x0B,
  OPCODE_JMP = 0x0C,
  OPCODE_JE = 0x0D,
  OPCODE_JNE = 0x0E,
  OPCODE_JL = 0x0F,
  OPCODE_JG = 0x10,
  OPCODE_CMP = 0x11,
  OPCODE_CALL = 0x12,
  OPCODE_RET = 0x13,
  OPCODE_PUSH = 0x14,
  OPCODE_POP = 0x15,
  OPCODE_HLT = 0x16,
  OPCODE_INC = 0x17,
  OPCODE_DEC = 0x18,
  OPCODE_PRINT = 0x19,
  OPCODE_ENTRY = 0x22,
  OPCODE_SYSCALL = 0x23,
  OPCODE_LOAD = 0x24,
  OPCODE_STORE = 0x25,
  OPCODE_INB = 0x26,
  OPCODE_OUTB = 0x27,
  OPCODE_CSL = 0x28,
  OPCODE_SSDP = 0x29,
  OPCODE_INFO = 0x30,
  OPCODE_SYSRET = 0x31,
} Opcode;

void parser(const char *asm_file, const char *out_file, int no_kernel_mode);
void interpret_easy64(const char *binname, char *arguments_string);
