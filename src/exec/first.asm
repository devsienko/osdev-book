use32

mov al, 75; capital K code
@@:
mov byte[0xB8000 + (24 * 80) * 2], al
inc al
jmp @b