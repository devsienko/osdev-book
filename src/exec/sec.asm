use32

mov al, 75; capital K code
@@:
mov byte[0xB8000 + (25 * 80 - 1) * 2], al
inc al



; cli
mov ax, 0x23
; mov ds, ax
; mov es, ax
; mov fs, ax
; mov gs, ax
    ;   mov eax, esp; \
      push 0x23
      push esp
      pushf
      push 0x1B
      push word[next]
; @@:
;       jmp @b
      iret; \
next: 
; 	hlt 