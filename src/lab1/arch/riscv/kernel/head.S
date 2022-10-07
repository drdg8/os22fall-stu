.extern start_kernel

    .section .text.entry
    .globl _start

_start:
    # ------------------
    # - your code here -
    # ------------------

    # lui sp, %hi(boot_stack_top)
    # addi sp, sp, %lo(boot_stack_top)

    # auipc sp, %hi(boot_stack_top)
    # addi sp, sp, %lo(boot_stack_top)

    # auipc sp, boot_stack_top
    # addi sp, sp, boot_stack_top
    la sp, boot_stack_top
    add s0, sp, x0

    addi sp, sp, -16          # store ra
    sd ra, 0(sp)
    addi s0, sp, 16           # change fp / s0
    jal ra, start_kernel
    ld ra, 0(sp)
    addi sp, sp, 16
    ret

    .section .bss.stack
    .globl boot_stack

boot_stack:
    .space 0x1000 # <-- change to your stack size

    .globl boot_stack_top

boot_stack_top:

