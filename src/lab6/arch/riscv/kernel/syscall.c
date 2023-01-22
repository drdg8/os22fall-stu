#include "proc.h"
// #include "syscall.h"
#include "../include/syscall.h"
#include "printk.h"

extern struct task_struct* current; 
void syscall(struct pt_regs *regs) {
    //get pid
    if(regs->x[17] == SYS_GETPID) {
        // here a0 for x[10]
        regs->x[10] = current->pid;
    }
    //sys_write
    else if (regs->x[17] == SYS_WRITE) {
        // fd
        if (regs->x[10] == 1) {
            // count
            uint64_t end = regs->x[12];
            char *out = regs->x[11];
            out[end] = '\0';
            regs->x[10] = printk(out);
            // ((char*)(regs->x[11]))[end] = '\0';
            // regs->x[10] = printk((char *)(regs->x[11]));
        }
    }
}