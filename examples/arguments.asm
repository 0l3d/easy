# EMULATOR BASED ARGUMENT PARSING
# WORKS ON EVERYPLATFORM (NOT ONLY LINUX)
section data 
  ascik ascii "Hello, World!" 0 # ignore this.
  # safe way to get arguments:
  arguments ascii 
section code
entry main 
get_args_from_stack:
  pop r1 # get argv raw string from stack.
  # or safe way:
  # mov arguments, r1 
  ret
main:
  mov 1, r0   # stdout
  mov 100, r2 # len = 100
runner: 
  call get_args_from_stack 
  syscall 1
  # example input: ./easy -i <filename> -a "--help"
  # example output: --help
  # 'cause im only writing, not parsing in this example.
