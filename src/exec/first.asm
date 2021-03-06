use32

org 0x20000

mov al, 75; capital K code
; mov byte[0xB8000 + (25 * 80 - 2) * 2], 75
@@:
mov byte[0xB8000 + (25 * 80 - 1) * 2], al
inc al
jmp @b