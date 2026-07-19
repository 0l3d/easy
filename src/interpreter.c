#include "easy.h"
#include <SDL2/SDL.h>
#include <inttypes.h>
#include <iso646.h>
#include <pthread.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>


// ROM EMULATION SIZE
#define EMULATED_ROM_SIZE 10240
// MEMORY EMULATION SIZE
#define EMULATED_MEMORY_SIZE 131072
// DISK EMULATION SIZE
#define EMULATED_DISK_SIZE 524288

// MMU
#define MEMORY_PAGE_SIZE 512;

// PRIVATE REGS EMULATION
uint64_t        syscall_decoder_position = 0;
uint64_t        disk_index_position = 0;
// PRIVATE REGS EMULATION

// EMULATOR DEBUGGER
int             enable_debug_console = 0;


// PORTS
#define ROM_SIZE 8
#define DISK_INDEX 9
#define DISK_BYTE 10
#define KEYBOARD 11
#define MOUSE 12
#define MEMORY_END 13
#define FRAMEBUFFER_START 14
#define FRAMEBUFFER_SIZE 15
#define DISK_END 18
#define CLOCK_TIMER_SET 19
#define USB_DEVICES 20
// PORTS

// SDL Handling 
char            SDL_keyboard_handling = 0;
// SDL Handling

// SYSCALL RETURN POS REGISTER: R7

// CPU COMPARING FLAGS
Flags           flags = { 0 };

// CPU COMPARING FLAGS

// For Memory Management Unit, can changeable by software.(Bootloader, Kernel,
// Micro-controller).
// SL - 0 - Raw Mode (Bootloader, Kernel, Micro-controller).
// SL - 1 - Strict Mode (User-space software)
// AND Flags. (Disk, Memory, Keyboard-Mouse-USB-Dev);
CPU             cpu;

// EMULATED ROM, MEMORY AND DISK
uint8_t         rom[EMULATED_ROM_SIZE];
uint8_t         memory[EMULATED_MEMORY_SIZE];
uint8_t         disk[EMULATED_DISK_SIZE];
// EMULATED ROM, MEMORY AND DISK

uint64_t
read_reg(uint8_t idx, RegAccessType access)
{
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

void
write_reg(uint8_t idx, RegAccessType access, uint64_t val)
{
	switch (access) {
	case REG64_FULL:
		cpu.reg[idx].u64 = val;
		break;
	case REG32_LOW:
		cpu.reg[idx].low32 = (uint32_t) val;
		break;
	case REG32_HIGH:
		cpu.reg[idx].high32 = (uint32_t) val;
		break;
	case REG16_LOW:
		cpu.reg[idx].low16 = (uint16_t) val;
		break;
	case REG16_MIDLOW:
		cpu.reg[idx].midlow16 = (uint16_t) val;
		break;
	case REG16_MIDHIGH:
		cpu.reg[idx].midhigh16 = (uint16_t) val;
		break;
	case REG16_HIGH:
		cpu.reg[idx].high16 = (uint16_t) val;
		break;
	case REG8_B0:
		cpu.reg[idx].b0 = (uint8_t) val;
		break;
	case REG8_B1:
		cpu.reg[idx].b1 = (uint8_t) val;
		break;
	case REG8_B2:
		cpu.reg[idx].b2 = (uint8_t) val;
		break;
	case REG8_B3:
		cpu.reg[idx].b3 = (uint8_t) val;
		break;
	case REG8_B4:
		cpu.reg[idx].b4 = (uint8_t) val;
		break;
	case REG8_B5:
		cpu.reg[idx].b5 = (uint8_t) val;
		break;
	case REG8_B6:
		cpu.reg[idx].b6 = (uint8_t) val;
		break;
	case REG8_B7:
		cpu.reg[idx].b7 = (uint8_t) val;
		break;
	}
}

size_t
access_size(RegAccessType acc)
{
	switch (acc) {
	case REG64_FULL:
		return 8;

	case REG32_LOW:
	case REG32_HIGH:
		return 4;

	case REG16_LOW:
	case REG16_MIDLOW:
	case REG16_MIDHIGH:
	case REG16_HIGH:
		return 2;

	default:
		return 1;
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

// INTERRUPT REGISTER: R2
// Keeps interrupt function position.
// When got an interrupt, system go to interrupt function.
// INTERRUPT ERROR CODE REGISTER: R4
// INTERRUPT RETURN REGISTER: R7
CPU_SL
get_cpu_sl()
{
	CPU_SL          return_val;
	memcpy(&return_val, &memory[cpu.reg[2].u64], sizeof(CPU_SL));
	return return_val;
}

// MMU - Memory Management Unit
// r6 -> MMU Paging Table Pointer
// Memory: 512 byte page. Program usable: 511 bytes.
// Block Layout: 1 Byte Block INFO (IS Free | Executable), 511 Byte usable
#define FREE 1
#define EXEC 2
void           *
resolve_ptr(uint64_t ptr)
{
	uint8_t        *m_addr = NULL;

	if (ptr < EMULATED_ROM_SIZE) {
		m_addr = rom;
	}
	else {
		ptr = ptr - EMULATED_ROM_SIZE;
		m_addr = memory;
	}

	if (cpu.reg[1].b0 == 0) {
		return m_addr + ptr;
	}

	if (enable_debug_console == 1) {
		printf("WARNING: MMU is enabled!\n");
	}

	uint64_t        addr = cpu.reg[6].u64;
	uint64_t        page_index = ptr / MEMORY_PAGE_SIZE;
	uint64_t        page_size = ptr % MEMORY_PAGE_SIZE;

	uint64_t        table_info;
	memcpy(&table_info, &memory[addr], sizeof(table_info));

	if (table_info >= page_index) {
		uint64_t        page_ind_addr =
			(page_index * sizeof(uint64_t)) + sizeof(table_info);
		uint64_t        physical_addr;
		memcpy(&physical_addr, &memory[page_ind_addr],
		       sizeof(page_ind_addr));
		if (memory[page_ind_addr] & EXEC) {
			cpu.reg[4].u64 = 0;	// TODO: ERROR CODE
			if (enable_debug_console == 1)
				printf("INTERRUPT: Code tries to store/load executable memory address.\n");
			return NULL;
		}
		return memory + physical_addr + page_size;
	}
	else {
		printf("INTERRUPT: MMU not found exact page.\n");
		cpu.reg[4].u64 = 0;	// TODO: ERROR CODE
		return NULL;
	}
	return NULL;
}

// MMU - End

int             emulator_running = 0;

uint64_t        timer = 0;

int             WINDOW_WIDTH = 512;
int             WINDOW_HEIGHT = 512;
int             framebuffer_size = 0;
int             starting_framebuffer_pos = 0;
int             fb_size_set = 0;

pthread_mutex_t fb_mutex = PTHREAD_MUTEX_INITIALIZER;

// Monitor - Framebuffer rendering.
// FrameBuffer Address is end 64kb of memory.
// Color Layout: RGB
void           *
create_framebuffer_window(void
			  *No_One_Know_Where_He_Came_From_He_Never_Knew_Himself)
{
	SDL_Event       event;
	SDL_Renderer   *renderer;
	SDL_Window     *window;

	uint8_t        *local_buffer;
	int             first_time_allocation = 1;

	SDL_Init(SDL_INIT_VIDEO);
	SDL_CreateWindowAndRenderer(WINDOW_WIDTH, WINDOW_HEIGHT,
				    SDL_WINDOW_SHOWN, &window, &renderer);
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
	SDL_RenderClear(renderer);

	while (emulator_running) {
		while (SDL_PollEvent(&event)) {
			SDL_KeyboardEvent key = event.key;
			switch (event.type) {
			case SDL_KEYDOWN:
			case SDL_KEYUP:
				if (key.type == SDL_KEYUP) {
					SDL_keyboard_handling = 0;
				}
				else {
					SDL_keyboard_handling =
						key.keysym.sym;
				}
			default:
				break;
			}

			if (event.type == SDL_QUIT)
				emulator_running = 0;
		}
		if (fb_size_set == 1) {
			if (first_time_allocation == 1) {
				local_buffer = malloc(framebuffer_size);
				first_time_allocation = 0;
			}
			pthread_mutex_lock(&fb_mutex);
			memcpy(local_buffer,
			       &memory[starting_framebuffer_pos],
			       framebuffer_size);
			pthread_mutex_unlock(&fb_mutex);

			int             i = 0;
			SDL_RenderClear(renderer);
			for (int y = 0; y < WINDOW_HEIGHT; y++) {
				for (int x = 0; x < WINDOW_WIDTH; x++) {
					uint8_t         color =
						local_buffer[i++];
					SDL_SetRenderDrawColor(renderer,
							       color, color,
							       color, 255);
					SDL_RenderDrawPoint(renderer, x, y);
				}
			}

			SDL_RenderPresent(renderer);
			SDL_Delay(16);
		}
	}

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
	return 0;
}

void
interpret_easy64(const char *binname, char *arguments_string)
{
	pthread_t       video_thread;
	cpu.sl.mode = 0;
	// STACK POINTER: r0
	cpu.reg[0].u64 = 0;
	FILE           *binfile = fopen(binname, "rb");
	if (binfile == NULL) {
		perror("cpu is failed to open binary file");
		return;
	}
	memset(cpu.reg, 0, sizeof(cpu.reg));

	uint64_t        pc = 0;
	emulator_running = 1;

	char           *disk_name = NULL;


	if (arguments_string != NULL) {
		if (strchr(arguments_string, ',')) {
			char           *out = strtok(arguments_string, ",");
			while (out != NULL) {
				if (strcmp(out, "debug") == 0) {
					enable_debug_console = 1;
				}
				else {
					disk_name = strdup(out);
				}
				out = strtok(NULL, ",");
			}
		}

		if (strcmp(arguments_string, "debug") == 0) {
			enable_debug_console = 1;
		}
		else {
			disk_name = strdup(arguments_string);
		}

		if (disk_name != NULL) {
			FILE           *kernel_file = fopen(disk_name, "rb");
			if (binfile == NULL) {
				perror("cpu is failed to open binary file");
				return;
			}
			fseek(kernel_file, 0, SEEK_END);
			long            size = ftell(kernel_file);
			rewind(kernel_file);
			fread(&disk, 1, size, kernel_file);
			fclose(kernel_file);
		}
	}

	if (enable_debug_console == 1) {
		printf("n -> run and next\ns -> timer next \ni -> set specific instruction breakpoint\ng -> set pc to specific pos\nc -> print current instrc \nv -> print register values \nm -> memory dump to easy_memory.dump \nd -> disk dump to easy_disk.img \nr -> rom dump to easy_rom.bin \nq -> stop debugging and stop emulator\nf -> print private values\n");
		printf("USAGE: \ni: i instrc_code imm src dst\ns: s howmany?\ng: g whichpos?\n");
	}

	int             debug_timer = 0;
	Opcode          deb_instruction;
	uint64_t        debug_imm = 0;
	uint16_t        debug_src = 0;
	uint16_t        debug_dst = 0;
	int             instruction_bp_enabled = 0;
	framebuffer_size = 0;


	pthread_create(&video_thread, NULL, create_framebuffer_window, NULL);
	fread(&rom, 1, EMULATED_ROM_SIZE, binfile);
	while (emulator_running == 1) {
		Instruction     instrc;
		void           *instrc_ptr = resolve_ptr(pc);
		if (instrc_ptr == NULL) {
			if (enable_debug_console == 1)
				printf("INTERRUPT: Undefined PC address: %lu\n", pc);
			emulator_running = 0;
			break;
		}
		memcpy(&instrc, instrc_ptr, sizeof(Instruction));
		if (enable_debug_console == 1) {
			printf("Current instrc op: %x, imm: %lu, src: %d, dst: %d, PC: %lu\n", instrc.opcode, instrc.imm64, instrc.src, instrc.dst, pc);

			/* Commands: 
			 * n -> next
			 * s -> timer next
			 * f -> print private values
			 * i -> set specific instruction break point
			 * g -> set PC to specific position
			 * p -> print register
			 * c -> current instrc
			 * v -> register print
			 * m -> memory dump 
			 * r -> rom dump
			 * d -> disk dump
			 * q -> stop debugging
			 */
			int             con = 0;
			while (con == 0) {
				if (debug_timer > 0) {
					debug_timer--;
					break;
				}
				if (instruction_bp_enabled == 1) {
					if (instrc.opcode == deb_instruction
					    && instrc.imm64 == debug_imm
					    && instrc.src == debug_src
					    && instrc.dst == debug_dst) {
						instruction_bp_enabled = 0;
						printf("Breakpoint reached!\n");
					}
					else {
						break;
					}
				}

				char            command;
				printf("\nDEBUGGER >");
				scanf("%c", &command);
				switch (command) {
				case 'n':
					printf("NEXT!\n");
					con = 1;
					break;
				case 'g':
					uint64_t pos;
					scanf("%lu", &pos);
					pc = pos;
					printf("New Program Counter position: %lu\n", pc);
					break;
				case 'f':
					printf("EMULATOR RUNNING: %d\n",
					       emulator_running);
					printf("TIMER REGISTER VALUE: %lu\n",
					       timer);
					printf("FRAMEBUFFER_SIZE: %d\n",
					       framebuffer_size);
					printf("FRAMEBUFFER_START_POS: %d\n",
					       starting_framebuffer_pos);
					printf("IS FB SIZE SET?: %d\n",
					       fb_size_set);
					printf("SYSCALL_DECODER_POSITION: %lu\n", syscall_decoder_position);
					printf("DISK_INDEX_POSITION: %lu\n",
					       disk_index_position);
					break;
				case 'i':
					instruction_bp_enabled = 1;
					scanf("%x", &deb_instruction);
					scanf("%lu", &debug_imm);
					scanf("%hu", &debug_src);
					scanf("%hu", &debug_dst);
					break;
				case 's':
					int             how_many;
					scanf("%d", &how_many);
					debug_timer = how_many;
					break;
				case 'c':
					printf("Current instrc op: %x, imm: %lu, src: %d, dst: %d, PC: %lu\n", instrc.opcode, instrc.imm64, instrc.src, instrc.dst, pc);
					break;
				case 'v':
					for (int i = 0; i < MAX_REGS; i++) {
						printf("R%d: %lu\n", i,
						       cpu.reg[i].u64);
					}
					printf("Registers printed.");
					break;
				case 'm':
					FILE * memory_dump_file =
						fopen("easy_memory.dump",
						      "wb");
					if (memory_dump_file == NULL) {
						perror("Easy failed to open 'easy_memory.dump' file during memory dump:");
						continue;
					}

					fwrite(memory, 1,
					       EMULATED_MEMORY_SIZE,
					       memory_dump_file);

					fclose(memory_dump_file);
					break;
				case 'd':
					FILE * disk_dump_file =
						fopen("easy_disk.img", "wb");
					if (disk_dump_file == NULL) {
						perror("Easy failed to open 'easy_disk.img' file during disk dump:");
						continue;
					}

					fwrite(disk, 1, EMULATED_DISK_SIZE,
					       disk_dump_file);

					fclose(disk_dump_file);
					break;
				case 'r':
					FILE * rom_dump_file =
						fopen("easy_rom.bin", "wb");
					if (rom_dump_file == NULL) {
						perror("Easy failed to open 'easy_rom.bin' file during rom dump:");
						continue;
					}

					fwrite(rom, 1, EMULATED_ROM_SIZE,
					       rom_dump_file);

					fclose(rom_dump_file);
					break;
				case 'q':
					con = 1;
					emulator_running = 0;
					break;
				}
			}
		}

		switch (instrc.opcode) {
		case OPCODE_MOV:
			if (instrc.src == 0xFFFF) {
				uint8_t         dst_reg =
					get_index(instrc.dst);
				uint8_t         access_type =
					get_access(instrc.dst);
				write_reg(dst_reg, access_type, instrc.imm64);
			}
			else {
				uint8_t         src_reg =
					get_index(instrc.src);
				uint8_t         src_acc =
					get_access(instrc.src);
				uint8_t         dst_reg =
					get_index(instrc.dst);
				uint8_t         dst_acc =
					get_access(instrc.dst);
				uint64_t        val =
					read_reg(src_reg, src_acc);
				write_reg(dst_reg, dst_acc, val);
			}
			break;
		case OPCODE_ADD:{
				uint8_t         dst_reg =
					get_index(instrc.dst);
				uint8_t         dst_acc =
					get_access(instrc.dst);
				if (instrc.src == 0xFFFF) {
					uint64_t        dst_val =
						read_reg(dst_reg, dst_acc);
					uint64_t        result =
						dst_val + instrc.imm64;
					write_reg(dst_reg, dst_acc, result);
				}
				else {
					uint8_t         src_reg =
						get_index(instrc.src);
					uint8_t         src_acc =
						get_access(instrc.src);
					uint64_t        adder =
						read_reg(src_reg, src_acc);
					uint64_t        addend =
						read_reg(dst_reg, dst_acc);
					uint64_t        result =
						addend + adder;
					write_reg(dst_reg, dst_acc, result);
				}
				break;
			}
		case OPCODE_SUB:{
				uint8_t         dst_reg =
					get_index(instrc.dst);
				uint8_t         dst_acc =
					get_access(instrc.dst);

				if (instrc.src == 0xFFFF) {
					uint64_t        dst_val =
						read_reg(dst_reg, dst_acc);
					write_reg(dst_reg, dst_acc,
						  dst_val - instrc.imm64);
				}
				else {
					uint8_t         src_reg =
						get_index(instrc.src);
					uint8_t         src_acc =
						get_access(instrc.src);
					uint64_t        dst_val =
						read_reg(dst_reg, dst_acc);
					uint64_t        src_val =
						read_reg(src_reg, src_acc);
					write_reg(dst_reg, dst_acc,
						  dst_val - src_val);
				}
				break;
			}
		case OPCODE_MUL:{
				uint8_t         dst_reg =
					get_index(instrc.dst);
				uint8_t         dst_acc =
					get_access(instrc.dst);

				if (instrc.src == 0xFFFF) {
					uint64_t        dst_val =
						read_reg(dst_reg, dst_acc);
					write_reg(dst_reg, dst_acc,
						  dst_val * instrc.imm64);
				}
				else {
					uint8_t         src_reg =
						get_index(instrc.src);
					uint8_t         src_acc =
						get_access(instrc.src);
					uint64_t        dst_val =
						read_reg(dst_reg, dst_acc);
					uint64_t        src_val =
						read_reg(src_reg, src_acc);
					write_reg(dst_reg, dst_acc,
						  dst_val * src_val);
				}
				break;
			}
		case OPCODE_DIV:{
				uint8_t         dst_reg =
					get_index(instrc.dst);
				uint8_t         dst_acc =
					get_access(instrc.dst);

				if (instrc.src == 0xFFFF) {
					if (instrc.imm64 == 0) {
						cpu.reg[7].u64 = pc;
						pc = cpu.reg[2].u64;
						cpu.reg[4].u64 = 10;
						continue;
					}
					uint64_t        dst_val =
						read_reg(dst_reg, dst_acc);
					write_reg(dst_reg, dst_acc,
						  dst_val / instrc.imm64);
					write_reg(dst_reg + 1, dst_acc,
						  dst_val % instrc.imm64);
				}
				else {
					uint8_t         src_reg =
						get_index(instrc.src);
					uint8_t         src_acc =
						get_access(instrc.src);
					uint64_t        dst_val =
						read_reg(dst_reg, dst_acc);
					uint64_t        src_val =
						read_reg(src_reg, src_acc);
					if (src_val == 0) {
						cpu.reg[7].u64 = pc;
						pc = cpu.reg[2].u64;
						cpu.reg[4].u64 = 10;
						continue;
					}
					write_reg(dst_reg, dst_acc,
						  dst_val / src_val);
					write_reg(dst_reg + 1, dst_acc,
						  dst_val % src_val);
				}
				break;
			}
		case OPCODE_AND:{
				uint8_t         dst_reg =
					get_index(instrc.dst);
				uint8_t         dst_acc =
					get_access(instrc.dst);

				if (instrc.src == 0xFFFF) {
					uint64_t        dst_val =
						read_reg(dst_reg, dst_acc);
					write_reg(dst_reg, dst_acc,
						  dst_val & instrc.imm64);
				}
				else {
					uint8_t         src_reg =
						get_index(instrc.src);
					uint8_t         src_acc =
						get_access(instrc.src);
					uint64_t        dst_val =
						read_reg(dst_reg, dst_acc);
					uint64_t        src_val =
						read_reg(src_reg, src_acc);
					write_reg(dst_reg, dst_acc,
						  dst_val & src_val);
				}
				break;
			}

		case OPCODE_OR:{
				uint8_t         dst_reg =
					get_index(instrc.dst);
				uint8_t         dst_acc =
					get_access(instrc.dst);

				if (instrc.src == 0xFFFF) {
					uint64_t        dst_val =
						read_reg(dst_reg, dst_acc);
					write_reg(dst_reg, dst_acc,
						  dst_val | instrc.imm64);
				}
				else {
					uint8_t         src_reg =
						get_index(instrc.src);
					uint8_t         src_acc =
						get_access(instrc.src);
					uint64_t        dst_val =
						read_reg(dst_reg, dst_acc);
					uint64_t        src_val =
						read_reg(src_reg, src_acc);
					write_reg(dst_reg, dst_acc,
						  dst_val | src_val);
				}
				break;
			}

		case OPCODE_XOR:{
				uint8_t         dst_reg =
					get_index(instrc.dst);
				uint8_t         dst_acc =
					get_access(instrc.dst);

				if (instrc.src == 0xFFFF) {
					uint64_t        dst_val =
						read_reg(dst_reg, dst_acc);
					write_reg(dst_reg, dst_acc,
						  dst_val ^ instrc.imm64);
				}
				else {
					uint8_t         src_reg =
						get_index(instrc.src);
					uint8_t         src_acc =
						get_access(instrc.src);
					uint64_t        dst_val =
						read_reg(dst_reg, dst_acc);
					uint64_t        src_val =
						read_reg(src_reg, src_acc);
					write_reg(dst_reg, dst_acc,
						  dst_val ^ src_val);
				}
				break;
			}

		case OPCODE_NOT:{
				uint8_t         dst_reg =
					get_index(instrc.dst);
				uint8_t         dst_acc =
					get_access(instrc.dst);

				if (instrc.src == 0xFFFF) {
					write_reg(dst_reg, dst_acc,
						  ~instrc.imm64);
				}
				else {
					uint8_t         src_reg =
						get_index(instrc.src);
					uint8_t         src_acc =
						get_access(instrc.src);
					uint64_t        src_val =
						read_reg(src_reg, src_acc);
					write_reg(dst_reg, dst_acc, ~src_val);
				}
				break;
			}

		case OPCODE_SHR:{
				uint8_t         dst_reg =
					get_index(instrc.dst);
				uint8_t         dst_acc =
					get_access(instrc.dst);

				if (instrc.src == 0xFFFF) {
					uint64_t        dst_val =
						read_reg(dst_reg, dst_acc);
					write_reg(dst_reg, dst_acc,
						  dst_val >> instrc.imm64);
				}
				else {
					uint8_t         src_reg =
						get_index(instrc.src);
					uint8_t         src_acc =
						get_access(instrc.src);
					uint64_t        dst_val =
						read_reg(dst_reg, dst_acc);
					uint64_t        src_val =
						read_reg(src_reg, src_acc);
					write_reg(dst_reg, dst_acc,
						  dst_val >> src_val);
				}
				break;
			}

		case OPCODE_SHL:{
				uint8_t         dst_reg =
					get_index(instrc.dst);
				uint8_t         dst_acc =
					get_access(instrc.dst);

				if (instrc.src == 0xFFFF) {
					uint64_t        dst_val =
						read_reg(dst_reg, dst_acc);
					write_reg(dst_reg, dst_acc,
						  dst_val << instrc.imm64);
				}
				else {
					uint8_t         src_reg =
						get_index(instrc.src);
					uint8_t         src_acc =
						get_access(instrc.src);
					uint64_t        dst_val =
						read_reg(dst_reg, dst_acc);
					uint64_t        src_val =
						read_reg(src_reg, src_acc);
					write_reg(dst_reg, dst_acc,
						  dst_val << src_val);
				}
				break;
			}

		case OPCODE_INC:{
				uint8_t         dst_reg =
					get_index(instrc.dst);
				uint8_t         dst_acc =
					get_access(instrc.dst);
				uint64_t        dst_val =
					read_reg(dst_reg, dst_acc);
				write_reg(dst_reg, dst_acc, dst_val + 1);
				break;
			}

		case OPCODE_DEC:{
				uint8_t         dst_reg =
					get_index(instrc.dst);
				uint8_t         dst_acc =
					get_access(instrc.dst);
				uint64_t        dst_val =
					read_reg(dst_reg, dst_acc);
				write_reg(dst_reg, dst_acc, dst_val - 1);
				break;
			}

		case OPCODE_CMP:{
				uint64_t        val1, val2;
				if (instrc.src != 0xFFFF) {
					uint8_t         src_reg =
						get_index(instrc.src);
					uint8_t         src_acc =
						get_access(instrc.src);
					val1 = read_reg(src_reg, src_acc);
				}
				else {
					val1 = instrc.imm64;
				}

				if (instrc.dst != 0xFFFF) {
					uint8_t         dst_reg =
						get_index(instrc.dst);
					uint8_t         dst_acc =
						get_access(instrc.dst);
					val2 = read_reg(dst_reg, dst_acc);
				}
				else {
					val2 = instrc.imm64;
				}

				int64_t         result =
					(int64_t) val1 - (int64_t) val2;
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
		case OPCODE_PUSH:{
				uint64_t        val;

				uint8_t         reg = get_index(instrc.dst);
				uint8_t         acc = get_access(instrc.dst);

				size_t          sz =
					access_size((RegAccessType) acc);

				val = read_reg(reg, acc);

				memcpy(&memory[cpu.reg[0].u64], &val, sz);

				cpu.reg[0].u64 -= sz;

				break;
			}
		case OPCODE_SYSCALL:{
				cpu.reg[7].u64 = pc;
				pc = syscall_decoder_position;
				cpu.sl.mode = 0;
				continue;
			}
		case OPCODE_SYSRET:{
				pc = cpu.reg[7].u64;
				cpu.sl.mode = 1;
				continue;
			}
		case OPCODE_INB:{
				uint8_t         dst_reg =
					get_index(instrc.dst);
				uint8_t         access_type =
					get_access(instrc.dst);
				if (instrc.imm64 == DISK_INDEX) {
					write_reg(dst_reg, access_type,
						  disk_index_position);
				}
				else if (instrc.imm64 == DISK_BYTE) {
					write_reg(dst_reg, access_type,
						  disk[disk_index_position]);
					disk_index_position++;
				}
				else if (instrc.imm64 == FRAMEBUFFER_START) {
					write_reg(dst_reg, access_type,
						  starting_framebuffer_pos);
				}
				else if (instrc.imm64 == FRAMEBUFFER_SIZE) {
					write_reg(dst_reg, access_type,
						  framebuffer_size);
				}
				else if (instrc.imm64 == KEYBOARD) {
					write_reg(dst_reg, access_type,
						  SDL_keyboard_handling);
				}
				else if (instrc.imm64 == MEMORY_END) {
					write_reg(dst_reg, access_type,
						  EMULATED_MEMORY_SIZE);
				}
				else if (instrc.imm64 == ROM_SIZE) {
					write_reg(dst_reg, access_type,
						  EMULATED_ROM_SIZE);
				}

			}
			break;
		case OPCODE_OUTB:{
				uint8_t         dst_reg =
					get_index(instrc.dst);
				uint8_t         dst_acc =
					get_access(instrc.dst);
				uint64_t        val =
					read_reg(dst_reg, dst_acc);
				if (instrc.imm64 == DISK_INDEX) {
					disk_index_position = val;
				}
				else if (instrc.imm64 == DISK_BYTE) {
					disk[disk_index_position] = val;
				}
				else if (instrc.imm64 == CLOCK_TIMER_SET) {
					timer = val;
				}
				else if (instrc.imm64 == FRAMEBUFFER_START) {
					pthread_mutex_lock(&fb_mutex);
					starting_framebuffer_pos = val;
					pthread_mutex_unlock(&fb_mutex);
				}
				else if (instrc.imm64 == FRAMEBUFFER_SIZE) {
					pthread_mutex_lock(&fb_mutex);
					if (fb_size_set == 0)
						fb_size_set = 1;
					framebuffer_size = val;
					pthread_mutex_unlock(&fb_mutex);
				}
			}
			break;
		case OPCODE_CSL:{
				uint8_t         mode = 0;
				if (instrc.dst != 0xFFFF) {
					uint64_t        val = 0;
					uint8_t         reg =
						get_index(instrc.dst);
					uint8_t         acc =
						get_access(instrc.dst);

					size_t          sz =
						access_size((RegAccessType)
							    acc);

					val = read_reg(reg, acc);
					mode = val;
				}
				else {
					mode = instrc.imm64;
				}
				cpu.sl.mode = mode;
			}
		case OPCODE_SSDP:{
				if (instrc.src != 0xFFFF) {
					uint8_t         dst_reg =
						get_index(instrc.dst);
					uint8_t         dst_acc =
						get_access(instrc.dst);
					uint64_t        val =
						read_reg(dst_reg, dst_acc);
					syscall_decoder_position = val;
				}
				else {
					syscall_decoder_position =
						pc + instrc.imm64;
				}
				continue;
			}
		case OPCODE_POP:{
				uint8_t         reg = get_index(instrc.dst);
				uint8_t         acc = get_access(instrc.dst);

				size_t          sz =
					access_size((RegAccessType) acc);

				cpu.reg[0].u64 += sz;
				uint64_t        val = 0;

				memcpy(&val, &memory[cpu.reg[0].u64], sz);

				write_reg(reg, acc, val);

				break;
			}

		case OPCODE_JMP:
			if (instrc.src != 0xFFFF) {
				uint8_t         dst_reg =
					get_index(instrc.dst);
				uint8_t         dst_acc =
					get_access(instrc.dst);
				uint64_t        val =
					read_reg(dst_reg, dst_acc);
				pc = val;
			}
			else {
				pc = pc + instrc.imm64;
			}
			continue;

		case OPCODE_CALL:{
				uint64_t        ret =
					pc + sizeof(Instruction);
				memcpy(&memory[cpu.reg[0].u64], &ret,
				       sizeof(uint64_t));
				cpu.reg[0].u64 -= sizeof(uint64_t);
				pc = pc + instrc.imm64;
				continue;
			}
		case OPCODE_RET:{
				cpu.reg[0].u64 += sizeof(uint64_t);
				uint64_t        ret;
				memcpy(&ret, &memory[cpu.reg[0].u64],
				       sizeof(uint64_t));
				pc = ret;
				continue;
			}
		case OPCODE_NOP:
			break;
		case OPCODE_HLT:
			emulator_running = 0;
			fclose(binfile);
			return;
		case OPCODE_PRINT:{
				uint8_t         reg = get_index(instrc.dst);
				uint8_t         acc = get_access(instrc.dst);
				uint64_t        val = read_reg(reg, acc);
				printf("DEBUG: %ld\n", val);
				break;
			}

		case OPCODE_LOAD:{
				uint8_t         dst_reg =
					get_index(instrc.dst);
				uint8_t         dst_acc =
					get_access(instrc.dst);
				uint64_t        addr;
				if (instrc.src == 0xFFFF) {
					addr = instrc.imm64;
				}
				else {
					uint8_t         src_reg =
						get_index(instrc.src);
					uint8_t         src_acc =
						get_access(instrc.src);
					addr = read_reg(src_reg, src_acc);
				}

				void           *ptr = resolve_ptr(addr);
				if (ptr == NULL) {
					cpu.reg[7].u64 = pc;
					cpu.reg[4].u64 = 0;	// TODO: ERROR CODE
					if (enable_debug_console == 1)
						printf("INTERRUPT: Undefined pointer at load.\n");
					pc = cpu.reg[2].u64;
					continue;
				}

				uint64_t        val = 0;
				size_t          sz =
					access_size((RegAccessType) dst_acc);

				memcpy(&val, ptr, sz);
				write_reg(dst_reg, dst_acc, val);
				break;
			}

		case OPCODE_STORE:{
				uint8_t         src_reg =
					get_index(instrc.src);
				uint8_t         src_acc =
					get_access(instrc.src);
				uint64_t        addr;

				if (instrc.dst == 0xFFFF) {
					addr = instrc.imm64;
				}
				else {
					uint8_t         dst_reg =
						get_index(instrc.dst);
					uint8_t         dst_acc =
						get_access(instrc.dst);
					addr = read_reg(dst_reg, dst_acc);
				}


				if (addr < EMULATED_ROM_SIZE) {
					cpu.reg[7].u64 = pc;
					cpu.reg[4].u64 = 0;	// TODO: ERROR CODE
					if (enable_debug_console == 1)
						printf("INTERRUPT: Code tries to store ROM.\n");
					pc = cpu.reg[2].u64;
					continue;
				}

				void           *ptr = resolve_ptr(addr);
				if (ptr == NULL) {
					cpu.reg[7].u64 = pc;
					cpu.reg[4].u64 = 0;	// TODO: ERROR CODE
					if (enable_debug_console == 1)
						printf("INTERRUPT: Undefined pointer at store.\n");
					pc = cpu.reg[2].u64;
					continue;
				}
				uint64_t        val =
					read_reg(src_reg, src_acc);
				size_t          sz =
					access_size((RegAccessType) src_acc);
				memcpy(ptr, &val, sz);
				break;
			}

		case OPCODE_ENTRY:
			pc = pc + instrc.imm64;
			continue;
		default:
			continue;
		}

		if (timer != 0 && cpu.reg[1].b1 == 1) {
			timer--;
		}
		else if (timer == 0 && cpu.reg[1].b1 == 1) {
			pc = cpu.reg[5].u64;
		}
		pc += sizeof(Instruction);
	}

	pthread_join(video_thread, NULL);
	fclose(binfile);

	if (emulator_running == 0) {
		free(disk_name);
	}
}
