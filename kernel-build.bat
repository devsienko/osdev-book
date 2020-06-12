fasm src\kernel\startup.asm bin\startup.o
gcc src\kernel\kernel.c -o bin\main.o -c

ld -T src\kernel\kernel.ld -o bin\disk\kernel.bin bin\startup.o bin\main.o
objcopy bin\disk\kernel.bin -O binary