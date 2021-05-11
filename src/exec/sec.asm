use32
org    0x20000

; extrn _switch_to_user_mode

mov al, 75; capital K code
@@:
mov byte[0xB8000 + (25 * 80 - 1) * 2], al
inc al

; [7FFFD01E] = esp + 0x1000;
; TSS *tss = 7FFFCFFE;
; mov eax, esp
; add eax, 0x1000
; mov [0x7FFFD01E], eax
cli
mov ax, 0x23
mov ds, ax
mov es, ax
mov fs, ax
mov gs, ax
mov eax, esp;
push 0x23
push esp
pushf
push 0x1B
push @f
iret; \
@@:
mov byte[0xB8000 + (25 * 80 - 1) * 2], al
inc al
jmp @b