.extern start_kernel

    .section .text.init
    .globl _start

_start:
    # ------------------
    # - your code here -
    # ------------------

    # set sp
    la sp, boot_stack_top
    add s0, sp, x0

    # set stvec = _traps
    la t0, _traps
    csrw stvec, t0

    # set sie[STIE] = 1
    addi t0, x0, 0b100000
    csrs sie, t0

    # init mm
    jal ra, mm_init

    # init task_struct
    jal ra, task_init

    # set first time interrupt
    jal ra, clock_set_next_event

    # set sstatus[SIE] = 1
    csrs sstatus, 0b10


    # ------------------

    # ------------------
    # - your lab1 code -
    # ------------------

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

