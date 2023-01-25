#include "defs.h"
#include "stdint.h"

// #define NR_TASKS  (1 + 31) // 用于控制 最大线程数量 （idle 线程 + 31 内核线程）
#define NR_TASKS 5

#define TASK_RUNNING    0 // 为了简化实验, 所有的线程都只有一种状态

#define PRIORITY_MIN 1
#define PRIORITY_MAX 10

#define TIME_MAX 10

typedef unsigned long* pagetable_t;

/* 用于记录 `线程` 的 `内核栈与用户栈指针` */
// struct thread_info {
//     uint64_t kernel_sp;
//     uint64_t user_sp;
// };

/* 线程状态段数据结构 */
struct thread_struct {
    uint64_t ra;
    uint64_t sp;
    uint64_t s[12];

    uint64_t sepc, sstatus, sscratch, satp; 
};

/* 线程数据结构 */
struct task_struct {
    // struct thread_info* thread_info;
    uint64_t state;    // 线程状态
    uint64_t counter;  // 运行剩余时间
    uint64_t priority; // 运行优先级 1最低 10最高
    uint64_t pid;      // 线程id

    struct thread_struct thread;

    pagetable_t pgd;
};

/* 线程初始化 创建 NR_TASKS 个线程 */
void task_init();

/* 在时钟中断处理中被调用 用于判断是否需要进行调度 */
void do_timer();

/* 调度程序 选择出下一个运行的线程 */
void schedule();

/* 线程切换入口函数*/
void switch_to(struct task_struct* next);

/* dummy funciton: 一个循环程序, 循环输出自己的 pid 以及一个自增的局部变量 */
void dummy();

