#include "easy.h"
#include <inttypes.h>
#include <iso646.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>
#include <sys/types.h>

// PORTABILITY
#if defined(__linux__)
#include <unistd.h>
#else
#include "portability.h"
#endif

Flags 		flags = {0};

Register64 	regsc[NUM_REGS];

uint8_t 	memory [MEMORY_SIZE] = {0};
uint8_t        *zmemory[ZMEMORY_SIZE] = {0};

uint64_t 
read_reg(uint8_t idx, RegAccessType access)
{
	switch (access) {
		case REG64_FULL:
		return regsc[idx].u64;
	case REG32_LOW:
		return regsc[idx].low32;
	case REG32_HIGH:
		return regsc[idx].high32;
	case REG16_LOW:
		return regsc[idx].low16;
	case REG16_MIDLOW:
		return regsc[idx].midlow16;
	case REG16_MIDHIGH:
		return regsc[idx].midhigh16;
	case REG16_HIGH:
		return regsc[idx].high16;
	case REG8_B0:
		return regsc[idx].b0;
	case REG8_B1:
		return regsc[idx].b1;
	case REG8_B2:
		return regsc[idx].b2;
	case REG8_B3:
		return regsc[idx].b3;
	case REG8_B4:
		return regsc[idx].b4;
	case REG8_B5:
		return regsc[idx].b5;
	case REG8_B6:
		return regsc[idx].b6;
	case REG8_B7:
		return regsc[idx].b7;
	}
	return 0;
}

void 
write_reg(uint8_t idx, RegAccessType access, uint64_t val)
{
	switch (access) {
		case REG64_FULL:
		regsc[idx].u64 = val;
		break;
	case REG32_LOW:
		regsc[idx].low32 = (uint32_t) val;
		break;
	case REG32_HIGH:
		regsc[idx].high32 = (uint32_t) val;
		break;
	case REG16_LOW:
		regsc[idx].low16 = (uint16_t) val;
		break;
	case REG16_MIDLOW:
		regsc[idx].midlow16 = (uint16_t) val;
		break;
	case REG16_MIDHIGH:
		regsc[idx].midhigh16 = (uint16_t) val;
		break;
	case REG16_HIGH:
		regsc[idx].high16 = (uint16_t) val;
		break;
	case REG8_B0:
		regsc[idx].b0 = (uint8_t) val;
		break;
	case REG8_B1:
		regsc[idx].b1 = (uint8_t) val;
		break;
	case REG8_B2:
		regsc[idx].b2 = (uint8_t) val;
		break;
	case REG8_B3:
		regsc[idx].b3 = (uint8_t) val;
		break;
	case REG8_B4:
		regsc[idx].b4 = (uint8_t) val;
		break;
	case REG8_B5:
		regsc[idx].b5 = (uint8_t) val;
		break;
	case REG8_B6:
		regsc[idx].b6 = (uint8_t) val;
		break;
	case REG8_B7:
		regsc[idx].b7 = (uint8_t) val;
		break;
	}
}

uint8_t 
get_index(uint16_t operand)
{
	return operand & 0x3F;
}

uint8_t 
get_access(uint16_t operand)
{
	return (operand >> 6) & 0x3F;
}

uint8_t 
get_main_type(uint16_t operand)
{
	return (operand >> 12) & 0xF;
}

void           *
resolve_ptr(uint64_t ptr, BinaryHeader * header, BSSSectionType * bss)
{
	uint64_t 	data_start = header->section_data;
	uint64_t 	data_end = data_start + header->data_size;

	if (ptr >= data_start && ptr < data_end) {
		return memory + (ptr - data_start);
	}
	for (int i = 0; i < header->bss_count; i++) {
		BSSSectionType 	bss_entry = bss[i];
		if (ptr >= bss_entry.addr && ptr < bss_entry.addr + bss_entry.size) {
			return (void *) ((uint8_t *) zmemory[bss_entry.bss_id] +
					 (ptr - bss_entry.addr));
		}
	}
	return NULL;
}

void 
interpret_easy64(const char *binname)
{
	FILE           *binfile = fopen(binname, "rb");
	if (binfile == NULL) {
		perror("cpu is failed to open binary file");
		return;
	}
	memset(regsc, 0, sizeof(regsc));

	//CPU FEATURES
		uint64_t call_stack[4096];
	int 		sppush = 0;
	int 		spcall = 0;

	Register64 	push_stack[4096];

	BinaryHeader 	header;

	fseek(binfile, 0, SEEK_SET);
	if (fread(&header, sizeof(BinaryHeader), 1, binfile) != 1) {
		perror("Binary header cant read failed");
		return;
	};

	fseek(binfile, header.section_data, SEEK_SET);
	if (fread(&memory, 1, header.data_size, binfile) !=
	    (size_t) header.data_size) {
		perror("Data section read failed");
		return;
	};

	BSSSectionType 	bss[header.bss_count];
	if (header.bss_count != 0) {
		fseek(binfile, header.section_bss, SEEK_SET);
		if (fread(&bss, sizeof(BSSSectionType), header.bss_count, binfile) !=
		    (size_t) header.bss_count) {
			perror("Bss section read failed");
			return;
		}
	}
	for (int i = 0; i < header.bss_count; i++) {
		zmemory[bss[i].bss_id] = (uint8_t *) calloc(bss[i].size, sizeof(uint8_t));
	}

	if (header.entry_start_point > 0) {
		fseek(binfile, header.entry_start_point, SEEK_SET);
	} else {
		fseek(binfile, header.section_code, SEEK_SET);
	}

	uint64_t 	pc = 0;

	Instruction 	instrc;
	while (fread(&instrc, sizeof(Instruction), 1, binfile)) {
		switch (instrc.opcode) {
		case OPCODE_MOV:
			if (instrc.src == 0xFF) {
				uint8_t 	dst_reg = get_index(instrc.dst);
				uint8_t 	access_type = get_access(instrc.dst);
				write_reg(dst_reg, access_type, instrc.imm64);
				regsc[dst_reg].type = VAL;
			} else if (instrc.src == 0xAD) {
				uint8_t 	dst_reg = get_index(instrc.dst);
				uint8_t 	access_type = get_access(instrc.dst);
				write_reg(dst_reg, access_type, instrc.imm64);
				regsc[dst_reg].type = PTR;
			} else {
				uint8_t 	src_reg = get_index(instrc.src);
				uint8_t 	src_acc = get_access(instrc.src);
				uint8_t 	dst_reg = get_index(instrc.dst);
				uint8_t 	dst_acc = get_access(instrc.dst);
				uint64_t 	val = read_reg(src_reg, src_acc);
				write_reg(dst_reg, dst_acc, val);
				if (regsc[src_reg].type == PTR) {
					regsc[dst_reg].type = PTR;
				} else {
					regsc[dst_reg].type = VAL;
				}
			}
			break;
		case OPCODE_ADD:{
				uint8_t 	dst_reg = get_index(instrc.dst);
				uint8_t 	dst_acc = get_access(instrc.dst);
				if (instrc.src == 0xFF) {
					uint64_t 	dst_val = read_reg(dst_reg, dst_acc);
					uint64_t 	result = dst_val + instrc.imm64;
					write_reg(dst_reg, dst_acc, result);
				} else {
					uint8_t 	src_reg = get_index(instrc.src);
					uint8_t 	src_acc = get_access(instrc.src);
					uint64_t 	adder = read_reg(src_reg, src_acc);
					uint64_t 	addend = read_reg(dst_reg, dst_acc);
					uint64_t 	result = addend + adder;
					write_reg(dst_reg, dst_acc, result);
				}
				break;
			}
		case OPCODE_SUB:{
				uint8_t 	dst_reg = get_index(instrc.dst);
				uint8_t 	dst_acc = get_access(instrc.dst);

				if (instrc.src == 0xFF) {
					uint64_t 	dst_val = read_reg(dst_reg, dst_acc);
					write_reg(dst_reg, dst_acc, dst_val - instrc.imm64);
				} else {
					uint8_t 	src_reg = get_index(instrc.src);
					uint8_t 	src_acc = get_access(instrc.src);
					uint64_t 	dst_val = read_reg(dst_reg, dst_acc);
					uint64_t 	src_val = read_reg(src_reg, src_acc);
					write_reg(dst_reg, dst_acc, dst_val - src_val);
				}
				break;
			}
		case OPCODE_MUL:{
				uint8_t 	dst_reg = get_index(instrc.dst);
				uint8_t 	dst_acc = get_access(instrc.dst);

				if (instrc.src == 0xFF) {
					uint64_t 	dst_val = read_reg(dst_reg, dst_acc);
					write_reg(dst_reg, dst_acc, dst_val * instrc.imm64);
				} else {
					uint8_t 	src_reg = get_index(instrc.src);
					uint8_t 	src_acc = get_access(instrc.src);
					uint64_t 	dst_val = read_reg(dst_reg, dst_acc);
					uint64_t 	src_val = read_reg(src_reg, src_acc);
					write_reg(dst_reg, dst_acc, dst_val * src_val);
				}
				break;
			}
		case OPCODE_DIV:{
				uint8_t 	dst_reg = get_index(instrc.dst);
				uint8_t 	dst_acc = get_access(instrc.dst);

				if (instrc.src == 0xFF) {
					if (instrc.imm64 == 0) {
						printf("Division by zero\n");
						break;
					}
					uint64_t 	dst_val = read_reg(dst_reg, dst_acc);
					write_reg(dst_reg, dst_acc, dst_val / instrc.imm64);
					write_reg(dst_reg + 1, dst_acc, dst_val % instrc.imm64);
				} else {
					uint8_t 	src_reg = get_index(instrc.src);
					uint8_t 	src_acc = get_access(instrc.src);
					uint64_t 	dst_val = read_reg(dst_reg, dst_acc);
					uint64_t 	src_val = read_reg(src_reg, src_acc);
					if (src_val == 0) {
						printf("Division by zero\n");
						break;
					}
					write_reg(dst_reg, dst_acc, dst_val / src_val);
					write_reg(dst_reg + 1, dst_acc, dst_val % src_val);
				}
				break;
			}
		case OPCODE_AND:{
				uint8_t 	dst_reg = get_index(instrc.dst);
				uint8_t 	dst_acc = get_access(instrc.dst);

				if (instrc.src == 0xFF) {
					uint64_t 	dst_val = read_reg(dst_reg, dst_acc);
					write_reg(dst_reg, dst_acc, dst_val & instrc.imm64);
				} else {
					uint8_t 	src_reg = get_index(instrc.src);
					uint8_t 	src_acc = get_access(instrc.src);
					uint64_t 	dst_val = read_reg(dst_reg, dst_acc);
					uint64_t 	src_val = read_reg(src_reg, src_acc);
					write_reg(dst_reg, dst_acc, dst_val & src_val);
				}
				break;
			}

		case OPCODE_OR:{
				uint8_t 	dst_reg = get_index(instrc.dst);
				uint8_t 	dst_acc = get_access(instrc.dst);

				if (instrc.src == 0xFF) {
					uint64_t 	dst_val = read_reg(dst_reg, dst_acc);
					write_reg(dst_reg, dst_acc, dst_val | instrc.imm64);
				} else {
					uint8_t 	src_reg = get_index(instrc.src);
					uint8_t 	src_acc = get_access(instrc.src);
					uint64_t 	dst_val = read_reg(dst_reg, dst_acc);
					uint64_t 	src_val = read_reg(src_reg, src_acc);
					write_reg(dst_reg, dst_acc, dst_val | src_val);
				}
				break;
			}

		case OPCODE_XOR:{
				uint8_t 	dst_reg = get_index(instrc.dst);
				uint8_t 	dst_acc = get_access(instrc.dst);

				if (instrc.src == 0xFF) {
					uint64_t 	dst_val = read_reg(dst_reg, dst_acc);
					write_reg(dst_reg, dst_acc, dst_val ^ instrc.imm64);
				} else {
					uint8_t 	src_reg = get_index(instrc.src);
					uint8_t 	src_acc = get_access(instrc.src);
					uint64_t 	dst_val = read_reg(dst_reg, dst_acc);
					uint64_t 	src_val = read_reg(src_reg, src_acc);
					write_reg(dst_reg, dst_acc, dst_val ^ src_val);
				}
				break;
			}

		case OPCODE_NOT:{
				uint8_t 	dst_reg = get_index(instrc.dst);
				uint8_t 	dst_acc = get_access(instrc.dst);

				if (instrc.src == 0xFF) {
					write_reg(dst_reg, dst_acc, ~instrc.imm64);
				} else {
					uint8_t 	src_reg = get_index(instrc.src);
					uint8_t 	src_acc = get_access(instrc.src);
					uint64_t 	src_val = read_reg(src_reg, src_acc);
					write_reg(dst_reg, dst_acc, ~src_val);
				}
				break;
			}

		case OPCODE_SHR:{
				uint8_t 	dst_reg = get_index(instrc.dst);
				uint8_t 	dst_acc = get_access(instrc.dst);

				if (instrc.src == 0xFF) {
					uint64_t 	dst_val = read_reg(dst_reg, dst_acc);
					write_reg(dst_reg, dst_acc, dst_val >> instrc.imm64);
				} else {
					uint8_t 	src_reg = get_index(instrc.src);
					uint8_t 	src_acc = get_access(instrc.src);
					uint64_t 	dst_val = read_reg(dst_reg, dst_acc);
					uint64_t 	src_val = read_reg(src_reg, src_acc);
					write_reg(dst_reg, dst_acc, dst_val >> src_val);
				}
				break;
			}

		case OPCODE_SHL:{
				uint8_t 	dst_reg = get_index(instrc.dst);
				uint8_t 	dst_acc = get_access(instrc.dst);

				if (instrc.src == 0xFF) {
					uint64_t 	dst_val = read_reg(dst_reg, dst_acc);
					write_reg(dst_reg, dst_acc, dst_val << instrc.imm64);
				} else {
					uint8_t 	src_reg = get_index(instrc.src);
					uint8_t 	src_acc = get_access(instrc.src);
					uint64_t 	dst_val = read_reg(dst_reg, dst_acc);
					uint64_t 	src_val = read_reg(src_reg, src_acc);
					write_reg(dst_reg, dst_acc, dst_val << src_val);
				}
				break;
			}

		case OPCODE_INC:{
				uint8_t 	dst_reg = get_index(instrc.dst);
				uint8_t 	dst_acc = get_access(instrc.dst);
				uint64_t 	dst_val = read_reg(dst_reg, dst_acc);
				write_reg(dst_reg, dst_acc, dst_val + 1);
				break;
			}

		case OPCODE_DEC:{
				uint8_t 	dst_reg = get_index(instrc.dst);
				uint8_t 	dst_acc = get_access(instrc.dst);
				uint64_t 	dst_val = read_reg(dst_reg, dst_acc);
				write_reg(dst_reg, dst_acc, dst_val - 1);
				break;
			}

		case OPCODE_CMP:{
				uint64_t 	val1 , val2;
				if (instrc.src != 0xFF) {
					uint8_t 	src_reg = get_index(instrc.src);
					uint8_t 	src_acc = get_access(instrc.src);
					val1 = read_reg(src_reg, src_acc);
				} else {
					val1 = instrc.imm64;
				}

				if (instrc.dst != 0xFF) {
					uint8_t 	dst_reg = get_index(instrc.dst);
					uint8_t 	dst_acc = get_access(instrc.dst);
					val2 = read_reg(dst_reg, dst_acc);
				} else {
					val2 = instrc.imm64;
				}

				int64_t 	result = (int64_t) val1 - (int64_t) val2;
				flags.zero = (result == 0);
				flags.sign = (result < 0);
				flags.greater = (result > 0);
				break;
			}
		case OPCODE_JE:
			if (flags.zero) {
				pc = instrc.imm64;
				fseek(binfile, header.section_code + (pc * sizeof(Instruction)),
				      SEEK_SET);
				continue;
			}
			break;

		case OPCODE_JNE:
			if (!flags.zero) {
				pc = instrc.imm64;
				fseek(binfile, header.section_code + (pc * sizeof(Instruction)),
				      SEEK_SET);
				continue;
			}
			break;

		case OPCODE_JL:
			if (flags.sign) {
				pc = instrc.imm64;
				fseek(binfile, header.section_code + (pc * sizeof(Instruction)),
				      SEEK_SET);
				continue;
			}
			break;

		case OPCODE_JG:
			if (flags.greater) {
				pc = instrc.imm64;
				fseek(binfile, header.section_code + (pc * sizeof(Instruction)),
				      SEEK_SET);
				continue;
			}
			break;
		case OPCODE_PUSH:{
				if (sppush >= 256) {
					printf("STACK OVERFLOW\n");
					fclose(binfile);
					return;
				}
				if (instrc.dst == 0xFF) {
					push_stack[sppush].u64 = instrc.imm64;
					push_stack[sppush].type = VAL;
				} else {
					uint8_t 	src_reg = get_index(instrc.dst);
					uint8_t 	src_acc = get_access(instrc.dst);
					uint64_t 	val = read_reg(src_reg, src_acc);

					push_stack[sppush].u64 = val;
					push_stack[sppush].type = regsc[src_reg].type;
				}

				sppush++;
				break;
			}

		case OPCODE_POP:{
				if (sppush <= 0) {
					printf("STACK UNDERFLOW\n");
					fclose(binfile);
					return;
				}
				uint8_t 	dst_reg = get_index(instrc.dst);
				uint8_t 	dst_acc = get_access(instrc.dst);

				uint64_t 	val = push_stack[sppush - 1].u64;
				write_reg(dst_reg, dst_acc, val);

				regsc[dst_reg].type = push_stack[sppush - 1].type;

				sppush--;
				break;
			}

		case OPCODE_JMP:
			pc = instrc.imm64;
			fseek(binfile, header.section_code + (pc * sizeof(Instruction)),
			      SEEK_SET);
			continue;

		case OPCODE_CALL:
			call_stack[spcall++] = pc + 1;
			pc = instrc.imm64;
			fseek(binfile, header.section_code + (pc * sizeof(Instruction)),
			      SEEK_SET);
			continue;
		case OPCODE_RET:
			if (spcall > 0) {
				pc = call_stack[--spcall];
				fseek(binfile, header.section_code + (pc * sizeof(Instruction)),
				      SEEK_SET);
				continue;
			} else {
				printf("CCPU READ ERROR: RET USAGE UNDEFINED");
				fclose(binfile);

				for (int i = 0; i < header.bss_count; i++) {
					free(zmemory[bss[i].bss_id]);
				}
				return;
			}
			break;
		case OPCODE_NOP:
			break;
		case OPCODE_HLT:
			fclose(binfile);
			for (int i = 0; i < header.bss_count; i++) {
				free(zmemory[bss[i].bss_id]);
			}
			return;
		case OPCODE_SYSCALL:{
				uint64_t 	syscall_id = 0;
				if (instrc.dst == 0xFF) {
					syscall_id = instrc.imm64;
				} else {
					uint8_t 	id_reg = get_index(instrc.dst);
					uint8_t 	id_acc = get_access(instrc.dst);
					syscall_id = read_reg(id_reg, id_acc);
				}

				long 		current_pos = ftell(binfile);

				uint64_t 	args  [6] = {0};
				for (int i = 0; i < 6; i++) {
					uint8_t 	acc = REG64_FULL;
					uint64_t 	val = read_reg(i, acc);

					if (regsc[i].type == PTR) {
						void           *resolved = resolve_ptr(val, &header, bss);
						args[i] = (uint64_t) resolved;
						write_reg(i, acc, 0);
					} else {
						args[i] = val;
						write_reg(i, acc, 0);
					}
				}
				#if defined(__linux__)
				long ret = syscall(syscall_id, args[0], args[1], args[2], args[3],
						       args[4], args[5]);
				#else
				long 		ret = e_syscall(syscall_id, args[0], args[1], args[2], args[3],
						       args[4], args[5]);
				#endif

				regsc[6].u64 = ret;
				fseek(binfile, current_pos, SEEK_SET);
				break;
			}
		case OPCODE_PRINT:{
				uint8_t 	reg = get_index(instrc.dst);
				uint8_t 	acc = get_access(instrc.dst);
				uint64_t 	val = read_reg(reg, acc);
				printf("DEBUG: %ld\n", val);
				break;
			}

		case OPCODE_LOAD:{
				uint8_t 	dst_reg = get_index(instrc.dst);
				uint8_t 	dst_acc = get_access(instrc.dst);
				uint8_t 	src_reg = get_index(instrc.src);
				uint8_t 	src_acc = get_access(instrc.src);

				uint64_t 	addr = read_reg(src_reg, src_acc);
				if (regsc[src_reg].type != PTR) {
					printf("LOAD: Invalid pointer access, REGISTER ISN'T POINTER\n");
					return;
				}
				void           *ptr = resolve_ptr(addr, &header, bss);
				if (ptr == NULL) {
					printf("LOAD: Invalid memory access at address %lx\n", addr);
					fclose(binfile);
					for (int i = 0; i < header.bss_count; i++) {
						free(zmemory[bss[i].bss_id]);
					}
					return;
				}
				write_reg(dst_reg, dst_acc, *(uint8_t *) ptr);
				regsc[dst_reg].type = VAL;
				break;
			}

		case OPCODE_STORE:{
				uint8_t 	src_reg = get_index(instrc.src);
				uint8_t 	src_acc = get_access(instrc.src);

				uint64_t 	addr;
				if (instrc.dst == 0xAD) {
					addr = instrc.imm64;
				} else {
					uint8_t 	dst_reg = get_index(instrc.dst);
					uint8_t 	dst_acc = get_access(instrc.dst);
					if (regsc[dst_reg].type != PTR) {
						printf("STORE: Invalid pointer access, REGISTER ISN'T POINTER\n");
						return;
					}
					addr = read_reg(dst_reg, dst_acc);
				}

				addr += regsc[10].u64;

				void           *ptr = resolve_ptr(addr, &header, bss);
				if (ptr == NULL) {
					printf("STORE: Invalid memory access at address %lx\n", addr);
					fclose(binfile);
					for (int i = 0; i < header.bss_count; i++) {
						free(zmemory[bss[i].bss_id]);
					}
					return;
				}
				uint64_t 	val = read_reg(src_reg, src_acc);
				*(uint8_t *) ptr = (uint8_t) val;
				break;
			}

		case OPCODE_ENTRY:
			pc = instrc.imm64;
			fseek(binfile, header.section_code + (pc * sizeof(Instruction)),
			      SEEK_SET);
			continue;
		default:
			continue;
		}

		pc++;
	}

	for (int i = 0; i < header.bss_count; i++) {
		free(zmemory[bss[i].bss_id]);
	}
	fclose(binfile);
}
