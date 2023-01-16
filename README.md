# minimal-kernel-implementation
Kernel for a student from Peter the Great St. Petersburg Polytechnic University


#### Building:
```md
g++ -ffreestanding -m32 -o kernel.o -c kernel.cpp
```

```md
ld --oformat binary -Ttext 0x10000 -o kernel.bin - -entry=kmain -m elf_i386 kernel.o
```

#### Launch:
```md
qemu –fda bootsect.bin –fdb kernel.bin
```
###### (if compiling in a 64-bit Linux environment, the gcc-multilib, binutils, g++,g++-multilib packages must be installed on the system)



##### The C code can be built using the Microsoft C Compiler using the following instructions (if no environment variables are specified, then the build must be done from the Visual Studio Command Prompt):

```md
cl.exe /GS- /c kernel.cpp
```

```md
link.exe /OUT:kernel.bin /BASE:0x10000 /FIXED /FILEALIGN:512 /MERGE:.rdata=.data /IGNORE:4254 /NODEFAULTLIB /ENTRY:kmain /SUBSYSTEM:NATIVE kernel.obj
```

```md
dumpbin /headers kernel.bin
```

```md
qemu –fda bootsect.bin –fdb kernel.bin
```

##### For successful compilation, assembler inserts in the program code in C must be written using Intel syntax:
```md
// Beginning of kernel.cpp
extern "C" int kmain();
__declspec(naked) void startup()
{
      __asm {
            call kmain;
} }
      // Endless kernel loop
      while(1)
      {
__asm hlt;
}
```
