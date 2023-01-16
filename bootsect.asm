.code16
.att_syntax

_start:
movw  %cs, %ax
movw  %ax, %ds
movw  %ax, %ss 
movw  $_start, %sp 

_get_letter:
movb $0x00, %ah 
int $0x16
#cmp %al, $0x62	#b
cmp $0x62, %al
je _get_m
#cmp %al, $0x73	#s
cmp  $0x73, %al
je _get_t
jmp _get_letter

_get_m:
#передать значение ядру
movb   $0x00, %ah 
int $0x16 
#cmp %al, $0x6D	#m
cmp $0x6D, %al
je _load_kernel
jmp _get_letter

_get_t:
movb  $0x00, %ah 
int $0x16 
#cmp %al, $0x74	#t
cmp $0x74, %al
je _get_d
jmp _get_letter

_get_d:
#передать значение ядру
movb $0x00, %ah 
int $0x16 
#cmp %al, $0x64	#d
cmp $0x64, %al
je _load_kernel
jmp _get_letter

_load_kernel:
movb $0x01, %dl #номер диска
movb $0x00, %dh #номер головки
movb $0x00, %ch #номер цилиндра
movb $0x01, %cl #номер сектора
movb $0x01, %al #количество секторов
movw $0x1000, %bx #адрес буфера 
movw %bx, %es
xorw %bx, %bx

movb $0x02, %ah 
int $0x13 

cli

lgdt gdt_info

inb $0x92, %al
orb $2, %al
outb %al, $0x92

movl %cr0, %eax 
orb $1, %al 
movl %eax, %cr0 
ljmp $0x8, $protected_mode
 
gdt: 
.byte 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
.byte 0xff, 0xff, 0x00, 0x00, 0x00, 0x9A, 0xCF, 0x00
.byte 0xff, 0xff, 0x00, 0x00, 0x00, 0x92, 0xCF, 0x00
gdt_info:
.word gdt_info - gdt
.word gdt, 0

.code32
protected_mode:
movw $0x10, %ax
movw %ax, %es 
movw %ax, %ds 
movw %ax, %ss
call 0x10000

.zero (512-(.-_start)-2) 
.byte 0x55, 0xAA 
