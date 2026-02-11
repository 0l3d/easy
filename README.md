# EASY64

Simple cpu architecture in C.

## Features

- Simple and fast CPU architecture
- Easy-to-understand assembly language
- Label handling and jump instructions
- Basic stack operations (push, pop, call, ret)
- Arithmetic, logic, and bitwise operations
- Debug instructions (print)
- Import support for modules.
- Subregister support (8, 16, 32-bit)
- And more!

## Usage

```bash
git clone https://github.com/0l3d/easy64.git
cd easy64/
make

./easy -h
```

## Examples

**Hello World**

```
section data # data section
  hello_world ascii "Hello, World!" # helloworld
section code # code section
entry start  # entry selection
start:       # start label
  mov 1, r0
  mov hello_world, r1  # write syscall -> write(r0 = 1, r1 = adress of hello_world, r2 = 14 (size));
  mov 14, r2
  syscall 1 # syscall and id (0 = read, 1 = write, references -> https://filippo.io/linux-syscall-table/)
```

### Compile and Interpret

```bash
./easy -c hello.asm # compiles to a.out
./easy -i a.out
```

# Portability 
Check the **"src/portability.h"**


# License

This project is licensed under the **GPL-3.0 License**.

# Author

Created By **ol3d**
