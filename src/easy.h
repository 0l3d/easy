#include <stdint.h>
#include <stdio.h>

#define NUM_REGS 20
#define MEMORY_SIZE 65536
#define ZMEMORY_SIZE 300

extern uint8_t 	memory[MEMORY_SIZE];

typedef struct {
	int 		section_data;
	int 		section_bss;
	int 		section_code;
	int 		bss_count;
	int 		data_size;
	int 		entry_start_point;
} 		BinaryHeader;

typedef enum {
	VAL,
	PTR,
} 		RegType;

typedef struct {
	RegType 	type;
	union {
		uint64_t 	u64;
		struct {
			uint32_t 	low32;
			uint32_t 	high32;
		};
		struct {
			uint16_t 	low16;
			uint16_t 	midlow16;
			uint16_t 	midhigh16;
			uint16_t 	high16;
		};
		struct {
			uint8_t 	b0    , b1, b2, b3, b4, b5, b6, b7;
		};
	};
} 		Register64;

typedef enum {
	REG64_FULL,
	REG32_LOW,
	REG32_HIGH,
	REG16_LOW,
	REG16_MIDLOW,
	REG16_MIDHIGH,
	REG16_HIGH,
	REG8_B0,
	REG8_B1,
	REG8_B2,
	REG8_B3,
	REG8_B4,
	REG8_B5,
	REG8_B6,
	REG8_B7
} 		RegAccessType;

typedef struct {
	uint8_t 	opcode;
	uint8_t 	src;
	uint8_t 	dst;
	uint64_t 	imm64;
} 		Instruction;

typedef enum {
	SECTION_CODE, SECTION_DATA, SECTION_BSS
} 		Section;

typedef enum {
	LABEL_TYPE_CODE, LABEL_TYPE_DATA
} 		LabelType;

typedef enum {
	DATA_TYPE_RP,
	DATA_TYPE_RW,
	DATA_TYPE_RB,
	DATA_TYPE_RE,
	DATA_TYPE_RR,
} 		BssType;

typedef enum {
	DATA_TYPE_ASCII,
	DATA_TYPE_BYTE,
	DATA_TYPE_HWORD,
	DATA_TYPE_WORD,
	DATA_TYPE_DWORD,
} 		DataType;

typedef struct {
	char 		name     [32];
	int 		address;
	LabelType 	type;
} 		Label;

typedef struct {
	char 		name     [32];
	int 		addr;
	DataType 	type;
	union {
		char           *ascii;
		int 		integer;
	};
} 		Data;

typedef struct {
	int 		bss_id;
	uint64_t 	addr;
	BssType 	type;
	uint64_t 	size;
} 		BSSSectionType;

typedef struct {
	char 		name     [32];
	BSSSectionType 	section;
} 		BSS;

typedef struct {
	int 		zero;
	int 		sign;
	int 		greater;
} 		Flags;

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
} 		Opcode;

void 		parser   (const char *asm_file, const char *out_file);
void 		interpret_easy64(const char *binname);
