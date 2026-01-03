section data
	mydata ascii "Hello,"
				 byte 10 
				 ascii "World!"
				 byte 10
				 byte 65 
				 ascii 65 65 65 10
				 byte 0

section code
  mov 1, r0
  mov mydata, r1
  mov 20, r2
  syscall 1

