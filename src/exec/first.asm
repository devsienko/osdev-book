use32

org 0x12000

mov eax, 75; capital K code
; mov byte[0xB8000 + (25 * 80 - 2) * 2], 75
mov byte[0xB8000 + (25 * 80 - 1) * 2], 75
@@:
mov cx, 20
jmp @b