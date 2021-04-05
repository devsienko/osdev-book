use32

;int 0x30 ; 0x20 - irq_base, 16 - handler index, 0x20 + 16 = 0x30  

mov al, 75; capital K code
@@:
mov byte[0xB8000 + (24 * 80) * 2], al
inc al
jmp @b