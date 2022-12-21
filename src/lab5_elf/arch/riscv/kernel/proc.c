//arch/riscv/kernel/proc.c
#include "proc.h"
#include "stdint.h"
#include "defs.h"
#include "elf.h"

extern void __dummy();

struct task_struct* idle;           // idle process
struct task_struct* current;        // 指向当前运行线程的 `task_struct`
struct task_struct* task[NR_TASKS]; // 线程数组, 所有的线程都保存在此

extern unsigned long swapper_pg_dir[512] __attribute__((__aligned__(0x1000)));

extern char uapp_start[];
extern char uapp_end[];

#define MIN(a, b) (((a) > (b)) ? (b) : (a))

static uint64_t load_program(struct task_struct* task) {
    Elf64_Ehdr* ehdr = (Elf64_Ehdr*)uapp_start;

    uint64_t phdr_start = (uint64_t)ehdr + ehdr->e_phoff;
    int phdr_cnt = ehdr->e_phnum;

    Elf64_Phdr* phdr;
    for (int i = 0; i < phdr_cnt; i++) {
        phdr = (Elf64_Phdr*)(phdr_start + sizeof(Elf64_Phdr) * i);
        if (phdr->p_type == PT_LOAD) {
            // alloc space and copy content
            uint64_t u_start = (uint64_t)ehdr + phdr->p_offset;

            uint64_t va = phdr->p_vaddr;
            uint64_t map_user = alloc_page(phdr->p_memsz/PGSIZE + 1);
            char *dst = (char *)map_user;
            char *src = (char *)u_start;
            // copy memory 
            for(uint64_t j = va&0xfff; j < (va&0xfff) + phdr->p_filesz; j++){
                dst[j] = src[j-va&0xfff];
            }
            for( ; va < phdr->p_vaddr + phdr->p_memsz; va += PGSIZE){
                // pretend each user space is started with p_vaddr
                // the va has to be 000 tailed
                create_mapping(task->pgd, va >> 12 << 12, map_user - PA2VA_OFFSET, PGSIZE, 0b11111);
            }
        }
    }

    task->thread.sepc = ehdr->e_entry;
}

void task_init() {
    // 1. 调用 kalloc() 为 idle 分配一个物理页
    // 2. 设置 state 为 TASK_RUNNING;
    // 3. 由于 idle 不参与调度 可以将其 counter / priority 设置为 0
    // 4. 设置 idle 的 pid 为 0
    // 5. 将 current 和 task[0] 指向 idle

    /* YOUR CODE HERE */
    // char *res = (char *)kalloc(10 * sizeof(char));
    uint64_t pgNum = kalloc();
    idle = (struct task_struct*)pgNum;
    idle->state = TASK_RUNNING;
    idle->counter = 0;
    idle->priority = 0;
    idle->pid = 0;
    current = idle;
    task[0] = idle;

    // 1. 参考 idle 的设置, 为 task[1] ~ task[NR_TASKS - 1] 进行初始化
    // 2. 其中每个线程的 state 为 TASK_RUNNING, counter 为 0, priority 使用 rand() 来设置, pid 为该线程在线程数组中的下标。
    // 3. 为 task[1] ~ task[NR_TASKS - 1] 设置 `thread_struct` 中的 `ra` 和 `sp`,
    // 4. 其中 `ra` 设置为 __dummy （见 4.3.2）的地址,  `sp` 设置为 该线程申请的物理页的高地址

    uint64_t Ustack;
    uint64_t va;
    for (int i = 1; i < NR_TASKS; i++ ){
        pgNum = kalloc();
        task[i] = (struct task_struct*)pgNum;
        task[i]->state = TASK_RUNNING;
        task[i]->counter = 0;
        task[i]->priority = rand();
        task[i]->pid = i;

        task[i]->thread.ra = (uint64_t)__dummy;
        task[i]->thread.sp = pgNum + PGSIZE;

        task[i]->pgd = (pagetable_t)alloc_page();
        // task[i]->thread_info->kernel_sp = pgNum + PGSIZE;
        // task[i]->thread_info->user_sp = Ustack + USER_END;

        // map kernel pgdir
        // here just need to map root pg
        for (int j = 0; j < 512; j++){
            task[i]->pgd[j] = swapper_pg_dir[j];
        }

        // uint64_t map_user;
        // // mapping user memory U|-|W|R|V
        // for(va = uapp_start; va < uapp_end; va += PGSIZE){
        //     map_user = alloc_page();
        //     char *dst = (char *)map_user;
        //     char *src = (char *)va;
        //     // copy memory 
        //     for(uint64_t j = 0; j < MIN(0x1000, uapp_end - va); j++){
        //         dst[j] = src[j];
        //     }
        //     // here pretend each user space is started with 0x0
        //     create_mapping(task[i]->pgd, USER_START+va-uapp_start, map_user - PA2VA_OFFSET, PGSIZE, 0b11111);
        // }
        load_program(task[i]);

        Ustack = alloc_page();
        create_mapping(task[i]->pgd, USER_END - PGSIZE, Ustack - PA2VA_OFFSET, PGSIZE, 0b10111);

        uint64_t satp = csr_read(satp);
        // Mode + Asid + ppn
        satp = ((satp >> 44) << 44) | ((unsigned long)task[i]->pgd - PA2VA_OFFSET) >> 12;
        task[i]->pgd = (pagetable_t)satp;

        // task[i]->thread.sepc = USER_START;

        uint64_t sstatus = csr_read(sstatus);
        // SSP SPIE SUM
        sstatus = sstatus & ~(1<<8);
        sstatus = sstatus | 1<<5;
        sstatus = sstatus | 1<<18;
        task[i]->thread.sstatus = sstatus;
        task[i]->thread.sscratch = USER_END;
    }
    printk("...proc_init done!\n");
}

void dummy() {
    uint64_t MOD = 1000000007;
    uint64_t auto_inc_local_var = 0;
    int last_counter = -1;
    while(1) {
        if (last_counter == -1 || current->counter != last_counter) {
            last_counter = current->counter;
            auto_inc_local_var = (auto_inc_local_var + 1) % MOD;
            // printk("[PID = %d] is running. auto_inc_local_var = %d\n", current->pid, auto_inc_local_var);
            printk("[PID = %d] is running. thread space begin at 0x%lx\n", current->pid, (uint64_t)current);
            printk("current->counter = %d\n", current->counter);
        }
    }
}

extern void __switch_to(struct task_struct* prev, struct task_struct* next);

uint64_t thread_offset = __builtin_offsetof(struct task_struct, thread);

void switch_to(struct task_struct* next) {
    /* YOUR CODE HERE */
    if(current != next){
        struct task_struct *tmp = current;
        current = next;
        // here the cpu_switch_to: change reg
        __switch_to(tmp, next);
/*
        __asm__ volatile(
            "addi sp, sp, -8\n"
            "sd ra, 0(sp)\n"
            "mv a0, %[current]\n"
            "mv a1, %[next]\n"
            "jal ra, __switch_to"
            "ld ra, 0(sp)\n"
            "addi sp, sp, 8\n"
            : 
            : [current] "r" (current), [next] "r" (next)
            : "memory"
        );
*/
    }
}

void do_timer(void) {
    // 1. 如果当前线程是 idle 线程 直接进行调度
    // 2. 如果当前线程不是 idle 对当前线程的运行剩余时间减1 若剩余时间仍然大于0 则直接返回 否则进行调度

    /* YOUR CODE HERE */
    if(current == idle){
        schedule();
    }else{
        current->counter--;
        if(current->counter)
            return;
        else
            schedule();
    }
}

void schedule(void) {
    /* YOUR CODE HERE */
    // switch_to(task[rand()%32]);
#ifdef SJF
    struct task_struct *next = idle;
    uint64_t time_max_more = TIME_MAX + 1;
    uint64_t min = time_max_more;
    while (1){
        for (int i = 1; i < NR_TASKS; i++){
            if (min > task[i]->counter && task[i]->counter && task[i]->state == TASK_RUNNING){
                next = task[i];
                min = task[i]->counter;
            }
        }
        if (min != time_max_more) break;
        if (min == time_max_more){
            for (int i = 1; i < NR_TASKS; i++){
                task[i]->counter = rand();
                // task[i]->counter = 1;
                // printk("SET [PID = %d COUNTER = %d]\n", i, task[i]->counter);
            }
            // continue;
        }
    }
    // printk("next = %x\n", (uint64_t)next);
    switch_to(next);
#endif
#ifdef PRIORITY
    struct task_struct *next = idle;
    uint64_t max = 0;
	while (1) {
        for (int i = 1; i < NR_TASKS; i++){
            if (max < task[i]->counter && task[i]->state == TASK_RUNNING){
                next = task[i];
                max = task[i]->counter;
            }
        }
		if (max) break;
        for (int i = 1; i < NR_TASKS; i++){
            task[i]->counter = (task[i]->counter >> 1) + task[i]->priority;
            printk("SET [PID = %d PRIORITY = %d COUNTER = %d]\n", i, task[i]->priority, task[i]->counter);
        }
	}
	switch_to(next);
#endif
}

