#include "defs.h"
#include "proc.h"
#include "../include/syscall.h"

void trap_handler(unsigned long scause, unsigned long sepc, struct pt_regs *regs) {
    // 通过 `scause` 判断trap类型
    // 如果是interrupt 判断是否是timer interrupt
    // 如果是timer interrupt 则打印输出相关信息, 并通过 `clock_set_next_event()` 设置下一次时钟中断
    // `clock_set_next_event()` 见 4.5 节
    // 其他interrupt / exception 可以直接忽略

    // printk("scause: %lx", scause);
    // while(1);

    // interrupt 1 started
    if (scause & (1 << 31)){
        // if (scause % 16 == 4){
        //     printk("[U] User Mode Timer Interrupt!\n")
        // }
        
        if (scause % 16 == 5){
            // printk("[S] Supervisor Mode Timer Interrupt!\n");
            clock_set_next_event();
            do_timer();
        } else {
            printk("[S] Unhandled trap, ");
            printk("scause: %lx, ", scause);
            printk("stval: %lx, ", regs->stval);
            printk("sepc: %lx\n", regs->sepc);
            while (1);
        }
    } else{
        if (scause % 16 == 8) {
            // printk("ECALL_FROM_U_MODE Interrupt!\n");
            syscall(regs);
        } else if (scause % 16 == 12){
            printk("Instruction Page Fault!\n");
            do_page_fault(regs);
        } else if (scause % 16 == 13){
            printk("Load Page Fault!\n");
            do_page_fault(regs);
        } else if (scause % 16 == 15){
            printk("Store/AMO Page Fault!\n");
            // printk("scause: %lx, ", scause);
            // printk("stval: %lx, ", regs->stval);
            // printk("sepc: %lx\n", regs->sepc);
            do_page_fault(regs);
            // while(1);
        } else {
            printk("[S] Unhandled trap, ");
            printk("scause: %lx, ", scause);
            printk("stval: %lx, ", regs->stval);
            printk("sepc: %lx\n", regs->sepc);
            while (1);
        }
    }
}

extern struct task_struct* current;        // 指向当前运行线程的 `task_struct`
extern char ramdisk_start[];
#define MAX(a, b) ((a) > (b) ? (a) : (b))

void do_page_fault(struct pt_regs *regs) {
    /*
     1. 通过 stval 获得访问出错的虚拟内存地址（Bad Address）
     2. 通过 find_vma() 查找 Bad Address 是否在某个 vma 中
     3. 分配一个页，将这个页映射到对应的用户地址空间
     4. 通过 (vma->vm_flags & VM_ANONYM) 获得当前的 VMA 是否是匿名空间
     5. 根据 VMA 匿名与否决定将新的页清零或是拷贝 uapp 中的内容
    */
    uint64_t bad_address = regs->stval;
    struct vm_area_struct *vma = find_vma(current, bad_address);
    if(vma != 0){
        uint64_t new_page = alloc_page();
        // 非匿名空间
        if(!(vma->vm_flags & VM_ANONYM)){
            printk("vm_start: %lx, vm_end: %lx, vm_size: %lx\n", vma->vm_start, vma->vm_end, vma->vm_content_size_in_file);
            uint64_t disk_file_start = (uint64_t)ramdisk_start + vma->vm_content_offset_in_file;
            uint64_t file_end = vma->vm_start + vma->vm_content_size_in_file;
            printk("disk_file_start: %lx, file_end: %lx, stval: %lx\n", disk_file_start, file_end, regs->stval);
            char *dst = (char *)new_page;
            char *src = (char *)disk_file_start;
            // tip: uint may overflow
            // uint64_t next_map_size = file_end > bad_address ? file_end - bad_address : 0;
            uint64_t next_map_size;
            if(file_end / PGSIZE > bad_address / PGSIZE){
                next_map_size = PGSIZE;
            } else if(file_end / PGSIZE == bad_address / PGSIZE){
                next_map_size = file_end & 0xfff;
            } else {
                next_map_size = 0;
            }
            for(uint64_t j = bad_address&0xfff; j < next_map_size; j++){
                dst[j] = src[j - bad_address&0xfff];
            }
            printk("next_map_size: %lx\n", next_map_size);
            printk("non-anonymous\n");
        } else {
            printk("anonymous\n");
        }
        // User and valid
        // here cannot use else if !!
        uint64_t flag = 0b10001;
        if(vma->vm_flags & VM_X_MASK){
            flag |= 0b01000;
        }
        if(vma->vm_flags & VM_W_MASK){
            flag |= 0b00100;
        }
        if(vma->vm_flags & VM_R_MASK){
            flag |= 0b00010;
        }
        printk("flag:%lx\n", flag);
        create_mapping(current->pgd, bad_address >> 12 << 12, new_page - PA2VA_OFFSET, PGSIZE, flag);
        printk("finish\n");
    }
}
