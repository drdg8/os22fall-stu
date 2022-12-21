.extern start_kernel
.extern task_init
.extern mm_init
.extern clock_set_next_event
.extern early_pgtbl

    .section .text.init
    .globl _start

_start:
    # ------------------
    # - your code here -
    # ------------------

    # set sp
    la sp, boot_stack_top
    li t0, 0xffffffdf80000000
    sub sp, sp, t0
    add s0, sp, x0

    # init virtual memory
    call setup_vm
    call relocate

    # init mm
    jal ra, mm_init

    # set vm final 
    jal ra, setup_vm_final

    # init task_struct
    jal ra, task_init

    # set first time interrupt
    jal ra, clock_set_next_event

    # set stvec = _traps
    la t0, _traps
    csrw stvec, t0

    # set sie[STIE] = 1
    addi t0, x0, 0b100000
    csrs sie, t0

    # set sstatus[SIE] = 1
    # csrs sstatus, 0b10

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

relocate:
    # set ra = ra + PA2VA_OFFSET
    # set sp = sp + PA2VA_OFFSET (If you have set the sp before)
    li t0, 0xffffffdf80000000
    add ra, ra, t0
    add sp, sp, t0

    # set satp with early_pgtbl
    # set MODE
    addi t2, x0, 8
    slli t2, t2, 60
    # set PPN
    la t1, early_pgtbl
    sub t1, t1, t0
    srli t1, t1, 12

    add t1, t1, t2
    csrw satp, t1

    # flush tlb
    sfence.vma zero, zero

    # flush icache
    fence.i

    ret

    .section .bss.stack
    .globl boot_stack

boot_stack:
    .space 0x1000 # <-- change to your stack size

    .globl boot_stack_top

boot_stack_top:

