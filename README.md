# Minimal-Kernel-implementation
Kernel for a student from Peter the Great St. Petersburg Polytechnic University

g++ -ffreestanding -m32 -o kernel.o -c kernel.cpp
ld --oformat binary -Ttext 0x10000 -o kernel.bin - -entry=kmain -m elf_i386 kernel.o
