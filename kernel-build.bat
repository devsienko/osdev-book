fasm src\startup.asm bin\startup.o
gcc -Wall -pedantic-errors src\kernel.c -o bin\main.o -nostdlib

ld -T src\kernel.ld -o bin\disk\kernel.bin bin\startup.o bin\main.o
objcopy bin\disk\kernel.bin -O binary