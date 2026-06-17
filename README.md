# EASY64

Simple cpu architecture in C.

Wiki: [Easy64 Wiki](https://github.com/0l3d/easy/wiki)

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
git clone https://git.sr.ht/~oled/easy
cd easy64/
make

./easy -h
```

### Compile and Interpret

```bash
./easy -c hello.asm -a "nokernel"
# if you dont put "nokernel", then assembler creates a Program assembly, contains like: header, data section, bss section bla bla.
# We are working on No Bootloader, No Kernel mode, so we need to initalize our memory from zero when using "nokernel".
./easy -i a.out # reads it default in "nokernel" mode.
```

# How to work on Easy64?

This Program is emulating real hardware, so you cant work on it without a bootloader, kernel, micro-controller or any software.  
For more information and a Kernel example works on easy64:  
Check the [Working on Easy64](https://github.com/0l3d/easy/wiki/Working-on--Easy64) Wiki page.

# License

This project is licensed under the **GPL-3.0 License**.

# Author

Created By **oled**
