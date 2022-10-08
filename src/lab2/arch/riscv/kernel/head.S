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

    // addi sp, sp, -64
    // sd a0, 0(sp)
    // sd a1, 8(sp)
    // sd a2, 16(sp)
    // sd a3, 24(sp)
    // sd a4, 32(sp)
    // sd a5, 40(sp)
    // sd a6, 48(sp)
    // sd a7, 56(sp)

    // rdtime t0
    // // li a0, TIMECLOCK
    // li a0, 10000000
    // add a0, a0, t0
    // li a1, 0
    // li a2, 0
    // li a3, 0
    // li a4, 0
    // li a5, 0

    // li a6, 0
    // li a7, 0

    // ecall

    // addi sp, sp, -64
    // ld a0, 0(sp)
    // ld a1, 8(sp)
    // ld a2, 16(sp)
    // ld a3, 24(sp)
    // ld a4, 32(sp)
    // ld a5, 40(sp)
    // ld a6, 48(sp)
    // ld a7, 56(sp)

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

