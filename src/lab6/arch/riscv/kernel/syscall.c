#include "proc.h"
// #include "syscall.h"
#include "../include/syscall.h"
#include "printk.h"

extern struct task_struct* current; 
extern struct task_struct* task[NR_TASKS];

void syscall(struct pt_regs *regs) {
    if(regs->x[17] == SYS_GETPID) {
        // here a0 for x[10]
        regs->x[10] = current->pid;
    } else if (regs->x[17] == SYS_WRITE) {
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
    } else if (regs->x[17] == SYS_CLONE) {
        sys_clone(regs);
    }
}

// extern void __ret_from_folk(void);

uint64_t sys_clone(struct pt_regs *regs) {
    /*
    深拷贝一份页表以及 VMA 中记录的用户态的内存，还需要复制内核态的寄存器状态和内核态的内存。
    并且在最后，需要将 task “伪装”成是因为调度而进入了 Ready Queue。

     1. 参考 task_init 创建一个新的 task, 将的 parent task 的整个页复制到新创建的 
        task_struct 页上(这一步复制了哪些东西?）.
        将 thread.ra 设置为 __ret_from_fork, 并正确设置 thread.sp
        (仔细想想，这个应该设置成什么值?可以根据 child task 的返回路径来倒推)

     2. 利用参数 regs 来计算出 child task 的对应的 pt_regs 的地址，
        并将其中的 a0, sp, sepc 设置成正确的值(为什么还要设置 sp?)

     3. 为 child task 申请 user stack, 并将 parent task 的 user stack 
        数据复制到其中。 (既然 user stack 也在 vma 中，这一步也可以直接在 5 中做，无需特殊处理)

     3.1. 同时将子 task 的 user stack 的地址保存在 thread_info->
        user_sp 中，如果你已经去掉了 thread_info，那么无需执行这一步

     4. 为 child task 分配一个根页表，并仿照 setup_vm_final 来创建内核空间的映射

     5. 根据 parent task 的页表和 vma 来分配并拷贝 child task 在用户态会用到的内存

     6. 返回子 task 的 pid
    */
    uint64_t pgNum = alloc_page();
    char *dst = (char *)pgNum;
    char *src = (char *)current;
    for(uint64_t i = 0; i < PGSIZE; i++){
        dst[i] = src[i];
    }
    struct task_struct *folk_task = (struct task_struct*)pgNum;
    bool find_empty_task = false;
    for(uint64_t i = 1; i < NR_TASKS; i++){
        if(task[i] == NULL){
            folk_task->pid = i;
            folk_task->thread.ra = __ret_from_folk;
            folk_task->thread.sp = ;
            task[i] = folk_task;
            find_empty_task = true;
        }
    }

}