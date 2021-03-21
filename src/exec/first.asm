use32

mov al, 75; capital K code
@@:
mov byte[0xB8000 + (25 * 80 - 1) * 2], al
inc al
jmp @b