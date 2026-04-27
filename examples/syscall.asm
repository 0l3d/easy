# SYSCALL EXAMPLE 
# Syscall Arguments: 
# r0 -> first 
# r1 -> second 
# r2 -> third 
# r3 -> fourth 
# r4 -> fifth
# r5 -> sixth
# r6 -> return value


section data 
  msg ascii "Hello, " 10
			ascii "World!" 10 
			
section code
entry write
write:
  mov 1, r0 
  mov msg, r1 
  mov 15, r2
  syscall 1  
