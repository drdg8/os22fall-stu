#include "defs.h"
#include "../include/syscall.h"

void trap_handler(unsigned long scause, unsigned long sepc, struct pt_regs *regs) {
    // 通过 `scause` 判断trap类型
    // 如果是interrupt 判断是否是timer interrupt
    // 如果是timer interrupt 则打印输出相关信息, 并通过 `clock_set_next_event()` 设置下一次时钟中断
    // `clock_set_next_event()` 见 4.5 节
    // 其他interrupt / exception 可以直接忽略

    // printk("scause: %lx", scause);
    // while(1);

    // interrupt
    if (scause & (1 << 31)){
        // if (scause % 16 == 4){
        //     printk("[U] User Mode Timer Interrupt!\n")
        // }
        if (scause % 16 == 5){
            // printk("[S] Supervisor Mode Timer Interrupt!\n");
            clock_set_next_event();
            do_timer();
        }
    } else{
        if (scause % 16 == 8) {
            // printk("ECALL_FROM_U_MODE Interrupt!\n");
            syscall(regs);
        }
    }
}

