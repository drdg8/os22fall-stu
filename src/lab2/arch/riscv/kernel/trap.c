// #include "clock.c"
// #include "../../../include/printk.h"

unsigned long pow(unsigned long x, int n){
    unsigned long ret = 1;
    for (int i = 0; i < n; i++){
        ret *= x;
    }
    return ret;
}

void trap_handler(unsigned long scause, unsigned long sepc) {
    // 通过 `scause` 判断trap类型
    // 如果是interrupt 判断是否是timer interrupt
    // 如果是timer interrupt 则打印输出相关信息, 并通过 `clock_set_next_event()` 设置下一次时钟中断
    // `clock_set_next_event()` 见 4.5 节
    // 其他interrupt / exception 可以直接忽略

    // YOUR CODE HERE
    // interrupt
    if (scause % pow(10, 31)){
        // if (scause % 16 == 4){
        //     printk("[U] User Mode Timer Interrupt!\n")
        //     clock_set_next_event();
        // }
        if (scause % 16 == 5){
            printk("[S] Supervisor Mode Timer Interrupt!\n");
            clock_set_next_event();
        }

    }
}

