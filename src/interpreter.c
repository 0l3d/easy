#include "easy.h"
#include <inttypes.h>
#include <iso646.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/syscall.h>
#include <sys/types.h>

// MEMORY EMULATION SIZE
#define EMULATED_MEMORY_SIZE 131072
// DISK EMULATION SIZE
#define EMULATED_DISK_SIZE 524288

// PRIVATE REGS EMULATION
uint64_t syscall_decoder_position = 0;
uint64_t disk_index_position = 0;

// PORTS
#define DISK_INDEX 9
#define DISK_BYTE 10
#define KEYBOARD 11
#define MOUSE 12
#define USB_DEVICES 14
// PORTS

// CPU COMPARING FLAGS
Flags flags = {0};
// CPU COMPARING FLAGS

// For Memory Management Unit, can changeable by software.(Bootloader, Kernel,
// Micro-controller).
// SL - 0 - Raw Mode (Bootloader, Kernel, Micro-controller).
// SL - 1 - Strict Mode (User-space software)
// AND Flags. (Disk, Memory, Keyboard-Mouse-USB-Dev);
CPU cpu;

// EMULATED MEMORY AND DISK
uint8_t memory[EMULATED_MEMORY_SIZE];
uint8_t disk[EMULATED_DISK_SIZE];
// EMULATED MEMORY AND DISK

uint64_t read_reg(uint8_t idx, RegAccessType access) {
  switch (access) {
  case REG64_FULL:
    return cpu.reg[idx].u64;
  case REG32_LOW:
    return cpu.reg[idx].low32;
  case REG32_HIGH:
    return cpu.reg[idx].high32;
  case REG16_LOW:
    return cpu.reg[idx].low16;
  case REG16_MIDLOW:
    return cpu.reg[idx].midlow16;
  case REG16_MIDHIGH:
    return cpu.reg[idx].midhigh16;
  case REG16_HIGH:
    return cpu.reg[idx].high16;
  case REG8_B0:
    return cpu.reg[idx].b0;
  case REG8_B1:
    return cpu.reg[idx].b1;
  case REG8_B2:
    return cpu.reg[idx].b2;
  case REG8_B3:
    return cpu.reg[idx].b3;
  case REG8_B4:
    return cpu.reg[idx].b4;
  case REG8_B5:
    return cpu.reg[idx].b5;
  case REG8_B6:
    return cpu.reg[idx].b6;
  case REG8_B7:
    return cpu.reg[idx].b7;
  }
  return 0;
}

void write_reg(uint8_t idx, RegAccessType access, uint64_t val) {
  switch (access) {
  case REG64_FULL:
    cpu.reg[idx].u64 = val;
    break;
  case REG32_LOW:
    cpu.reg[idx].low32 = (uint32_t)val;
    break;
  case REG32_HIGH:
    cpu.reg[idx].high32 = (uint32_t)val;
    break;
  case REG16_LOW:
    cpu.reg[idx].low16 = (uint16_t)val;
    break;
  case REG16_MIDLOW:
    cpu.reg[idx].midlow16 = (uint16_t)val;
    break;
  case REG16_MIDHIGH:
    cpu.reg[idx].midhigh16 = (uint16_t)val;
    break;
  case REG16_HIGH:
    cpu.reg[idx].high16 = (uint16_t)val;
    break;
  case REG8_B0:
    cpu.reg[idx].b0 = (uint8_t)val;
    break;
  case REG8_B1:
    cpu.reg[idx].b1 = (uint8_t)val;
    break;
  case REG8_B2:
    cpu.reg[idx].b2 = (uint8_t)val;
    break;
  case REG8_B3:
    cpu.reg[idx].b3 = (uint8_t)val;
    break;
  case REG8_B4:
    cpu.reg[idx].b4 = (uint8_t)val;
    break;
  case REG8_B5:
    cpu.reg[idx].b5 = (uint8_t)val;
    break;
  case REG8_B6:
    cpu.reg[idx].b6 = (uint8_t)val;
    break;
  case REG8_B7:
    cpu.reg[idx].b7 = (uint8_t)val;
    break;
  }
}

uint8_t get_index(uint16_t operand) { return operand & 0x3F; }

uint8_t get_access(uint16_t operand) { return (operand >> 6) & 0x3F; }

uint8_t get_main_type(uint16_t operand) { return (operand >> 12) & 0xF; }

void *resolve_ptr(uint64_t ptr) { return memory + ptr; };

void interpret_easy64(const char *binname, char *arguments_string) {
  // FIRMWARE LOADS SL0:
  cpu.sl.mode = 0;
  // STACK POINTER: r0
  cpu.reg[0].u64 = 0;
  FILE *binfile = fopen(binname, "rb");
  if (binfile == NULL) {
    perror("cpu is failed to open binary file");
    return;
  }
  memset(cpu.reg, 0, sizeof(cpu.reg));

  uint64_t pc = 0;

  if (arguments_string != NULL) {
    FILE *kernel_file = fopen(arguments_string, "rb");
    if (binfile == NULL) {
      perror("cpu is failed to open binary file");
      return;
    }
    fseek(kernel_file, 0, SEEK_END);
    long size = ftell(kernel_file);
    rewind(kernel_file);
    fread(&disk, 1, size, kernel_file);
    fclose(kernel_file);
  }

  fread(&memory, 1, 1024, binfile);

  while (pc < EMULATED_MEMORY_SIZE) {
    Instruction instrc;
    memcpy(&instrc, &memory[pc], sizeof(Instruction));
    switch (instrc.opcode) {
    case OPCODE_MOV:
      if (instrc.src == 0xFF) {
        uint8_t dst_reg = get_index(instrc.dst);
        uint8_t access_type = get_access(instrc.dst);
        write_reg(dst_reg, access_type, instrc.imm64);
      } else {
        uint8_t src_reg = get_index(instrc.src);
        uint8_t src_acc = get_access(instrc.src);
        uint8_t dst_reg = get_index(instrc.dst);
        uint8_t dst_acc = get_access(instrc.dst);
        uint64_t val = read_reg(src_reg, src_acc);
        write_reg(dst_reg, dst_acc, val);
      }
      break;
    case OPCODE_ADD: {
      uint8_t dst_reg = get_index(instrc.dst);
      uint8_t dst_acc = get_access(instrc.dst);
      if (instrc.src == 0xFF) {
        uint64_t dst_val = read_reg(dst_reg, dst_acc);
        uint64_t result = dst_val + instrc.imm64;
        write_reg(dst_reg, dst_acc, result);
      } else {
        uint8_t src_reg = get_index(instrc.src);
        uint8_t src_acc = get_access(instrc.src);
        uint64_t adder = read_reg(src_reg, src_acc);
        uint64_t addend = read_reg(dst_reg, dst_acc);
        uint64_t result = addend + adder;
        write_reg(dst_reg, dst_acc, result);
      }
      break;
    }
    case OPCODE_SUB: {
      uint8_t dst_reg = get_index(instrc.dst);
      uint8_t dst_acc = get_access(instrc.dst);

      if (instrc.src == 0xFF) {
        uint64_t dst_val = read_reg(dst_reg, dst_acc);
        write_reg(dst_reg, dst_acc, dst_val - instrc.imm64);
      } else {
        uint8_t src_reg = get_index(instrc.src);
        uint8_t src_acc = get_access(instrc.src);
        uint64_t dst_val = read_reg(dst_reg, dst_acc);
        uint64_t src_val = read_reg(src_reg, src_acc);
        write_reg(dst_reg, dst_acc, dst_val - src_val);
      }
      break;
    }
    case OPCODE_MUL: {
      uint8_t dst_reg = get_index(instrc.dst);
      uint8_t dst_acc = get_access(instrc.dst);

      if (instrc.src == 0xFF) {
        uint64_t dst_val = read_reg(dst_reg, dst_acc);
        write_reg(dst_reg, dst_acc, dst_val * instrc.imm64);
      } else {
        uint8_t src_reg = get_index(instrc.src);
        uint8_t src_acc = get_access(instrc.src);
        uint64_t dst_val = read_reg(dst_reg, dst_acc);
        uint64_t src_val = read_reg(src_reg, src_acc);
        write_reg(dst_reg, dst_acc, dst_val * src_val);
      }
      break;
    }
    case OPCODE_DIV: {
      uint8_t dst_reg = get_index(instrc.dst);
      uint8_t dst_acc = get_access(instrc.dst);

      if (instrc.src == 0xFF) {
        if (instrc.imm64 == 0) {
          printf("Division by zero\n");
          break;
        }
        uint64_t dst_val = read_reg(dst_reg, dst_acc);
        write_reg(dst_reg, dst_acc, dst_val / instrc.imm64);
        write_reg(dst_reg + 1, dst_acc, dst_val % instrc.imm64);
      } else {
        uint8_t src_reg = get_index(instrc.src);
        uint8_t src_acc = get_access(instrc.src);
        uint64_t dst_val = read_reg(dst_reg, dst_acc);
        uint64_t src_val = read_reg(src_reg, src_acc);
        if (src_val == 0) {
          printf("Division by zero\n");
          break;
        }
        write_reg(dst_reg, dst_acc, dst_val / src_val);
        write_reg(dst_reg + 1, dst_acc, dst_val % src_val);
      }
      break;
    }
    case OPCODE_AND: {
      uint8_t dst_reg = get_index(instrc.dst);
      uint8_t dst_acc = get_access(instrc.dst);

      if (instrc.src == 0xFF) {
        uint64_t dst_val = read_reg(dst_reg, dst_acc);
        write_reg(dst_reg, dst_acc, dst_val & instrc.imm64);
      } else {
        uint8_t src_reg = get_index(instrc.src);
        uint8_t src_acc = get_access(instrc.src);
        uint64_t dst_val = read_reg(dst_reg, dst_acc);
        uint64_t src_val = read_reg(src_reg, src_acc);
        write_reg(dst_reg, dst_acc, dst_val & src_val);
      }
      break;
    }

    case OPCODE_OR: {
      uint8_t dst_reg = get_index(instrc.dst);
      uint8_t dst_acc = get_access(instrc.dst);

      if (instrc.src == 0xFF) {
        uint64_t dst_val = read_reg(dst_reg, dst_acc);
        write_reg(dst_reg, dst_acc, dst_val | instrc.imm64);
      } else {
        uint8_t src_reg = get_index(instrc.src);
        uint8_t src_acc = get_access(instrc.src);
        uint64_t dst_val = read_reg(dst_reg, dst_acc);
        uint64_t src_val = read_reg(src_reg, src_acc);
        write_reg(dst_reg, dst_acc, dst_val | src_val);
      }
      break;
    }

    case OPCODE_XOR: {
      uint8_t dst_reg = get_index(instrc.dst);
      uint8_t dst_acc = get_access(instrc.dst);

      if (instrc.src == 0xFF) {
        uint64_t dst_val = read_reg(dst_reg, dst_acc);
        write_reg(dst_reg, dst_acc, dst_val ^ instrc.imm64);
      } else {
        uint8_t src_reg = get_index(instrc.src);
        uint8_t src_acc = get_access(instrc.src);
        uint64_t dst_val = read_reg(dst_reg, dst_acc);
        uint64_t src_val = read_reg(src_reg, src_acc);
        write_reg(dst_reg, dst_acc, dst_val ^ src_val);
      }
      break;
    }

    case OPCODE_NOT: {
      uint8_t dst_reg = get_index(instrc.dst);
      uint8_t dst_acc = get_access(instrc.dst);

      if (instrc.src == 0xFF) {
        write_reg(dst_reg, dst_acc, ~instrc.imm64);
      } else {
        uint8_t src_reg = get_index(instrc.src);
        uint8_t src_acc = get_access(instrc.src);
        uint64_t src_val = read_reg(src_reg, src_acc);
        write_reg(dst_reg, dst_acc, ~src_val);
      }
      break;
    }

    case OPCODE_SHR: {
      uint8_t dst_reg = get_index(instrc.dst);
      uint8_t dst_acc = get_access(instrc.dst);

      if (instrc.src == 0xFF) {
        uint64_t dst_val = read_reg(dst_reg, dst_acc);
        write_reg(dst_reg, dst_acc, dst_val >> instrc.imm64);
      } else {
        uint8_t src_reg = get_index(instrc.src);
        uint8_t src_acc = get_access(instrc.src);
        uint64_t dst_val = read_reg(dst_reg, dst_acc);
        uint64_t src_val = read_reg(src_reg, src_acc);
        write_reg(dst_reg, dst_acc, dst_val >> src_val);
      }
      break;
    }

    case OPCODE_SHL: {
      uint8_t dst_reg = get_index(instrc.dst);
      uint8_t dst_acc = get_access(instrc.dst);

      if (instrc.src == 0xFF) {
        uint64_t dst_val = read_reg(dst_reg, dst_acc);
        write_reg(dst_reg, dst_acc, dst_val << instrc.imm64);
      } else {
        uint8_t src_reg = get_index(instrc.src);
        uint8_t src_acc = get_access(instrc.src);
        uint64_t dst_val = read_reg(dst_reg, dst_acc);
        uint64_t src_val = read_reg(src_reg, src_acc);
        write_reg(dst_reg, dst_acc, dst_val << src_val);
      }
      break;
    }

    case OPCODE_INC: {
      uint8_t dst_reg = get_index(instrc.dst);
      uint8_t dst_acc = get_access(instrc.dst);
      uint64_t dst_val = read_reg(dst_reg, dst_acc);
      write_reg(dst_reg, dst_acc, dst_val + 1);
      break;
    }

    case OPCODE_DEC: {
      uint8_t dst_reg = get_index(instrc.dst);
      uint8_t dst_acc = get_access(instrc.dst);
      uint64_t dst_val = read_reg(dst_reg, dst_acc);
      write_reg(dst_reg, dst_acc, dst_val - 1);
      break;
    }

    case OPCODE_CMP: {
      uint64_t val1, val2;
      if (instrc.src != 0xFF) {
        uint8_t src_reg = get_index(instrc.src);
        uint8_t src_acc = get_access(instrc.src);
        val1 = read_reg(src_reg, src_acc);
      } else {
        val1 = instrc.imm64;
      }

      if (instrc.dst != 0xFF) {
        uint8_t dst_reg = get_index(instrc.dst);
        uint8_t dst_acc = get_access(instrc.dst);
        val2 = read_reg(dst_reg, dst_acc);
      } else {
        val2 = instrc.imm64;
      }

      int64_t result = (int64_t)val1 - (int64_t)val2;
      flags.zero = (result == 0);
      flags.sign = (result < 0);
      flags.greater = (result > 0);
      break;
    }
    case OPCODE_JE:
      if (flags.zero) {
        pc = pc + instrc.imm64;
        continue;
      }
      break;

    case OPCODE_JNE:
      if (!flags.zero) {
        pc = pc + instrc.imm64;
        continue;
      }
      break;

    case OPCODE_JL:
      if (flags.sign) {
        pc = pc + instrc.imm64;
        continue;
      }
      break;

    case OPCODE_JG:
      if (flags.greater) {
        pc = pc + instrc.imm64;
        continue;
      }
      break;
    case OPCODE_PUSH: {
      if (instrc.dst == 0xFF) {
        memory[cpu.reg[0].u64] = instrc.imm64;
      } else {
        uint8_t src_reg = get_index(instrc.dst);
        uint8_t src_acc = get_access(instrc.dst);
        uint64_t val = read_reg(src_reg, src_acc);
        memory[cpu.reg[0].u64] = val;
      }

      cpu.reg[0].u64++;
      break;
    }
    case OPCODE_SYSCALL: {
    }
    case OPCODE_INB: {
      if (instrc.imm64 == 10) {
        uint8_t dst_reg = get_index(instrc.dst);
        uint8_t access_type = get_access(instrc.dst);
        write_reg(dst_reg, access_type, disk[disk_index_position]);
        disk_index_position++;
      }
    } break;
    case OPCODE_OUTB: {
      if (instrc.imm64 == 9) {
        uint8_t dst_reg = get_index(instrc.dst);
        uint8_t dst_acc = get_access(instrc.dst);
        uint64_t val = read_reg(dst_reg, dst_acc);
        disk_index_position = val;
      }
    } break;
    case OPCODE_CSL: {
    }
    case OPCODE_SSDP: {
    }
    case OPCODE_POP: {
      uint8_t dst_reg = get_index(instrc.dst);
      uint8_t dst_acc = get_access(instrc.dst);

      uint64_t val = memory[cpu.reg[0].u64];
      write_reg(dst_reg, dst_acc, val);

      cpu.reg[0].u64--;
      break;
    }

    case OPCODE_JMP:
      if (instrc.src != 0xFF) {
        uint8_t dst_reg = get_index(instrc.dst);
        uint8_t dst_acc = get_access(instrc.dst);
        uint64_t val = read_reg(dst_reg, dst_acc);
        pc = val;
      } else {
        pc = pc + instrc.imm64;
      }
      continue;

    case OPCODE_CALL: {
      uint64_t ret = pc + sizeof(Instruction);
      memcpy(&memory[cpu.reg[0].u64], &ret, sizeof(uint64_t));
      cpu.reg[0].u64 += sizeof(uint64_t);
      pc = pc + instrc.imm64;
      continue;
    }
    case OPCODE_RET: {
      cpu.reg[0].u64 -= sizeof(uint64_t);
      uint64_t ret;
      memcpy(&ret, &memory[cpu.reg[0].u64], sizeof(uint64_t));
      pc = ret;
      continue;
    }
    case OPCODE_NOP:
      break;
    case OPCODE_HLT:
      fclose(binfile);
      return;
    case OPCODE_PRINT: {
      uint8_t reg = get_index(instrc.dst);
      uint8_t acc = get_access(instrc.dst);
      uint64_t val = read_reg(reg, acc);
      printf("DEBUG: %ld\n", val);
      break;
    }

    case OPCODE_LOAD: {
      uint8_t dst_reg = get_index(instrc.dst);
      uint8_t dst_acc = get_access(instrc.dst);
      uint8_t src_reg = get_index(instrc.src);
      uint8_t src_acc = get_access(instrc.src);

      uint64_t addr = read_reg(src_reg, src_acc);

      void *ptr = resolve_ptr(addr);
      write_reg(dst_reg, dst_acc, *(uint8_t *)ptr);
      break;
    }

    case OPCODE_STORE: {
      uint8_t src_reg = get_index(instrc.src);
      uint8_t src_acc = get_access(instrc.src);

      uint64_t addr;

      if (instrc.dst == 0xAD) {
        addr = instrc.imm64;
      } else {
        uint8_t dst_reg = get_index(instrc.dst);
        uint8_t dst_acc = get_access(instrc.dst);
        addr = read_reg(dst_reg, dst_acc);
      }

      void *ptr = resolve_ptr(addr);
      if (ptr == NULL) {
        fprintf(stderr, "STORE: Invalid memory access at address %lx\n", addr);
        fclose(binfile);
        return;
      }
      uint64_t val = read_reg(src_reg, src_acc);
      *(uint8_t *)ptr = (uint8_t)val;
      break;
    }

    case OPCODE_ENTRY:
      pc = pc + instrc.imm64;
      continue;
    default:
      continue;
    }

    pc += sizeof(Instruction);
  }

  fclose(binfile);
}
