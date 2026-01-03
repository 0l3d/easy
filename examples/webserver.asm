# Web server example for EASY64 ASSEMBLY Linux Only
# Try: 
# ./easy -c examples/webserver.asm
# ./easy -i a.out
# then:
# curl http://127.0.0.1:8080


section data
    resp ascii "HTTP/1.1 200 OK" 13 10
         ascii "Content-Type: text/plain; charset=utf-8" 13 10
         ascii "Content-Length: 13" 13 10
         ascii "Connection: close" 13 10 13 10
         ascii "Hello, World!"
    # len 112

    sockaddr hword 2
             hword 8080 !
	    # hword 36895      # BIG ENDIAN '8080'
            # the ! operator is swaps bytes like 901F to 1F90
            # or you can use bytes like this: 
            # byte 31 
            # byte 144
             word 0
             dword 0

section code
    # socket(AF_INET=2, SOCK_STREAM=1, IPPROTO_TCP=6)
    mov 2, r0
    mov 1, r1
    mov 6, r2
    syscall 41
    print r6
    mov r6, r8               # server_fd

    # bind(server_fd, &sockaddr, 16)
    mov r8, r0
    mov sockaddr, r1
    mov 16, r2
    syscall 49
    print r6

    # listen(server_fd, 16)
    mov r8, r0
    mov 16, r1
    syscall 50
    print r6

looper:
    # accept(server_fd, NULL, NULL)
    mov r8, r0
    mov 0, r1
    mov 0, r3
    syscall 43
    print r6
    mov r6, r9               # client_fd

    # write(client_fd, resp, 110)
    mov r9, r0
    mov resp, r1
    mov 112, r2              # len from comment above
    syscall 1
    print r6                 # bytes written

    # close(client_fd)
    mov r9, r0
    syscall 3
    print r6

    jmp looper

exit:
    mov 0, r0
    syscall 60
