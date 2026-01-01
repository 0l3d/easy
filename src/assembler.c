#include "easy.h"
#include <ctype.h>
#include <inttypes.h>
#include <pthread.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int 		error_report = 0;
int 		linecounter = 0;

Label 		labels  [300] = {0};
int 		label_count = 0;

BSS 		bss       [ZMEMORY_SIZE] = {0};
int 		bss_count;

Data 		datas    [300] = {0};
int 		data_count = 0;

Section 	sections;

BinaryHeader 	header;

Register64 	regs[NUM_REGS];

int
tokenizer(char *bufin, char *bufout[], int max_count)
{
	int 		token_count = 0;
	const char     *p = bufin;
	while (*p != '\0') {
		if (*p == '#') {
			break;
		} else if (*p == ',') {
			char           *sub = malloc(2);
			sub[0] = ',';
			sub[1] = '\0';
			bufout[token_count++] = sub;
			p++;
		} else if (*p == ':') {
			char           *sub = malloc(2);
			sub[0] = ':';
			sub[1] = '\0';
			bufout[token_count++] = sub;
			p++;
		} else if (isspace(*p)) {
			p++;
			continue;
		} else if (*p == '\0') {
			break;
		} else if (*p == '"') {
			char 		in_string = *p;
			const char     *string_start = ++p;
			while (*p && *p != in_string)
				p++;
			int 		stringlen = p - string_start;
			char           *in_string_tokens = malloc(stringlen + 1);
			strncpy(in_string_tokens, string_start, stringlen);
			in_string_tokens[stringlen] = '\0';
			bufout[token_count++] = in_string_tokens;
			if (*p)
				p++;
		} else {
			const char     *word_start = p;
			while (*p && !isspace(*p) && *p != ',' && *p != ':')
				p++;
			int 		word_len = p - word_start;
			char           *word = malloc(word_len + 1);
			strncpy(word, word_start, word_len);
			word[word_len] = '\0';
			bufout[token_count++] = word;
		}
		if (token_count >= max_count)
			break;
	}
	bufout[token_count] = NULL;
	return token_count;
}

int
section(char **tokens)
{
	if (strcmp(tokens[0], "section") == 0) {
		if (strcmp(tokens[1], "code") == 0) {
			sections = SECTION_CODE;
		} else if (strcmp(tokens[1], "data") == 0) {
			sections = SECTION_DATA;
		} else if (strcmp(tokens[1], "bss") == 0) {
			sections = SECTION_BSS;
		} else {
			printf("Got an error Line %d, Situation: 'undefined section'.",
			       linecounter);
		}
	}
	return 0;
}

void
labelsp(const char *label_name, int pos, LabelType type)
{
	if (label_count == 300) {
		printf("MAXIMUM LABEL : 300 ");
		return;
	}
	strcpy(labels[label_count].name, label_name);
	labels[label_count].address = pos;
	labels[label_count].type = type;
	label_count++;
}

void
datasp(const char *data_label_name, int pos, DataType type)
{
	if (data_count == 300) {
		printf("MAXIMUM DATA : 300");
		return;
	}
	strcpy(datas[data_count].name, data_label_name);
	datas[data_count].addr = pos;
	datas[data_count].type = type;
	data_count++;
}

void
bsssp(const char *bss_label_name, int pos, BssType type, int size)
{
	if (bss_count == 300) {
		printf("MAXIMUM BSS : 300");
		return;
	}
	strcpy(bss[bss_count].name, bss_label_name);
	bss[bss_count].section.bss_id = bss_count;
	bss[bss_count].section.addr = pos;
	bss[bss_count].section.size = size;
	bss[bss_count].section.type = type;
	bss_count++;
}

uint16_t
en_registers(const char *reg_name)
{
	int 		index = -1;
	uint8_t 	main_type = 0;
	RegAccessType 	access = REG64_FULL;

	if (reg_name[0] == 'r') {
		main_type = 0;
		access = REG64_FULL;
		index = atoi(&reg_name[1]);
	} else if (reg_name[0] == 'e') {
		main_type = 1;
		index = atoi(&reg_name[1]);
		if (strstr(reg_name, "l"))
			access = REG32_LOW;
		else if (strstr(reg_name, "h"))
			access = REG32_HIGH;
		else
			access = REG32_LOW;
	} else if (reg_name[0] == 'b') {
		main_type = 2;
		index = atoi(&reg_name[1]);
		if (strstr(reg_name, "ml"))
			access = REG16_MIDLOW;
		else if (strstr(reg_name, "mh"))
			access = REG16_MIDHIGH;
		else if (strstr(reg_name, "h"))
			access = REG16_HIGH;
		else
			access = REG16_LOW;
	} else if (reg_name[0] == 'p') {
		main_type = 3;
		index = atoi(&reg_name[1]);
		if (strstr(reg_name, "b0"))
			access = REG8_B0;
		else if (strstr(reg_name, "b1"))
			access = REG8_B1;
		else if (strstr(reg_name, "b2"))
			access = REG8_B2;
		else if (strstr(reg_name, "b3"))
			access = REG8_B3;
		else if (strstr(reg_name, "b4"))
			access = REG8_B4;
		else if (strstr(reg_name, "b5"))
			access = REG8_B5;
		else if (strstr(reg_name, "b6"))
			access = REG8_B6;
		else if (strstr(reg_name, "b7"))
			access = REG8_B7;
		else
			access = REG8_B0;
	} else {
		return 0xFFFF;
	}

	return (main_type << 12) | (access << 6) | (index & 0x3F);
}

int
get_data_addr(const char *name)
{
	for (int i = 0; i < data_count; i++) {
		if (strcmp(datas[i].name, name) == 0) {
			return datas[i].addr;
		}
	}
	return -1;
}

int
get_bss_addr(const char *name)
{
	for (int i = 0; i < bss_count; i++) {
		if (strcmp(bss[i].name, name) == 0) {
			return bss[i].section.addr;
		}
	}
	return -1;
}

void
def_operants(char **tokenized, Instruction * instrc, uint8_t opcode)
{

	if (tokenized[2][0] != ',') {
		printf("Got an error on Line %d, Situation: ',' undefined on %s\n",
		       linecounter, tokenized[0]);
		error_report = 1;
	} else if (strcmp(tokenized[1], tokenized[3]) == 0) {
		printf("Warning: same register? Line: %d. Situation: %s -> %s on %s\n",
		     linecounter, tokenized[1], tokenized[3], tokenized[0]);
	}
	instrc->opcode = opcode;
	if (isalpha(tokenized[1][0]) || tokenized[1][0] == '\'') {
		if (isalpha(tokenized[1][1])) {
			if (tokenized[1][0] == '\'') {
				if (tokenized[1][2] != '\'') {
					printf("Got an error Line: %d, Situation: 'string error' on %s",
					       linecounter, tokenized[0]);
					return;
				}
				instrc->src = 0xFF;
				instrc->imm64 = tokenized[1][1];
			} else {
				int 		addr = 0;
				addr = get_data_addr(tokenized[1]);
				if (addr == -1) {
					addr = get_bss_addr(tokenized[1]);
					if (addr == -1) {
						printf("Got an error Line: %d, Situation: 'undefined label' on %s",
						 linecounter, tokenized[0]);
						return;
					}
				}
				instrc->src = 0xAD;
				instrc->imm64 = addr;
			}
		} else {
			instrc->src = en_registers(tokenized[1]);
		}
	} else {
		instrc->src = 0xFF;
		instrc->imm64 = (uint64_t) strtol(tokenized[1], NULL, 0);
	}

	if (isalpha(tokenized[3][0])) {
		instrc->dst = en_registers(tokenized[3]);
	} else {
		instrc->dst = 0xFF;
		instrc->imm64 = (uint64_t) strtol(tokenized[3], NULL, 0);
	}
}

int
get_label_addr(const char *name)
{
	for (int i = 0; i < label_count; i++) {
		if (strcmp(labels[i].name, name) == 0) {
			return labels[i].address;
		}
	}
	return -1;
}

void
lbl_operand(char **tokens, Instruction * instrc, uint8_t opcode)
{
	instrc->opcode = opcode;
	const char     *label = tokens[1];

	int 		label_addr = get_label_addr(label);
	if (label_addr == -1) {
		printf("Got an error Line: %d, Sitation: 'undefined label' %s\n",
		       linecounter, label);
		error_report = 1;
	} else {
		instrc->imm64 = label_addr;
		instrc->src = 0xFF;
	}
}

void
nlbl_operants(char **tokens, Instruction * instrc, uint8_t opcode)
{
	instrc->opcode = opcode;
	if (isalpha(tokens[1][0])) {
		instrc->dst = en_registers(tokens[1]);
		instrc->imm64 = 0;
	} else {
		instrc->dst = 0xFF;
		instrc->imm64 = (uint64_t) strtol(tokens[1], NULL, 0);
	}
	instrc->src = 0xFF;
}

void
nothing_operants(Instruction * instrc, uint8_t opcode)
{
	instrc->opcode = opcode;
	instrc->dst = 0xFF;
	instrc->src = 0xFF;
}

void
nlblnorg_operants(char **tokens, Instruction * instrc, uint8_t opcode)
{
	instrc->opcode = opcode;
	instrc->imm64 = (uint64_t) strtol(tokens[1], NULL, 0);
	instrc->dst = 0xFF;
	instrc->src = 0xFF;
}

void
revdef_operants(char **tokenized, Instruction * instrc, uint8_t opcode)
{
	if (tokenized[2][0] != ',') {
		printf("Got an error on Line %d, Situation: ',' undefined on %s\n",
		       linecounter, tokenized[0]);
		error_report = 1;
	} else if (strcmp(tokenized[1], tokenized[3]) == 0) {
		printf("Warning: same register? Line: %d. Situation: %s -> %s on %s\n",
		     linecounter, tokenized[1], tokenized[3], tokenized[0]);
	}
	instrc->opcode = opcode;
	if (isalpha(tokenized[3][0])) {
		if (isalpha(tokenized[3][1])) {
			int 		addr = 0;
			addr = get_data_addr(tokenized[3]);
			if (addr == -1) {
				addr = get_bss_addr(tokenized[3]);
				if (addr == -1) {
					printf("Got an error Line: %d, Situation: 'undefined label' on %s",
					       linecounter, tokenized[0]);
					return;
				}
			}
			instrc->dst = 0xAD;
			instrc->imm64 = addr;
		} else {
			printf("Got an error on Line %d, Situation: 'undefined label' on %s",
			       linecounter, tokenized[0]);
		}
	}
	instrc->src = en_registers(tokenized[1]);
}

int
opcode(char *tokenized[], Instruction * instrc)
{
	if (!tokenized[0]) {
		return -1;
	}
	if (strcmp(tokenized[0], "nop") == 0) {
		nothing_operants(instrc, OPCODE_NOP);
		return 1;
	} else if (strcmp(tokenized[0], "mov") == 0) {
		def_operants(tokenized, instrc, OPCODE_MOV);
		return 1;
	} else if (strcmp(tokenized[0], "add") == 0) {
		def_operants(tokenized, instrc, OPCODE_ADD);
		return 1;
	} else if (strcmp(tokenized[0], "sub") == 0) {
		def_operants(tokenized, instrc, OPCODE_SUB);
		return 1;
	} else if (strcmp(tokenized[0], "mul") == 0) {
		def_operants(tokenized, instrc, OPCODE_MUL);
		return 1;
	} else if (strcmp(tokenized[0], "div") == 0) {
		def_operants(tokenized, instrc, OPCODE_DIV);
		return 1;
	} else if (strcmp(tokenized[0], "and") == 0) {
		def_operants(tokenized, instrc, OPCODE_AND);
		return 1;
	} else if (strcmp(tokenized[0], "or") == 0) {
		def_operants(tokenized, instrc, OPCODE_OR);
		return 1;
	} else if (strcmp(tokenized[0], "xor") == 0) {
		def_operants(tokenized, instrc, OPCODE_XOR);
		return 1;
	} else if (strcmp(tokenized[0], "not") == 0) {
		def_operants(tokenized, instrc, OPCODE_NOT);
		return 1;
	} else if (strcmp(tokenized[0], "shl") == 0) {
		def_operants(tokenized, instrc, OPCODE_SHL);
		return 1;
	} else if (strcmp(tokenized[0], "shr") == 0) {
		def_operants(tokenized, instrc, OPCODE_SHR);
		return 1;
	} else if (strcmp(tokenized[0], "jmp") == 0) {
		lbl_operand(tokenized, instrc, OPCODE_JMP);
		return 1;
	} else if (strcmp(tokenized[0], "je") == 0) {
		lbl_operand(tokenized, instrc, OPCODE_JE);
		return 1;
	} else if (strcmp(tokenized[0], "jne") == 0) {
		lbl_operand(tokenized, instrc, OPCODE_JNE);
		return 1;
	} else if (strcmp(tokenized[0], "jl") == 0) {
		lbl_operand(tokenized, instrc, OPCODE_JL);
		return 1;
	} else if (strcmp(tokenized[0], "jg") == 0) {
		lbl_operand(tokenized, instrc, OPCODE_JG);
		return 1;
	} else if (strcmp(tokenized[0], "cmp") == 0) {
		def_operants(tokenized, instrc, OPCODE_CMP);
		return 1;
	} else if (strcmp(tokenized[0], "call") == 0) {
		lbl_operand(tokenized, instrc, OPCODE_CALL);
		return 1;
	} else if (strcmp(tokenized[0], "ret") == 0) {
		nothing_operants(instrc, OPCODE_RET);
		return 1;
	} else if (strcmp(tokenized[0], "push") == 0) {
		nlbl_operants(tokenized, instrc, OPCODE_PUSH);
		return 1;
	} else if (strcmp(tokenized[0], "pop") == 0) {
		nlbl_operants(tokenized, instrc, OPCODE_POP);
		return 1;
	} else if (strcmp(tokenized[0], "hlt") == 0) {
		nothing_operants(instrc, OPCODE_HLT);
		return 1;
	} else if (strcmp(tokenized[0], "inc") == 0) {
		nlbl_operants(tokenized, instrc, OPCODE_INC);
		return 1;
	} else if (strcmp(tokenized[0], "dec") == 0) {
		nlbl_operants(tokenized, instrc, OPCODE_DEC);
		return 1;
	} else if (strcmp(tokenized[0], "print") == 0) {
		nlbl_operants(tokenized, instrc, OPCODE_PRINT);
		return 1;
	} else if (strcmp(tokenized[0], "entry") == 0) {
		lbl_operand(tokenized, instrc, OPCODE_ENTRY);
		return 1;
	} else if (strcmp(tokenized[0], "syscall") == 0) {
		nlbl_operants(tokenized, instrc, OPCODE_SYSCALL);
		return 1;
	} else if (strcmp(tokenized[0], "load") == 0) {
		def_operants(tokenized, instrc, OPCODE_LOAD);
		return 1;
	} else if (strcmp(tokenized[0], "store") == 0) {
		revdef_operants(tokenized, instrc, OPCODE_STORE);
		return 1;
	}
	return 0;
}

char           *imported_files[1000];
int 		imported_count = 0;

int 		byte_offset = 0;

void
refresh_header(const char *out_file)
{
	FILE           *outfile = fopen(out_file, "r+b");
	if (outfile == NULL) {
		perror("refresh header open failed");
		return;
	}
	fseek(outfile, 0, SEEK_SET);
	fwrite(&header, 1, sizeof(BinaryHeader), outfile);
	fclose(outfile);
}

//pass label functions and sections
int
pass(int count, char **tokens)
{
	if (count == 2 && strcmp(tokens[1], ":") == 0) {
		for (int i = 0; i < count; i++) {
			free(tokens[i]);
		}
		return 0;
	}
	if (count >= 1 && strcmp(tokens[0], "section") == 0) {
		for (int i = 0; i < count; i++) {
			free(tokens[i]);
		}
		return 0;
	}
	return -1;
}

void
collect_bss(char line[1024])
{
	char           *tokens[10];
	int 		count = tokenizer(line, tokens, 10);

	if (count > 0) {
		if (strcmp(tokens[0], "section") == 0 && strcmp(tokens[1], "bss") == 0) {
			if (!header.section_bss) {
				header.section_bss = byte_offset + sizeof(BinaryHeader);
				sections = SECTION_BSS;
			}
		}
	}
	if (sections == SECTION_BSS) {
		if (count >= 3 && strcmp(tokens[1], "reb") == 0) {
			int 		size = atoi(tokens[2]);
			bsssp(tokens[0], byte_offset + sizeof(BinaryHeader), DATA_TYPE_RB, size);
			byte_offset += sizeof(BSSSectionType);
		}
	}
	for (int i = 0; i < count; i++) {
		free(tokens[i]);
	}
}

void
write_bss(const char *asm_file, const char *out_file)
{
	for (int i = 0; i < imported_count; i++) {
		FILE           *imported_file = fopen(imported_files[i], "r");
		if (imported_file == NULL) {
			perror("Imported file error");
			return;
		}
		char 		line     [1024];
		while (fgets(line, sizeof(line), imported_file)) {
			collect_bss(line);
		}
		fclose(imported_file);
	}
	FILE           *mainfile = fopen(asm_file, "r");
	if (mainfile == NULL) {
		perror("mainfile file error");
		return;
	}
	char 		line     [1024];
	while (fgets(line, sizeof(line), mainfile)) {
		collect_bss(line);
	}
	fclose(mainfile);
	header.bss_count = bss_count;
	FILE           *outfile = fopen(out_file, "wb");
	if (outfile == NULL) {
		perror("write out file failed");
		return;
	}
	fwrite(&header, 1, sizeof(BinaryHeader), outfile);
	for (int i = 0; i < bss_count; i++) {
		fwrite(&bss[i].section, sizeof(BSSSectionType), 1, outfile);
	}
	fclose(outfile);
}

int
is_any_type(char *type)
{
	if (strcmp(type, "byte") == 0 ||
	    strcmp(type, "hword") == 0 ||
	    strcmp(type, "word") == 0 ||
	    strcmp(type, "dword") == 0) {
		return 1;
	}
	return 0;
}

void
collect_data(char line[1024])
{
	char           *tokens[10];
	int 		count = tokenizer(line, tokens, 10);
	if (count > 0) {
		if (strcmp(tokens[0], "section") == 0 && strcmp(tokens[1], "data") == 0) {
			if (!header.section_data) {
				header.section_data = byte_offset + sizeof(BinaryHeader);
				sections = SECTION_DATA;
			}
		}
		if (count >= 3 && strcmp(tokens[1], "ascii") == 0) {
			datasp(tokens[0], byte_offset + sizeof(BinaryHeader), DATA_TYPE_ASCII);
			for (int i = 2; i < count; i++) {
				if (isdigit(tokens[i][0])) {
					byte_offset += 1;
					header.data_size += 1;
					continue;
				}
				for (int j = 0; j < strlen(tokens[i]); j++) {
					if (tokens[i][j] == '\'') {
						byte_offset += 1;
						header.data_size += 1;
						j += 2;
						continue;
					}
					byte_offset += 1;
					header.data_size += 1;
				}
			}
		} else if (count >= 2 && strcmp(tokens[1], "byte") == 0 || strcmp(tokens[0], "byte") == 0) {
			if (strcmp(tokens[0], "byte") != 0) {
				datasp(tokens[0], byte_offset + sizeof(BinaryHeader), DATA_TYPE_BYTE);
			}
			byte_offset += 1;
			header.data_size += 1;
		} else if (count >= 2 && strcmp(tokens[1], "hword") == 0 || strcmp(tokens[0], "hword") == 0) {
			if (strcmp(tokens[0], "hword") != 0) {
				datasp(tokens[0], byte_offset + sizeof(BinaryHeader), DATA_TYPE_HWORD);
			}
			byte_offset += 2;
			header.data_size += 2;
		} else if (count >= 2 && strcmp(tokens[1], "word") == 0 || strcmp(tokens[0], "word") == 0) {
			if (strcmp(tokens[0], "word") != 0) {
				datasp(tokens[0], byte_offset + sizeof(BinaryHeader), DATA_TYPE_WORD);
			}
			byte_offset += 4;
			header.data_size += 4;
		} else if (count >= 2 && strcmp(tokens[1], "dword") == 0 || strcmp(tokens[0], "dword") == 0) {
			if (strcmp(tokens[0], "dword") != 0) {
				datasp(tokens[0], byte_offset + sizeof(BinaryHeader), DATA_TYPE_DWORD);
			}
			byte_offset += 8;
			header.data_size += 8;

		}
	}
	for (int i = 0; i < count; i++) {
		free(tokens[i]);
	}
}



int
writer_data(char line[1024], FILE * outfile)
{
	char           *tokens[10];
	int 		count = tokenizer(line, tokens, 10);
	if (sections == SECTION_DATA) {
		if (pass(count, tokens) == 0) {
			return -1;
		}
		int 		counteri = 0;
		while (counteri < count) {
			if (count >= 2 && strcmp(tokens[counteri], "ascii") == 0) {
				int 		digit = 0;
				for (int i = 2; i < count; i++) {
					if (isdigit(tokens[i][0])) {
						digit = atoi(tokens[i]);
						fputc(digit, outfile);
						return -1;
					}
					for (int j = 0; j < strlen(tokens[i]); j++) {
						if (tokens[i][j] == '\'') {
							fputc(tokens[i][++j], outfile);
							j++;
							return -1;
						}
						fputc(tokens[i][j], outfile);
					}
				}
			} else if (count >= 2 && strcmp(tokens[counteri], "byte") == 0) {
				if (isdigit(tokens[++counteri][0])) {
					int 		digit = atoi(tokens[counteri]);
					fputc(digit, outfile);
				} else {
					fputc(tokens[counteri][1], outfile);
				}
			} else if (count >= 2 && strcmp(tokens[counteri], "hword") == 0) {
				uint16_t 	val = (uint16_t) strtol(tokens[++counteri], NULL, 0);
				fputc(val & 0xFF, outfile);
				fputc((val >> 8) & 0xFF, outfile);
			} else if (count >= 2 && strcmp(tokens[counteri], "word") == 0) {
				uint32_t 	val = (uint32_t) strtol(tokens[++counteri], NULL, 0);
				fputc(val & 0xFF, outfile);
				fputc((val >> 8) & 0xFF, outfile);
				fputc((val >> 16) & 0xFF, outfile);
				fputc((val >> 24) & 0xFF, outfile);
			} else if (count >= 2 && strcmp(tokens[counteri], "dword") == 0) {
				uint64_t 	val = (uint64_t) strtol(tokens[++counteri], NULL, 0);
				fputc(val & 0xFF, outfile);
				fputc((val >> 8) & 0xFF, outfile);
				fputc((val >> 16) & 0xFF, outfile);
				fputc((val >> 24) & 0xFF, outfile);
				fputc((val >> 32) & 0xFF, outfile);
				fputc((val >> 40) & 0xFF, outfile);
				fputc((val >> 48) & 0xFF, outfile);
				fputc((val >> 56) & 0xFF, outfile);
			}
			counteri++;
		}
	}
	for (int i = 0; i < count; i++) {
		free(tokens[i]);
	}

	return 0;
}

void
write_data(const char *asm_file, const char *out_file)
{
	FILE           *outfile = fopen(out_file, "ab");
	if (outfile == NULL) {
		perror("append out file failed");
		return;
	}
	for (int i = 0; i < imported_count; i++) {
		FILE           *imported_file = fopen(imported_files[i], "r");
		if (imported_file == NULL) {
			perror("Imported file error");
			return;
		}
		char 		line     [1024];
		while (fgets(line, sizeof(line), imported_file)) {
			collect_data(line);
		}
		fclose(imported_file);
	}
	FILE           *mainfile = fopen(asm_file, "r");
	if (mainfile == NULL) {
		perror("mainfile file error");
		return;
	}
	char 		line     [1024];

	while (fgets(line, sizeof(line), mainfile)) {
		collect_data(line);
	}
	for (int i = 0; i < imported_count; i++) {
		FILE           *imported_file = fopen(imported_files[i], "r");
		if (imported_file == NULL) {
			perror("Imported file error");
			return;
		}
		char 		line     [1024];
		while (fgets(line, sizeof(line), imported_file)) {
			if (writer_data(line, outfile) == -1) {
				continue;
			}
		}
		fclose(imported_file);
	}
	mainfile = fopen(asm_file, "r");
	while (fgets(line, sizeof(line), mainfile)) {
		if (writer_data(line, outfile) == -1) {
			continue;
		}
	}
	refresh_header(out_file);
	fclose(mainfile);
	fclose(outfile);
}

int 		current_instruction_pos = 0;
void
collect_code(char line[1024])
{
	char           *tokens[10];
	int 		count = tokenizer(line, tokens, 10);
	if (count > 0) {
		if (strcmp(tokens[0], "section") == 0 && strcmp(tokens[1], "code") == 0) {
			if (!header.section_code) {
				header.section_code = byte_offset + sizeof(BinaryHeader);
				sections = SECTION_CODE;
			}
		}
		if (sections == SECTION_CODE) {
			if (count >= 2 && strcmp(tokens[1], ":") == 0) {
				labelsp(tokens[0], current_instruction_pos, LABEL_TYPE_CODE);
			}
			if (count >= 1 && tokens[0] && strcmp(tokens[0], "section") != 0 &&
			    strcmp(tokens[0], "import") != 0 &&
			    (count < 2 ||
			     (tokens[1] && strcmp(tokens[1], "ascii") != 0 &&
			      strcmp(tokens[1], "hword") != 0 && strcmp(tokens[1], "word") != 0 &&
			      strcmp(tokens[1], "dword") != 0 &&
			      strcmp(tokens[1], "reb") != 0))) {

				if (count == 1 ||
				    (count >= 2 && tokens[1] && strcmp(tokens[1], ":") != 0)) {
					current_instruction_pos++;
				}
			}
		}
	}
	for (int i = 0; i < count; i++) {
		free(tokens[i]);
	}
}

int
writer_code(char line[1024], FILE * outfile)
{
	char           *tokens[10];
	int 		count = tokenizer(line, tokens, 10);
	if (count > 0) {
		if (pass(count, tokens) == 0) {
			return -1;
		}
		Instruction 	instrc = {0};
		if (opcode(tokens, &instrc)) {
			fwrite(&instrc, sizeof(instrc), 1, outfile);
		}
	} else {
		return -1;
	}
	for (int i = 0; i < count; i++) {
		free(tokens[i]);
	}
	return 0;
}

void
write_code(const char *asm_file, const char *out_file)
{
	FILE           *outfile = fopen(out_file, "ab");
	if (outfile == NULL) {
		perror("append out file failed");
		return;
	}
	for (int i = 0; i < imported_count; i++) {
		FILE           *imported_file = fopen(imported_files[i], "r");
		if (imported_file == NULL) {
			perror("Imported file error");
			return;
		}
		char 		line     [1024];

		while (fgets(line, sizeof(line), imported_file)) {
			collect_code(line);
		}
		fclose(imported_file);
	}
	FILE           *mainfile = fopen(asm_file, "r");
	if (mainfile == NULL) {
		perror("mainfile file error");
		return;
	}
	char 		line     [1024];

	while (fgets(line, sizeof(line), mainfile)) {
		collect_code(line);
	}
	for (int i = 0; i < imported_count; i++) {
		FILE           *imported_file = fopen(imported_files[i], "r");
		if (imported_file == NULL) {
			perror("Imported file error");
			return;
		}
		char 		line     [1024];
		while (fgets(line, sizeof(line), imported_file)) {
			writer_code(line, outfile);
		}
		fclose(imported_file);
	}
	int 		addr = ftell(outfile);
	if (imported_count > 0) {
		header.entry_start_point = addr;
	} else {
		header.entry_start_point = 0;
	}
	mainfile = fopen(asm_file, "r");
	if (mainfile == NULL) {
		perror("mainfile file error");
		return;
	}
	while (fgets(line, sizeof(line), mainfile)) {
		writer_code(line, outfile);
	}
	refresh_header(out_file);
	fclose(mainfile);
	fclose(outfile);
}

void
parser(const char *asm_file, const char *out_file)
{
	FILE           *asmfi = fopen(asm_file, "rb");
	if (asmfi == NULL) {
		perror("asmfi open failed");
		return;
	}
	char 		line     [256];
	while (fgets(line, sizeof(line), asmfi)) {
		char           *tokens[10];
		int 		count = tokenizer(line, tokens, 10);
		if (count > 0) {
			if (strcmp(tokens[0], "import") == 0) {
				imported_files[imported_count++] = strdup(tokens[1]);
			}
		}
		for (int i = 0; i < count; i++) {
			free(tokens[i]);
		}
	}
	fclose(asmfi);
	write_bss(asm_file, out_file);
	write_data(asm_file, out_file);
	write_code(asm_file, out_file);
	for (int i = 0; i < imported_count; i++) {
		free(imported_files[i]);
	}
}
