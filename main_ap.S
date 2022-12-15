// https://wiki.osdev.org/Symmetric_Multiprocessing
// this code will be relocated to 0x8000, sets up environment for calling a C function

    .code16
    .global ap_trampoline
ap_trampoline:
    cli
	hlt
    cld
    ljmp    $0, $0x8040
    .align 16
_gdt_table:
    .long 0, 0
    .long 0x0000FFFF, 0x00CF9A00    // flat code
    .long 0x0000FFFF, 0x008F9200    // flat data
    .long 0x00000068, 0x00CF8900    // tss
_gdt_entry:
    .word _gdt_entry - _gdt_table - 1
    .long 0x8010
    .long 0, 0
    .align 64
_start: //0x8040
    xorw    %ax, %ax
    movw    %ax, %ds
    lgdtl   0x8030
    movl    %cr0, %eax
    orl     $1, %eax
    movl    %eax, %cr0
    ljmp    $8, $0x8060 //_start_32
    .align 32
    .code32
_start32: //0x8060
    movw    $16, %ax
    movw    %ax, %ds
    movw    %ax, %ss
    // get lapic id
    mov     $1, %eax
    cpuid
    shrl    $24, %ebx
    movl    %ebx, %edi
    // set up 4K stack, 0x1F0000-(4K*lapic_id)
    shll    $12, %ebx
    movl    $0x1F0000, %esp
    subl    %ebx, %esp
    pushl   %edi // drop apic id onto the stack

    // yip yip
loop:
    jmp loop

    // yip yip
    lock    incb aprunning
    // jump into C code (should never return)
    ljmp    $8, $secondary_ap_main

.global ap_trampoline_end
ap_trampoline_end: