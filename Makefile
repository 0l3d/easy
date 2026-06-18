CC      = clang
TARGET  = easy

SOURCES = ./src/interpreter.c \
          ./src/assembler.c \
          ./src/cpu.c

OBJECTS = ${SOURCES:.c=.o}

LDFLAGS = -lSDL2
CFLAGS  = -g

NOR_FLAGS = ${BASE_CFLAGS}

SEC_FLAGS = \
        -g -O2 -march=native \
        -Wall -Wextra -Wpedantic \
        -fstack-protector-strong -D_FORTIFY_SOURCE=2 \
        -Wformat=2 -Wformat-security -Wstringop-overflow -Wnull-dereference \
        -fanalyzer -Werror -Werror=unused-result \
        -fsanitize=address,undefined -fno-omit-frame-pointer

# Suffix rule 
.SUFFIXES: .c .o

.c.o:
	${CC} ${CFLAGS} -c $< -o $@

${TARGET}: ${OBJECTS}
	${CC} -o ${TARGET} ${OBJECTS} ${LDFLAGS}

nor: CFLAGS = ${NOR_FLAGS}
nor: ${TARGET}

sec: CFLAGS = ${SEC_FLAGS}
sec: ${TARGET}

minimize:
	strip ${TARGET}

clean:
	rm -f ${OBJECTS} ${TARGET} a.out

.PHONY: nor sec clean minimize

