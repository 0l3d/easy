section data
  str ascii "Hello, World!" 0 # ascii -> Hello, World!\0

section code 
  mov str, r0   # r0 = *ascii
  mov 0, r1     # r1 = 0

loop:
  load r0, r3  # r3 = *r0 
  inc r0       # r0++ 
  inc r1       # r1++ 

  cmp r3, 0    # if r3 == '\0'
  je .lenout   #jump lenout 

  jmp loop     # else continue

.lenout:
  print r1    # DEBUG 
  mov r1, r10 # HELLO WORLD LEN r10 = r1 
  mov 0, r1   # r1 = 0 

  # write(1, str, r10)
  mov 1, r0   
  mov str, r1 
  mov r10, r2 
  syscall 1

  mov 0, r0 
  syscall 60

