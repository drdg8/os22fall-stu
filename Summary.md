# os22fall

[toc]

## 总
使用 qemu 结合 opensbi, 通过交叉编译链实现包括**中断处理、进程调度、虚拟地址、页表、用户态内核态切换、缺页、fork**等简单功能的操作系统. 

使用RISC-V, makefile, gdb, c++

了解了 OS 的基础内容与 riscv 汇编. 

## 使用方法

所有部分均为精简. 详细知识点和实验流程请查看实验doc了解. 

可查看最新实验文档, 有部分修改, 但整体内容未改变. 

https://zju-sec.github.io/os24fall-stu/

此实验代码一以贯之, 可按顺序浏览, 了解所有文件的作用以及如何组织运行. 

实验目的 / 基础知识 / 代码目录 / 代码框架 / 运行流程 / 实验精简 / 思考

## 待完善

riscv内容: 可查看riscv/目录

Lab2 基础知识
Lab2 思考
Lab3 思考
Lab4 思考
Lab6 

## Lab 0: GDB & QEMU 调试 64 位 RISC-V LINUX

### 实验目的

- 使用交叉编译工具, 完成Linux内核代码编译
- 使用QEMU运行内核
- 熟悉GDB和QEMU联合调试

### 基础知识精简

```gdb
# in A shell
make Debug 

# in B shell
gdb-multiarch /home/drdg8/lab/os22fall-stu/src/lab1/vmlinux

# in gdb
target remote :1234
b _start    break 设置断点
c           continue
n           单步调试 (过程，函数直接执行)
si          执行单条指令
bt          backtrace 看函数调用栈和层级
p _traps    print 打印值及地址
finish      跳出函数
i r ra      info reg ra
i r stvec
x/4x <addr>
```

## Lab 1: RV64 内核引导

### 实验目的

* 学习 RISC-V 汇编， 编写 head.S 实现跳转到内核运行的第一个 C 函数。
* 学习 OpenSBI，理解 OpenSBI 在实验中所起到的作用，并调用 OpenSBI 提供的接口完成字符的输出。
* 学习 Makefile 相关知识， 补充项目中的 Makefile 文件， 来完成对整个工程的管理。

### 基础知识

* 计算机运行到OS: 硬件基础初始化后, pc移动到内存中的Bootloader(M模式), Bootloader初始化硬件, 加载内核, 之后开始运行Kernel

* opensbi作为 M mode 和 S mode 的接口规范. Syscall作为 S mode 和 U mode 的接口规范.

* Makefile 是一个工程文件的编译规则，描述了整个工程的编译和链接流程

* vmlinux.lds 作为链接脚本（Linker Script）指定程序的内存布局， GNU ld 即链接器，用于将 `*.o` 文件（和库文件）链接成可执行文件。

* vmlinux 通常指 Linux Kernel 编译出的可执行文件 (Executable and Linkable Format / ELF)

### 代码目录

```
.
├── Makefile                    *A
├── arch
│   └── riscv
│       ├── Makefile            *A
│       ├── include
│       │   ├── defs.h          *A
│       │   └── sbi.h           *A
│       └── kernel
│           ├── Makefile        *A
│           ├── head.S          *A
│           ├── sbi.c           *A
│           └── vmlinux.lds     *A
├── include
│   ├── print.h                 *A
│   └── types.h                 *A
├── init
│   ├── Makefile                *A
│   ├── main.c                  *A
│   └── test.c                  *A
└── lib
    ├── Makefile                *A
    └── print.c                 *A
```

### 代码框架

- 非arch部分
    * Makefile 描述工程的编译规则
    * print相关函数使用ecall实现printk, 打印错误信息和字符串.
    * main.c的start_kernel作为程序起始点.
    * test.c循环while(1)等待interrupt.
    * types.h定义ull为uint64. 在新版中已被合并.

- arch部分
    * Makefile 描述工程的编译规则
    * defs.h定义需要函数, 本实验是read_csr write_csr
    * sbi相关函数实现ecall相关函数, 通过ecall联通S mode和M mode. 
    * vmlinux.lds 作为链接脚本指定程序的内存布局 如何将`*.o`文件（和库文件）链接成可执行文件

### 运行流程

1. Makefile 
    - 定义如何编译 运行
1. vmlinux.lds 
    - 连接脚本指定 ENTRY = _start / BASE_ADDR, 以及各内存布局
1.  _start: head.S 
    - 定义程序栈
    - 跳转至 start_kernel() 在 main.c
3. start_kernel() 调用 printk() 和 read_csr / write_csr
    - printk 调用 ecall. 
        - ecall 由内核态实现, 调用sbi_ecall, 由opensbi定义. 
    - write / read_csr 宏由defs.h 定义csrw / csrr实现

### 实验精简

- 编写head.S
    * 为即将运行的第一个 C 函数设置程序栈。通过跳转指令，跳转至 main.c 中的 start_kernel 函数
- 完善 Makefile 
- 补充 sbi.c
    * 实现调用 OpenSBI 接口的功能。 sbi_ecall
- `puts()` 和 `puti()`
    * 调用`sbi_ecall` , 完成 `puts()` 和 `puti()` 。
- 修改 defs
* read_csr write_csr

## Lab 2: RV64 时钟中断处理

### 实验前言

在 lab1 中我们成功的将一个最简单的 OS 启动起来， 但还没有办法与之交互。我们在课程中讲过操作系统启动之后由事件（event）驱动，在本次实验中我们将引入一种重要的事件 trap，trap 给了 OS 与硬件、软件交互的能力。在 lab1 中我们介绍了在 RISC-V 中有三种特权级 ( M 态、 S 态、 U 态 )， 在 Boot 阶段， OpenSBI 已经帮我们将 M 态的 trap 处理进行了初始化，这一部分不需要我们再去实现，因此本次试验我们重点关注 S 态的 trap 处理。

### 实验目的
* 学习 RISC-V 的 trap 处理相关寄存器与指令，完成对 trap 处理的初始化。
* 理解 CPU 上下文切换机制，并正确实现上下文切换功能。
* 编写 trap 处理函数，完成对特定 trap 的处理。
* 调用 OpenSBI 提供的接口，完成对时钟中断事件的设置。

### 基础知识
* Interrupt 中断, 正常的, 异步的, 如keyboard interrupt
* Exception 异常, 异常的, 同步的, 如illegal instruction

* S Mode相关寄存器:
    - sstatus: 存储trap相关设置. Supervisor Status Register.
    - sie: 是否开启trap. Supervisor Interrupt Eable Register
    - stvec: 中断向量表基址, 指向中断处理入口函数(单个)或中断向量表(多个). Supervisor Trap Vector Base Address Register
    - scause: 记录 trap 发生的原因. Supervisor Cause Register
    - sepc: 记录触发 exception 的指令地址. Supervisor Exception Program Counter

* 特权指令
    - ecall: Environment Call
        - S 态执行，触发 `ecall-from-s-mode-exception`，从而进入 M Mode 下的处理流程(如设置定时器等)
        - U 态执行，触发 `ecall-from-u-mode-exception`，从而进入 S Mode 下的处理流程(常用来进行系统调用)。
    - sret: S 态 trap 返回, 通过 `sepc` 来设置 `pc` 的值

* 上下文处理: 在处理 trap 前，需对系统的当前状态(寄存器)进行保存，在处理完成之后，我们再将系统恢复至原先的状态，就可以确保之前的程序继续正常运行。

* 时钟中断: Superviosr Timer Interrupt. 非重点, 不介绍.

* ricsv寄存器表.

#### Interrupt 和 Exception

- 中断(Interrupt)：是为了设备与CPU之间的通信。典型的有如服务请求，任务完成提醒等。比如我们熟知的时钟中断，硬盘读写服务请求中断。中断的发生与系统处在用户态还是在内核态无关，只决定于EFLAGS寄存器的一个标志位。

中断是异步的，因为从逻辑上来说，中断的产生与当前正在执行的进程无关。事实上，中断是如此有用，Linux用它来统计时钟，进行硬盘读写等。

中断是正常的, 在程序正常运行时就会产生.

- 异常(Exception)：异常是由当前正在执行的进程产生。异常包括很多方面，有出错（fault），有陷入（trap），也有可编程异常（programmable exception）。

Exception是不正常的, 是程序出现错误时产生.

出错（fault）和陷入（trap）最重要的一点区别是他们发生时所保存的EIP值的不同。出错（fault）保存的EIP指向触发异常的那条指令；而陷入（trap）保存的EIP指向触发异常的那条指令的下一条指令。

因此，当从异常返回时，出错（fault）会重新执行那条指令；而陷入（trap）就不会重新执行。比如我们熟悉的缺页异常（page fault），由于是fault，所以当缺页异常处理完成之后，还会去尝试重新执行那条触发异常的指令（那时多半情况是不再缺页）。

陷入的最主要的应用是在调试中，被调试的进程遇到你设置的断点，会停下来等待你的处理，等到你让其重新执行了，它当然不会再去执行已经执行过的断点指令。

https://www.cnblogs.com/johnnyflute/p/3765008.html

在本实验中 Trap为Interrupt和Exception的总称. 

### 代码目录

```
.
├── Makefile
├── arch
│   └── riscv
│       ├── Makefile
│       ├── include
│       │   ├── defs.h      *M
│       │   └── sbi.h
│       └── kernel
│           ├── Makefile
│           ├── clock.c     *A
│           ├── entry.S     *A
│           ├── head.S      *M
│           ├── sbi.c
│           ├── trap.c      *A
│           └── vmlinux.lds *M
├── include
│   ├── printk.h            *A
│   ├── stddef.h            *A
│   └── types.h
├── init
│   ├── Makefile
│   ├── main.c
│   └── test.c
└── lib
    ├── Makefile
    └── printk.c            *A
```

### 代码框架

- 非arch部分
    * printk相关函数作为简化print实现. 
    * stddef.h printk实现辅助函数. 
- arch部分
    * 修改vmlinux.lds, 加入.text.init 和 .text.entry 部分
    * 修改head.S, 初始化CSR使其支持时钟中断. 
    * 加入entry.S, 实现_traps支持. 上下文切换并执行trap_handler
    * 加入trap.c, 实现trap_handler, 查看scause, sepc识别时钟中断. 
    * 加入clock.c, 实现设置下一个时钟中断事件. 

### 运行流程

1. Makefile 
    - 定义如何编译 运行
1. vmlinux.lds 
    - 连接脚本指定 ENTRY = _start / BASE_ADDR, 以及各内存布局
1.  _start: head.S 
    - 定义程序栈
    - 开启trap处理
        - stvec _traps
        - sie[STIE]
        - 第一次时钟中断
        - sstatus
    - jal ra, start_kernel. 
1. start_kernel(): main.c
    - test()
1. test(): test.c 
    - while(1)

1. 当出现第二次时钟中断时, opensbi 通过 stvec 跳转到 _traps
1. _traps: entry.S
    - 上下文切换(开始/结束)
        - save test() register
    - trap_handler()
1. trap_handler(): trap.c
    - scause, sepc识别时钟中断
        - clock_set_next_event()
1. clock_set_next_event(): clock.c 
    - get_cycles(): `rdtime`
    - clock_set_next_event(): `sbi_ecall`
1. return clock_set_next_event() 
    -> trap_handler() 
    -> _traps 
    -> while(1). 
1. 直到下一个时钟中断. 重复上述过程.
    
### 实验精简

1. 在head.S中开启trap处理. 
    1. 设置 `stvec`， 将 `_traps` 所表示的地址写入 `stvec`
    2. 开启时钟中断，将 `sie[STIE]` 置 1。
    3. 设置第一次时钟中断，参考 `clock_set_next_event()` 
        - jal ra, clock_set_next_event
    4. 开启 S 态下的中断响应， 将 `sstatus[SIE]` 置 1。

2. `_traps`的实现在entry.S
    1. 保存 CPU 的寄存器（上下文）到内存中（栈上）。
    2. 将 `scause` 和 `sepc` 中的值传入 trap 处理函数 `trap_handler`
    3. 在完成对 trap 的处理之后， 我们从内存中（栈上）恢复CPU的寄存器（上下文）。
    
3. `trap.c` 中实现 `trap_handler()`
    - 查看`scause`和 `sepc` 跳转到clock.c时钟中断处理函数. 

4. 实现时钟中断相关函数: clock.c
    - get_cycles(): `rdtime`
    - clock_set_next_event: ()  `sbi_ecall`

### 思考

总结一下 RISC-V 的 calling convention，并解释 Caller / Callee Saved Register 有什么区别？

!!! tip "答案"
    就是把要传入的参数放到对应的寄存器里，a0-a7分别对应传入的7个参数，然后跳转到被调用的函数，一般用 `jal ra, Func1`指令。

    <p align='center'>
        <img src='image/report/15.png'/>
    </p>

    对于 Caller / Callee Saved register，可以看下图：

    <p align='center'>
        <img src='image/report/16.png'/>
    </p>
    根据定义，需要被caller saved 的就是caller saved register，有ra等。对于Callee Saved Register，也有sp,s0等。


## Lab 3: RV64 内核线程调度

### 实验前言

在 lab2 中, 我们利用 trap 赋予了 OS 与软件, 硬件的交互能力。但是目前我们的 OS 还不具备多进程调度以及并发执行的能力。在本次实验中, 我们将利用时钟中断, 来实现多进程的调度以使得多个进程/线程并发执行。

### 实验目的

* 了解线程概念, 并学习线程相关结构体, 并实现线程的初始化功能。
* 了解如何使用时钟中断来实现线程的调度。
* 了解线程切换原理, 并实现线程的切换。
* 掌握简单的线程调度算法, 并完成两种简单调度算法的实现。

### 基础知识
- 进程是运行中的程序. 

- 本实验中线程相关属性
    * `线程ID`：用于唯一确认一个线程。
    * `运行栈`：每个线程有独立的运行栈, 保存运行时的数据。
    * `执行上下文`：当线程离开执行状态时, 需保存其上下文（`状态寄存器`）, 以便之后将其恢复, 继续运行。
    * `运行时间片`：为每个线程分配的运行时间。
    * `优先级`： 配合调度算法, 选出下一个执行的线程。

- [线程切换流程图](#线程切换流程图)

#### 线程切换流程图

<h5 id = "线程切换流程图">线程切换流程图</h5>

```
           Process 1         Operating System            Process 2
               +
               |                                            X
 P1 executing  |                                            X
               |                                            X
               v Timer Interrupt Trap                       X
               +---------------------->                     X
                                      +                     X
               X                  do_timer()                X
               X                      +                     X
               X                  schedule()                X
               X                      +                     X
               X              save state to PCB1            X
               X                      +                     X
               X           restore state from PCB2          X
               X                      +                     X
               X                      |                     X
               X                      v Timer Interrupt Ret
               X                      +--------------------->
               X                                            |
               X                                            |  P2 executing
               X                                            |
               X                       Timer Interrupt Trap v
               X                      <---------------------+
               X                      +
               X                  do_timer()
               X                      +
               X                  schedule()
               X                      +
               X              save state to PCB2
               X                      +
               X           restore state from PCB1
               X                      +
               X                      |
                 Timer Interrupt Ret  v
               <----------------------+
               |
 P1 executing  |
               |
               v
```

* 在每次处理时钟中断时, 操作系统首先会将当前线程的运行剩余时间减少一个单位。之后根据调度算法来确定是继续运行还是调度其他线程来执行。
* 在进程调度时, 操作系统会遍历所有可运行的线程, 按照一定的调度算法选出下一个执行的线程。最终将选择得到的线程与当前线程切换。
* 在切换的过程中, 首先我们需要保存当前线程的执行上下文, 再将将要执行线程的上下文载入到相关寄存器中, 至此我们就完成了线程的调度与切换。

### 代码目录

```
.
├── Makefile
├── arch
│   └── riscv
│       ├── Makefile
│       ├── include
│       │   ├── defs.h          *M
│       │   ├── mm.h            *A
│       │   ├── proc.h          *A
│       │   └── sbi.h
│       └── kernel
│           ├── Makefile
│           ├── clock.c
│           ├── entry.S         *M
│           ├── head.S          *M
│           ├── mm.c            *A
│           ├── proc.c          *A
│           ├── sbi.c
│           ├── trap.c          *M
│           └── vmlinux.lds
├── include
│   ├── printk.h
│   ├── rand.h                  *A
│   ├── stddef.h
│   ├── string.h                *A
│   └── types.h
├── init
│   ├── Makefile
│   ├── main.c
│   └── test.c
└── lib
    ├── Makefile
    ├── printk.c
    ├── rand.c                  *A
    └── string.c                *A
```

### 代码框架

* mm.c/h: 一个简单的物理内存管理接口
* rand.c/h: 提供伪随机数序列
* string.c/.h: 提供 memset 接口用以初始化一段内存空间。
* 修改 defs.h, 加入物理内存管理所需宏
* proc.c/h: 实验重点, 进行线程的管理
* 修改head.S 加入mm_init 和 task_init
* 修改entry.S 加入_switch_to 和 __dummy
* 修改trap.c 加入do_timer()

```c 
// proc.c/.h
/* 线程状态段数据结构 */
struct thread_struct {
    uint64 ra;
    uint64 sp;
    uint64 s[12];
};

/* 线程数据结构 */
struct task_struct {
    uint64 state;    // 线程状态
    uint64 counter;  // 运行剩余时间
    uint64 priority; // 运行优先级 1最低 10最高
    uint64 pid;      // 线程id

    struct thread_struct thread;
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
```

### 运行流程

1. Makefile 
    - 定义如何编译 运行
1. vmlinux.lds 
    - 连接脚本指定 ENTRY = _start / BASE_ADDR, 以及各内存布局
1.  _start: head.S 
    - 定义程序栈
    - mm_init           *A
    - task_init         *A
    - 开启trap处理
        - stvec _traps
        - sie[STIE]
        - 第一次时钟中断
        - sstatus
    - jal ra, start_kernel. 
1. mm_init: mm.c
    - 提供 kalloc() 用于数据页的分配
1. task_init: proc.c
    - 根据 task_struct 数据结构设置 idle 和 task[] 的初始化.
    - ra 设置为 __dummy
    - sp 设置为物理页的高地址
1. start_kernel(): main.c
    - test()
1. test(): test.c 
    - while(1)

1. 当出现第二次时钟中断时, opensbi 通过 stvec 跳转到 _traps
1. _traps: entry.S
    - 上下文切换(开始/结束)
        - save test() register
    - 跳转至 trap_handler()
1. trap_handler(): trap.c
    - 查看 scause, sepc 识别时钟中断
        - clock_set_next_event()
        - do_timer()                    *A
1. clock_set_next_event(): clock.c
    - get_cycles(): `rdtime`
    - clock_set_next_event(): `sbi_ecall`
1. do_timer(): proc.c 
    - idle 跳转 schedule() 调度
    - 非idle 运行时间-1
        - `>0` 返回 
        - `=0` 调度
1. schedule(): proc.c 
    - 根据 SJF / PRIORITY 选择 next 线程(task_struct)
    - switch_to(next)
1. switch_to(): proc.c 
    - 如果 `current task != next task` : __switch_to(prev, next)
1. __switch_to(): entry.S 
    - 上下文切换. 
        - 在 __switch_to 时, 才是真正意义的context_switch, 也就是存储旧值, 加载新值. 
        - save `prev` register.
            - 存caller saved register.
            - 这里 idle 的 ra如下. 
                > __switch__to -> switch_to() -> schedule() -> dotimer() -> trap_handler() -> _traps -> test.c:while(1)
            - `idle` 状态从此结束 不再参与调度. 
        - load `next` register.
        - 在第二次时钟中断时, `ret`命令 pc 返回 ra, 又因为 task[] ra 被设置成 __dummy, 跳转至 __dummy. 
1. __dummy: entry.S
    - 设置 sepc 为 dummy
    - `sret` 返回 dummy().      // 真是非常神奇啊, context_switch 之后就变成完全不同的线程了 
1. dummy(): proc.c 
    - while(1) 
        - getpid() print
1. 直到下一个时钟中断. 下次时间中断进入 _traps 存储这次 task 的 sp / ra, task 进入休眠状态, 直到下次schedule() -> switch_to(next) 被调度时通过 task_struct -> ra 重新运行, 回到dummy(). 
> 重新运行时 __switch_to ret ra -> __switch_to end -> switch_to end -> schedule() end -> do_timer() end -> trap_handler() end -> _traps -> dummy()

### 实验精简

- mm_init
    - 初始化_ekernel到PHY_END的内存空间并使用 struct kmem 管理.
    - 提供kalloc()用于数据页的分配
```c
struct {
    struct run *freelist;
} kmem;

void mm_init(void) {
    kfreerange(_ekernel, (char *)PHY_END);
    printk("...mm_init done!\n");
}

uint64 kalloc() {
    struct run *r;

    r = kmem.freelist;
    kmem.freelist = r->next;
    
    memset((void *)r, 0x0, PGSIZE);
    return (uint64) r;
}
```

- task_init
    * 在初始化线程的时候，为每个线程分配一个 4 KiB 的物理页，我们将 `task_struct` 存放在该页的低地址部分，将线程的栈指针 `sp` 指向该页的高地址。具体内存布局如下图所示：

```
                    ┌─────────────┐◄─── High Address
                    │             │
                    │    stack    │
                    │             │
                    │             │
              sp ──►├──────┬──────┤
                    │      │      │
                    │      ▼      │
                    │             │
                    │             │
                    │             │
                    │             │
    4KiB Page       │             │
                    │             │
                    │             │
                    │             │
                    ├─────────────┤
                    │             │
                    │             │
                    │ task_struct │
                    │             │
                    │             │
                    └─────────────┘◄─── Low Address
```
- 
    * 当我们的 OS 运行起来的时候，其本身就是一个线程（idle 线程），但是我们并没有为它设计好 `task_struct`，所以第一步我们要：
        - 为 `idle` 设置好 `task_struct` 的内容；
        - 将 `current`，`task[0]` 都指向 `idle`；
    * 为了方便起见，我们将 `task[1]` ~ `task[NR_TASKS - 1]` 全部初始化，这里和 `idle` 设置的区别在于要为这些线程设置 `thread_struct` 中的 `ra` 和 `sp`，具体见代码：

```c 
    void task_init(){
    // 3. 为 task[1] ~ task[NR_TASKS - 1] 设置 thread_struct 中的 ra 和 sp
    //     - ra 设置为 __dummy（见 4.2.2）的地址
    //     - sp 设置为该线程申请的物理页的高地址
    }
```

- do_timer
    - 如果当前线程是 idle 线程 直接进行调度
    - 如果当前线程不是 idle 对当前线程的运行剩余时间减1 若剩余时间仍然大于0 则直接返回 否则进行调度

- schedule(),  线程调度算法实现 
    * [Linux v0.11 调度算法代码](https://elixir.bootlin.com/linux/0.11/source/kernel/sched.c#L122)实现一个优先级调度算法
    * `task_init` 的时候随机为各个线程赋予了优先级
    * 调度时选择 `counter` 最大的线程运行
    * 如果所有线程 `counter` 都为 0，则令所有线程 `counter = priority`
        - 即优先级越高，运行的时间越长，且越先运行
        - 设置完后需要重新进行调度
    * 最后通过 `switch_to` 切换到下一个线程

- switch_to
    * 判断下一个执行的线程 `next` 与当前的线程 `current` 是否为同一个线程，如果是同一个线程，则无需做任何处理，否则调用 `__switch_to` 进行线程切换：

- __switch_to
    * 保存当前线程的 `ra`，`sp`，`s0~s11` 到当前线程的 `thread_struct` 中；
    * 将下一个线程的 `thread_struct` 中的相关数据载入到 `ra`，`sp`，`s0~s11` 中进行恢复

- __dummy
    * 在 `__dummy` 中将 sepc 设置为 `dummy()` 的地址，并使用 `sret` 从 S 模式中返回.

### 思考
1. 在 RV64 中一共用 32 个通用寄存器, 为什么 context_switch 中只保存了14个?

因为context_swtich是被schedule调用的，因此其作为callee只需保存callee需要保存的寄存器即可。

下图就展示了callee需要保存的reg，其中就有13个。而ra则是作为context_switch实现的核心，会随着进程改变而改变，因此旧值也需要保存。

<p>
    <img src='os_lab3/10.png'>
</p>

2. 用gdb追踪一次完整的线程切换流程, 并关注每一次 ra 的变换

首先是第一次进入switch_to，可以看到ra是schedule的最后位置。再进入__switch_to，可以看到现在的ra是switch_to函数。

<p>
    <img src='os_lab3/11.png'><br>
</p>

之后在__switch_to进行context switch之后，可以看到现在的ra就是新进程里的__dummy了。

经过__dummy之后, 由于sepc被设置为dummy, 并使用sret, pc就返回dummy循环, 直到下次时钟interrupt, 其从dummy中进入_traps, 这也是为什么下一次_traps存储的ra是dummy的原因. 

sret返回sepc, ret返回pc.

从理论来说, __dummy中sepc设置成dummy, pc跳转到dummy, 相关的sp, a0/a1之类的也需要跟着改变, 但实际上由于dummy不需要参数, sp也是用的内核栈, 需要caller修改的寄存器都可以置0, 因此只需要修改pc即可. 

<p>
    <img src='os_lab3/12.png'><br>
</p>

再第二次进入switch_to，可以看到前一个进程的ra还在switch_to函数中。

<p>
    <img src='os_lab3/13.png'><br>
</p>

之后进行完context switch之后，可以发现ra不是指向__dummy了，而是上次没做完的switch_to.

<p>
    <img src='os_lab3/14.png'><br>
</p>

switch_to出来后进入schedule的末尾，再进入do_timer，trap_handler,最后从_trap出来，load出下一个进程的ra,也就是dummy

<p>
    <img src='os_lab3/15.png'><br>
    <img src='os_lab3/16.png'><br>
    <img src='os_lab3/17.png'><br>
</p>

## Lab 4: RV64 虚拟内存管理

### 实验前言
在 lab3 中我们赋予了 OS 对多个线程调度以及并发执行的能力，由于目前这些线程都是内核线程，因此他们可以共享运行空间，即运行不同线程对空间的修改是相互可见的。但是如果我们需要线程相互**隔离**，以及在多线程的情况下更加**高效**的使用内存，就必须引入`虚拟内存`这个概念。

虚拟内存可以为正在运行的进程提供独立的内存空间，制造一种每个进程的内存都是独立的假象。同时虚拟内存到物理内存的映射也包含了对内存的访问权限，方便 Kernel 完成权限检查。

在本次实验中，我们需要关注 OS 如何**开启虚拟地址**以及通过设置页表来实现**地址映射**和**权限控制**。

### 实验目的
* 学习虚拟内存的相关知识，实现物理地址到虚拟地址的切换。
* 了解 RISC-V 架构中 SV39 分页模式，实现虚拟地址到物理地址的映射，并对不同的段进行相应的权限设置。

### 基础知识
* [Kernel 的虚拟内存布局](#虚拟内存布局)
    - 可继续看 satp / VA / PA / PTE / VA->PA 等内容.
* satp: 控制supervisor模式下的地址转换和保护。Supervisor Address Translation and Protection Register
    - Mode + ASID + PPN.(4 + 16 + 44) 其中存储顶级页表的物理页号。
* SV39 虚拟与物理地址.
    - 虚拟地址 9 + 9 + 9 + 12
    - 物理地址 56 = 44 + 12
    - Sv41 模式定义物理地址有 56 位，虚拟地址有 64 位。但是，虚拟地址的 64 位只有低 39 位有效。通过虚拟内存布局图我们可以发现，其 63-39 位为 0 时代表 user space address， 为 1 时 代表 kernel space address。
    - Sv39 支持三级页表结构，VPN[2-0](Virtual Page Number)分别代表每级页表的`虚拟页号`，PPN[2-0](Physical Page Number)分别代表每级页表的`物理页号`。物理地址和虚拟地址的低12位表示页内偏移（page offset）。
* Sv39 页表项
    - 64b * 512(2^9, VA->VPN 9位索引) = 4KB 页表大小.
    - Reserved + PPN + Flag. (10 + 44 + 10)
* 虚拟地址转化为物理地址
    - 看图. 链接中有更多细节. 

#### Kernel 的虚拟内存布局

<h5 id = "虚拟内存布局">虚拟内存布局</h5>

```
start_address           end_address
    0x0                 0x3fffffffff
     │                       │
┌────┘                 ┌─────┘
↓        256G          ↓                                
┌───────────────────────┬──────────┬────────────────┐
│      User Space       │    ...   │  Kernel Space  │
└───────────────────────┴──────────┴────────────────┘
                                    ↑    256G      ↑
                      ┌─────────────┘              │ 
                      │                            │
              0xffffffc000000000          0xffffffffffffffff
                start_address                 end_address
```
通过上图我们可以看到 RV64 将 `0x0000004000000000` 以下的虚拟空间作为 `user space`。将 `0xffffffc000000000` 及以上的虚拟空间作为 `kernel space`。由于我们还未引入用户态程序，目前我们只需要关注 `kernel space`。

具体的虚拟内存布局可以[参考这里](https://elixir.bootlin.com/linux/v5.15/source/Documentation/riscv/vm-layout.rst)。

> 在 `RISC-V Linux Kernel Space` 中有一段区域被称为 `direct mapping area`，为了方便 kernel 可以高效率的访问 RAM，kernel 会预先把所有物理内存都映射至这一块区域 ( PA + OFFSET == VA )， 这种映射也被称为 `linear mapping`。在 RISC-V Linux Kernel 中这一段区域为 `0xffffffe000000000 ~ 0xffffffff00000000`, 共 124 GB 。


#### RISC-V Virtual-Memory System (Sv39)

##### `satp` Register（Supervisor Address Translation and Protection Register）

```c
 63      60 59                  44 43                                0
 ---------------------------------------------------------------------
|   MODE   |         ASID         |                PPN                |
 ---------------------------------------------------------------------
```

* MODE 字段的取值如下图：
```c
                             RV 64
     ----------------------------------------------------------
    |  Value  |  Name  |  Description                          |
    |----------------------------------------------------------|
    |    0    | Bare   | No translation or protection          |
    |  1 - 7  | ---    | Reserved for standard use             |
    |    8    | Sv39   | Page-based 39 bit virtual addressing  | <-- 我们使用的mode
    |    9    | Sv48   | Page-based 48 bit virtual addressing  |
    |    10   | Sv57   | Page-based 57 bit virtual addressing  |
    |    11   | Sv64   | Page-based 64 bit virtual addressing  |
    | 12 - 13 | ---    | Reserved for standard use             |
    | 14 - 15 | ---    | Reserved for standard use             |
     -----------------------------------------------------------
```
* ASID ( Address Space Identifier ) ： 此次实验中直接置 0 即可。
* PPN ( Physical Page Number ) ：顶级页表的物理页号。我们的物理页的大小为 4KB， PA >> 12 == PPN。
* 具体介绍请阅读 [RISC-V Privileged Spec 4.1.10](https://www.five-embeddev.com/riscv-isa-manual/latest/supervisor.html#sec:satp) 。

##### RISC-V Sv39 Virtual Address and Physical Address
```c
     38        30 29        21 20        12 11                           0
     ---------------------------------------------------------------------
    |   VPN[2]   |   VPN[1]   |   VPN[0]   |          page offset         |
     ---------------------------------------------------------------------
                            Sv39 virtual address

```

```c
 55                30 29        21 20        12 11                           0
 -----------------------------------------------------------------------------
|       PPN[2]       |   PPN[1]   |   PPN[0]   |          page offset         |
 -----------------------------------------------------------------------------
                            Sv39 physical address

```
* Sv39 模式定义物理地址有 56 位，虚拟地址有 64 位。但是，虚拟地址的 64 位只有低 39 位有效。通过虚拟内存布局图我们可以发现，其 63-39 位为 0 时代表 user space address， 为 1 时 代表 kernel space address。
* Sv39 支持三级页表结构，VPN[2-0](Virtual Page Number)分别代表每级页表的`虚拟页号`，PPN[2-0](Physical Page Number)分别代表每级页表的`物理页号`。物理地址和虚拟地址的低12位表示页内偏移（page offset）。
* 具体介绍请阅读 [RISC-V Privileged Spec 4.4.1](https://www.five-embeddev.com/riscv-isa-manual/latest/supervisor.html#sec:sv39) 。


##### RISC-V Sv39 Page Table Entry
```c
 63      54 53        28 27        19 18        10 9   8 7 6 5 4 3 2 1 0
 -----------------------------------------------------------------------
| Reserved |   PPN[2]   |   PPN[1]   |   PPN[0]   | RSW |D|A|G|U|X|W|R|V|
 -----------------------------------------------------------------------
                                                     |   | | | | | | | |
                                                     |   | | | | | | | `---- V - Valid
                                                     |   | | | | | | `------ R - Readable
                                                     |   | | | | | `-------- W - Writable
                                                     |   | | | | `---------- X - Executable
                                                     |   | | | `------------ U - User
                                                     |   | | `-------------- G - Global
                                                     |   | `---------------- A - Accessed
                                                     |   `------------------ D - Dirty (0 in page directory)
                                                     `---------------------- Reserved for supervisor software
```

* 0 ～ 9 bit: protection bits
    * V : 有效位，当 V = 0, 访问该 PTE 会产生 Pagefault。
    * R : R = 1 该页可读。
    * W : W = 1 该页可写。
    * X : X = 1 该页可执行。
    * U , G , A , D , RSW 本次实验中设置为 0 即可。
* 具体介绍请阅读 [RISC-V Privileged Spec 4.4.1](https://www.five-embeddev.com/riscv-isa-manual/latest/supervisor.html#sec:sv39)


##### RISC-V Address Translation
虚拟地址转化为物理地址流程图如下，具体描述见 [RISC-V Privileged Spec 4.3.2](https://www.five-embeddev.com/riscv-isa-manual/latest/supervisor.html#sv32algorithm) :
```text
                                Virtual Address                                     Physical Address

                          9             9            9              12          55        12 11       0
   ┌────────────────┬────────────┬────────────┬─────────────┬────────────────┐ ┌────────────┬──────────┐
   │                │   VPN[2]   │   VPN[1]   │   VPN[0]    │     OFFSET     │ │     PPN    │  OFFSET  │
   └────────────────┴────┬───────┴─────┬──────┴──────┬──────┴───────┬────────┘ └────────────┴──────────┘
                         │             │             │              │                 ▲          ▲
                         │             │             │              │                 │          │
                         │             │             │              │                 │          │
┌────────────────────────┘             │             │              │                 │          │
│                                      │             │              │                 │          │
│                                      │             │              └─────────────────┼──────────┘
│    ┌─────────────────┐               │             │                                │
│511 │                 │  ┌────────────┘             │                                │
│    │                 │  │                          │                                │
│    │                 │  │     ┌─────────────────┐  │                                │
│    │                 │  │ 511 │                 │  │                                │
│    │                 │  │     │                 │  │                                │
│    │                 │  │     │                 │  │     ┌─────────────────┐        │
│    │   44       10   │  │     │                 │  │ 511 │                 │        │
│    ├────────┬────────┤  │     │                 │  │     │                 │        │
└───►│   PPN  │  flags │  │     │                 │  │     │                 │        │
     ├────┬───┴────────┤  │     │   44       10   │  │     │                 │        │
     │    │            │  │     ├────────┬────────┤  │     │                 │        │
     │    │            │  └────►│   PPN  │  flags │  │     │                 │        │
     │    │            │        ├────┬───┴────────┤  │     │   44       10   │        │
     │    │            │        │    │            │  │     ├────────┬────────┤        │
   1 │    │            │        │    │            │  └────►│   PPN  │  flags │        │
     │    │            │        │    │            │        ├────┬───┴────────┤        │
   0 │    │            │        │    │            │        │    │            │        │
     └────┼────────────┘      1 │    │            │        │    │            │        │
     ▲    │                     │    │            │        │    └────────────┼────────┘
     │    │                   0 │    │            │        │                 │
     │    └────────────────────►└────┼────────────┘      1 │                 │
     │                               │                     │                 │
 ┌───┴────┐                          │                   0 │                 │
 │  satp  │                          └────────────────────►└─────────────────┘
 └────────┘
```

### 代码目录

```
.
├── Makefile
├── arch
│   └── riscv
│       ├── Makefile
│       ├── include
│       │   ├── defs.h          *M
│       │   ├── mm.h
│       │   ├── proc.h
│       │   ├── sbi.h
│       │   └── types.h
│       └── kernel
│           ├── Makefile
│           ├── clock.c
│           ├── entry.S
│           ├── head.S          *M
│           ├── mm.c            *M
│           ├── proc.c
│           ├── sbi.c
│           ├── trap.c
│           ├── vm.c            *A
│           └── vmlinux.lds.S   *A
├── include
│   ├── printk.h
│   ├── rand.h
│   ├── stddef.h
│   ├── string.h
│   └── types.h
├── init
│   ├── Makefile
│   ├── main.c
│   └── test.c
└── lib
    ├── Makefile
    ├── printk.c
    ├── rand.c
    └── string.c
```

### 代码框架

* 修改defs.h, 加入vm相关. 
* 加入vmlinux.lds.S, 使 vmlinux 采用虚拟地址
* vm相关. 用于虚拟内存的实现
* 修改head.S, 使其初始化虚拟地址
* 修改mm.c 使mm_init符合虚拟地址

### 运行流程

1. Makefile 
    - 定义如何编译 运行
1. vmlinux.lds.S -> vmlinux.lds
    - 连接脚本指定 ENTRY = _start / BASE_ADDR, 以及各内存布局
    - VMA>ramv LAM>ram 指定虚拟地址和对应的物理地址     *A
1.  _start: head.S 
    - 定义程序栈
    - setup_vm              *A
    - relocate              *A
    - mm_init               *M
    - setup_vm_final        *A
    - task_init
    - 开启trap处理
        - 第一次时钟中断
        - stvec _traps
        - sie[STIE]
        - sstatus
    - jal ra, start_kernel. 
1. setup_vm: vm.c
    - 0x80000000 开始的 1GB 区域进行两次映射
        - 等值映射 PA == VA
        - `direct mapping area` `PA + PV2VA_OFFSET == VA`
    - 映射存入early_pgtbl
1. relocate: vm.c
    - satp 设置为 early_pgtbl
    - 设置一级页表
    - sfence.vma 开启 MMU
1. mm_init: mm.c
    - 提供 kalloc() 用于数据页的分配
    - 调整为虚拟地址. 
1. setup_vm_final
    - 三级页表映射
    - satp 设置为 swapper_pg_dir , 刷新页表
1. task_init: proc.c
    - 根据 task_struct 数据结构设置 idle 和 task[] 的初始化.
    - ra 设置为 __dummy
    - sp 设置为物理页的高地址
1. start_kernel(): main.c
    - test()
1. test(): test.c 
    - while(1)

1. 当出现第二次时钟中断时, opensbi 通过 stvec 跳转到 _traps
1. _traps: entry.S
    - 上下文切换(开始/结束)
        - save test() register
    - 跳转至 trap_handler()
1. trap_handler(): trap.c
    - 查看 scause, sepc 识别时钟中断
        - clock_set_next_event()
        - do_timer()
1. clock_set_next_event(): clock.c
    - get_cycles(): `rdtime`
    - clock_set_next_event(): `sbi_ecall`
1. do_timer(): proc.c 
    - idle 跳转 schedule() 调度
    - 非idle 运行时间-1
        - `>0` 返回 
        - `=0` 调度
1. schedule(): proc.c 
    - 根据 SJF / PRIORITY 选择 next 线程(task_struct)
    - switch_to(next)
1. switch_to(): proc.c 
    - 如果 `current task != next task` : __switch_to(prev, next)
1. __switch_to(): entry.S 
    - 上下文切换. 
        - 在 __switch_to 时, 才是真正意义的context_switch, 也就是存储旧值, 加载新值. 
        - save `prev` register.
            - 存caller saved register.
            - 这里 idle 的 ra如下. 
                > __switch__to -> switch_to() -> schedule() -> dotimer() -> trap_handler() -> _traps -> test.c:while(1)
            - `idle` 状态从此结束 不再参与调度. 
        - load `next` register.
        - 在第二次时钟中断时, `ret`命令 pc 返回 ra, 又因为 task[] ra 被设置成 __dummy, 跳转至 __dummy. 
1. __dummy: entry.S
    - 设置 sepc 为 dummy
    - `sret` 返回 dummy().      // 真是非常神奇啊, context_switch 之后就变成完全不同的线程了 
1. dummy(): proc.c 
    - while(1) 
        - getpid() print
1. 直到下一个时钟中断. 下次时间中断进入 _traps 存储这次 task 的 sp / ra, task 进入休眠状态, 直到下次schedule() -> switch_to(next) 被调度时通过 task_struct -> ra 重新运行, 回到dummy(). 
> 重新运行时 __switch_to ret ra -> __switch_to end -> switch_to end -> schedule() end -> do_timer() end -> trap_handler() end -> _traps -> dummy()

### 实验精简

- setup_vm
    * 将 0x80000000 开始的 1GB 区域进行两次映射，其中一次是等值映射 ( PA == VA ) ，另一次是将其映射至高地址 ( PA + PV2VA_OFFSET == VA )。如下图所示：
```
   Physical Address
    -------------------------------------------
                        | OpenSBI | Kernel |
    -------------------------------------------
                        ^
                    0x80000000
                        ├───────────────────────────────────────────────────┐
                        |                                                   |
   Virtual Address      ↓                                                   ↓
    -----------------------------------------------------------------------------------------------
                        | OpenSBI | Kernel |                                | OpenSBI | Kernel |
    -----------------------------------------------------------------------------------------------
                        ^                                                   ^
                    0x80000000                                       0xffffffe000000000
```
- `relocate` 函数
    * 完成对 `satp` 的设置，以及跳转到对应的虚拟地址。

- setup_vm_final
    * 由于 setup_vm_final 中需要申请页面的接口，应该在其之前完成内存管理初始化，mm.c 中初始化的函数接收的起始结束地址需调整为虚拟地址。
    * 对 所有物理内存 (128M) 进行映射，并设置正确的权限。
        * 不再需要进行等值映射
        * 不再需要将 OpenSBI 的映射至高地址，因为 OpenSBI 运行在 M 态， 直接使用的物理地址。
        * 采用三级页表映射。
        * 在 head.S 中 适当的位置调用 setup_vm_final 。
```
   Physical Address
        PHY_START                           PHY_END
            ↓                                  ↓
    --------------------------------------------------------
            | OpenSBI | Kernel |               |
    --------------------------------------------------------
            ^                                  ^
       0x80000000                              └───────────────────────────────────────────────────┐
            └───────────────────────────────────────────────────┐                                  |
                                                                |                                  |
                                                             VM_START                              |
   Virtual Address                                              ↓                                  ↓
    ----------------------------------------------------------------------------------------------------
                                                                | OpenSBI | Kernel |               |
    -----------------------------------------------------------------------------------------------------
                                                                ^
                                                        0xffffffe000000000
```

### 思考

为什么要做临时虚拟地址空间映射？
    一旦开启MMU，PC的下一条指令地址会经过MMU转化，未开启MMU之前地址的翻译是不需要经过MMU转化直接访问。对应开启MMU之后，应该要使用虚拟地址，才能访问到正确的指令内存。
    前面描述了虚拟地址转换为物理地址是通过MMU自动转换，但是需要给MMU创建好页表，这样MMU才能自动查询到物理地址。页表也是对应的物理内存，也是需要分配的，在正常系统运行时，页表的分配可以通过系统的内存管理接口获取到，但是在系统刚运行时，内存管理并没有初始化好，无法调用接口分配到页表，这样即使使能MMU但是找不到页表，也无法翻译出物理地址。
https://www.laumy.tech/1282.html

- 思考题
    1. 关于 PIE
    > 在开始实验开启虚拟地址之前，我们还需要对 Makefile 进行一些修改来防止后面运行/调试出现问题。如果大家观察过之前的 lab 编译后再经过 objdump 反汇编的结果，你可能会发现 head.S 的第一句设置栈的汇编代码被翻译成了“奇怪”的东西：
    > 
    > ```asm
    >     la sp, boot_stack_top
    >     80200000:	00003117          	auipc	sp,0x3
    >     80200004:	02013103          	ld	sp,32(sp) # 80203020 <_GLOBAL_OFFSET_TABLE_+0x18>
    > ```
    > 
    > 你可能好奇过这里为什么要吧 `boot_stack_top` 的地址 ld 出来，而且是从哪里 ld 出来的。答案 objdump 的注释已经给出了，是从一个叫 `_GLOBAL_OFFSET_TABLE_` 的地方取出来的，这也就是你可能听说过的 GOT 表（全局偏移表），有了它就可以实现 PIE（位置无关执行）了，在每次取地址的时候，只要通过 GOT 表来取，就可以使得即使代码被放在不同地址执行，也可以取到正确的地址。
    > 
    > 当然，在我们实现 kernel 的时候，所有的地址都是不变的，代码也始终会被 load 到 0x80200000 的地方，因此这个 PIE 对我们并没有作用。相反，在本次试验中它还会有副作用，因为 GOT 表里的地址都是最终的虚拟地址，所以在 kernel 启用虚拟地址之前，一切从 GOT 表取出的地址都是错的，这种情况下需要手动给 `la` 得到的地址减去 `PA2VA_OFFSET` 才能得到正确的物理地址。
    > 
    > 为了避免这种情况，我们可以直接关掉 PIE，只需要在 Makefile 的 `CF` 中加一个 `-fno-pie` 就可以强制不编译出 PIE 的代码。在这种情况下第一句的汇编代码会被编译成：
    > 
    > ```asm
    >     la sp, boot_stack_top
    >     80200000:	00005117          	auipc	sp,0x5
    >     80200004:	00010113          	mv	sp,sp
    > ```
    > 
    > 它直接使用 `auipc` 根据目标基于当前 pc 的偏移就能计算出正确的地址，而且这种代码不管是否启用了虚拟地址，都是有效的。因此在实验开始前，请同学们在 Makefile 中加上 `-fno-pie`。
    > 
    > ps: 这一段关于PIE内容是从最新的lab中找到的, 这也解答了为什么工程中`la sp, boot_stack_top`需要如下. 为了保持可运行, 原工程中未作修改.
    > ```asm
    >     # set sp
    >     la sp, boot_stack_top
    >     li t0, 0xffffffdf80000000
    >     sub sp, sp, t0
    >     add s0, sp, x0
    > ```

    2. 为什么我们在 setup_vm 中需要做等值映射?
        * 因为这能让我们在虚拟地址上和虚拟地址对应的物理地址上都有kernel代码的映射，为后面setup_vm_final能通过虚拟地址访问物理地址做准备，
        * 而且如果没有等值映射，pc还是跑在低地址，PA!=VA就会导致异常，不能平滑的切换虚拟地址和物理地址。
        * 切换到虚拟内存模式后，CPU 需要使用页表进行地址转换。为了避免立即丢失对关键代码和数据的访问，等值映射提供了一个平滑过渡：
            - 等值映射确保切换到虚拟内存模式时，现有的代码不需要修改地址即可继续运行。
            - 一旦虚拟内存模式稳定运行，其他更复杂的地址映射（如 direct mapping 或高内存地址映射）可以逐步生效。
        * 在系统引导阶段，CPU 和外设通常使用物理地址进行访问，而部分代码在此阶段运行于物理地址上下文中。因此，在切换到虚拟内存模式之前，需要保证某些关键的代码和数据能够通过虚拟地址直接访问到原有的物理地址。
            - 等值映射使虚拟地址与物理地址相同（即 PA == VA），这确保了切换到虚拟内存模式后，仍然可以正确执行这些关键代码。

> 3. 在 Linux 中，是不需要做等值映射的。请探索一下不在 setup_vm 中做等值映射的方法。

> 我们可以看`arch/riscv/kernel/head.S`中的`clear_bss_done`段（301行），这是_start_kernel中的一段，其中设置了sp之后，其`call setup_vm`设置初始的页表之后，将early_pg_dir作为参数传入了relocate之中。

> <p>
    > <img src='os_lab4/18.png' /><br>
> </p>

> 之后就是relocate的部分。其中第一部分就是加上虚拟地址和物理地址的偏移量。之后就是设置stvec为1f和计算early_pg_dir的虚拟地址。但其先不写入satp，而是先写入trampoline_pg_dir，从而开启mmu。

> 开启mmu之后直接触发异常进入`1f`，其中设置了stvec是`.Lsecondary_park`，这里就是个死循环，用于debug。继续设置satp为传入的early_pg_table，如果没有触发异常，说明页表设置成功，这样程序就在虚拟地址上跑了。最后返回_start_kernel。

> <p>
    > <img src='os_lab4/19.png' /><br>
> </p>



## Lab 5: RV64 用户态程序

### 实验前言
在 Lab3 中，我们启用了**虚拟内存**，这为进程间地址空间相互隔离打下了基础。然而，我们当时只创建了内核线程，它们共用了地址空间（共用一个内核页表 `swapper_pg_dir`）。在本次实验中，我们将引入**用户态进程**：

- 当启动用户态应用程序时，内核将为该应用程序创建一个进程，并提供了专用虚拟地址空间等资源
    - 每个应用程序的虚拟地址空间是私有的，一个应用程序无法更改属于另一个应用程序的数据
    - 每个应用程序都是独立运行的，如果一个应用程序崩溃，其他应用程序和操作系统将不会受到影响
- 用户态应用程序可访问的虚拟地址空间是受限的
    - 在用户态下，应用程序无法访问内核的虚拟地址，防止其修改关键操作系统数据
    - 当用户态程序需要访问关键资源的时候，可以通过**系统调用**来完成用户态程序与操作系统之间的互动

### 实验目的
* 创建用户态进程，并设置 `sstatus` 来完成内核态转换至用户态。
* 正确设置用户进程的**用户态栈**和**内核态栈**， 并在异常处理时正确切换。
* 补充异常处理逻辑，完成指定的系统调用（ SYS_WRITE, SYS_GETPID ）功能。

### 基础知识

* **用户模式**（U-Mode）和**内核模式**（S-Mode）
* **系统调用** 用户态应用程序请求内核服务的一种方式。
    - 在 RISC-V 中，我们使用 `ecall` 指令进行系统调用，处理器会提升特权模式，跳转到异常处理函数，处理这条系统调用。
* `sstatus[SUM]` 与 `PTE[U]`
    - `PTE[U]` 置 0 时，该页表项对应的内存页为内核页, 反之为用户页
    - 让 S 特权级下的程序能够访问用户页，需 `sstatus[SUM]` 置 1
* 用户态栈与内核态栈
    - 用户态程序进行系统调用陷入内核处理时, 使用内核态程序的栈空间.
    - 需为用户态程序和内核态程序分别分配栈空间，并在异常处理的过程中对栈进行切换。
* ELF（Executable and Linkable Format）
    - ELF 文件可以包含将程序正确加载入内存的元数据（metadata）
    - ELF 文件在运行时可以由加载器（loader）将动态链接在程序上的动态链接库（shared library）正确地从硬盘或内存中加载
    - ELF 文件包含的重定位信息可以让该程序继续和其他可重定位文件和库再次链接，构成新的可执行文件

### 代码目录

```
.
├── Makefile
├── arch
│   └── riscv
│       ├── Makefile    
│       ├── include     
│       │   ├── defs.h  
│       │   ├── mm.h    
│       │   ├── proc.h  
│       │   ├── sbi.h   
│       │   ├── stdint.h    
│       │   ├── syscall.h   
│       │   └── types.h     
│       └── kernel  
│           ├── Makefile    
│           ├── clock.c     
│           ├── entry.S     
│           ├── head.S  
│           ├── mm.c    
│           ├── proc.c  
│           ├── sbi.c   
│           ├── syscall.c   
│           ├── trap.c  
│           ├── vm.c    
│           └── vmlinux.lds.S   
├── include     
│   ├── elf.h   
│   ├── printk.h    
│   ├── rand.h  
│   ├── stddef.h    
│   ├── string.h    
│   └── types.h     
├── init    
│   ├── Makefile    
│   ├── main.c  
│   └── test.c  
├── lib     
│   ├── Makefile    
│   ├── printk.c    
│   ├── rand.c  
│   └── string.c    
└── user    
    ├── Makefile    
    ├── getpid.c    
    ├── link.lds    
    ├── printf.c    
    ├── start.S     
    ├── stddef.h    
    ├── stdio.h     
    ├── syscall.h   
    ├── uapp        
    └── uapp.S
```

### 代码框架
* 修改 `vmlinux.lds.S`，将用户态程序 `uapp` 加载至 `.data` 段
* 修改 `defs.h`, 加入user space相关
* 修改 mm.c/.h , 实现 buddy system
* 加入 stdint.h, 辅助buddy system
* 加入 elf.h
* 加入user文件夹. 用户态程序
    ├── Makefile
    ├── getpid.c        # 用户态程序
    ├── link.lds
    ├── printf.c        # 类似 printk
    ├── start.S         # 用户态程序入口
    ├── stddef.h
    ├── stdio.h
    ├── syscall.h
    └── uapp.S          # 提取二进制内容
* 修改 proc.c/.h, 使其适应内核/用户态区分.
* 修改 entry.S, 加入内核/用户态栈区分等等. 
* 修改 trap.c, 加入内核/用户态栈区分等等. 
* 加入 syscall.c/.h, 使其支持用户态 syscall

### 运行流程

1. Makefile 
    - 定义如何编译 运行
1. vmlinux.lds.S -> vmlinux.lds
    - 连接脚本指定 ENTRY = _start / BASE_ADDR, 以及各内存布局
    - VMA>ramv LAM>ram 指定虚拟地址和对应的物理地址     *A
1.  _start: head.S 
    - 定义程序栈
    - setup_vm
    - relocate
    - mm_init               *M
    - setup_vm_final
    - task_init             *M
    - 开启trap处理
        - 第一次时钟中断
        - stvec _traps
        - sie[STIE]
        - sstatus
    - jal ra, start_kernel. 
1. setup_vm: vm.c
    - 0x80000000 开始的 1GB 区域进行两次映射
        - 等值映射 PA == VA
        - `direct mapping area` `PA + PV2VA_OFFSET == VA`
    - 映射存入early_pgtbl
1. relocate: vm.c
    - satp 设置为 early_pgtbl
    - 设置一级页表
    - sfence.vma 开启 MMU
1. mm_init: mm.c
    - 修改为了 buddy system. 
    - 提供 alloc_page()
1. setup_vm_final
    - 三级页表映射
    - satp 设置为 swapper_pg_dir , 刷新页表
1. task_init: proc.c
    - 根据 task_struct 数据结构设置 idle 和 task[] 的初始化.
        * 加入sepc, sstatus, sscratch(存储用户态sp)与用户态页表 pgd.
    - ra 设置为 __dummy
    - sp 设置为物理页的高地址
    * 对每个进程:
        - 内核页表 -> 用户页表
        - uapp copy
        - 用户态栈
1. start_kernel(): main.c
    - schedule() 提前调度
        - switch_to()
        - __swtich_to()
            - 加入存储 sepc, sstatus, scratch, satp   
            - ret ra = __dummy
        - __dummy
            - sret sepc = UAPP_START
        - uapp.S
>     - test()
> 1. test(): test.c 
>     - while(1)
**lab5后不再适用**

1. 当出现第二次时钟中断或者 syscall 时, opensbi 通过 stvec 跳转到 _traps
1. _traps: entry.S
    - 上下文切换(开始/结束)
        - 加入用户态 / 内核态栈切换
    - 跳转至 trap_handler()
1. trap_handler(): trap.c
    - scause, sepc 识别时钟中断
        - clock_set_next_event()
        - do_timer()
    - scause, sepc 识别syscall(Environment Call from U-mode)
        - syscall(pt_regs)
1. syscall(): sycall.c
    - 根据 pt_regs 对不同情况实现 getpid() / sys_write()
    - sepc + 4
        - ra -> trap_handler() -> _traps sret sepc -> Uapp
1. clock_set_next_event(): clock.c
    - get_cycles(): `rdtime`
    - clock_set_next_event(): `sbi_ecall`
1. do_timer(): proc.c 
    - idle 跳转 schedule() 调度
    - 非idle 运行时间-1
        - `>0` 返回 
        - `=0` 调度
1. schedule(): proc.c 
    - 根据 SJF / PRIORITY 选择 next 线程(task_struct)
    - switch_to(next)
1. switch_to(): proc.c 
    - 如果 `current task != next task` : __switch_to(prev, next)
1. __switch_to(): entry.S 
    - 上下文切换. 
        - 在 __switch_to 时, 才是真正意义的context_switch, 也就是存储旧值, 加载新值. 
        - save `prev` register.
            - 存caller saved register.
            - `idle` 状态从此结束 不再参与调度. 
        - load `next` register.
        - 使用当前 pgd 修改 satp
        - 在第二次时钟中断时, `ret`命令 pc 返回 ra, 又因为 task[] ra 被设置成 __dummy, 跳转至 __dummy.
1. __dummy: entry.S
    - 加入用户态 / 内核态栈切换
    - `sret` 返回 uapp
>    - 设置 sepc 为 dummy
>    - `sret` 返回 dummy().
**设置 sepc 为 dummy 在lab5之后的实验被取消了, 所以 sret 返回的是 task_struct 里的 sepc , 也就是 USER_START**
> 1. dummy(): proc.c 
>     - while(1) 
>         - getpid() print

1. 直到下一个时钟中断. 下次时间中断进入 _traps 存储这次 task 的 sp / ra, task 进入休眠状态, 直到下次schedule() -> switch_to(next) 被调度时通过 task_struct -> ra 重新运行, 回到uapp. 
> 重新运行时 __switch_to ret ra -> __switch_to end -> switch_to end -> schedule() end -> do_timer() end -> trap_handler() end -> _traps -> uapp

!!! tip "elf"
    由于 elf 与运行流程并无太大关联, 因此不在流程中列出. 

### 实验精简

!!! tip  
    * 在根目录下 `make` 会生成 `user/uapp.o`（这个会被链接到 vmlinux 中）`user/uapp.elf` `user/uapp.bin`，以及我们最终测试使用的 ELF 可执行文件 `user/uapp`。
            
    * 通过 `objdump` 我们可以看到 uapp 使用 ecall 来调用 SYSCALL（在 U-Mode 下使用 ecall 会触发 `environment-call-from-U-mode` 异常），从而将控制权交给处在 S-Mode 的 OS，由内核来处理相关异常
    * 首先将用户态程序 strip 成纯二进制文件运行。这种情况下，用户程序运行的第一条指令位于二进制文件的开始位置, 也就是说 `uapp_start` 处的指令就是我们要执行的第一条指令。
    * 我们将运行纯二进制文件作为第一步，在确认用户态的纯二进制文件能够运行后，我们再将存储到内存中的用户程序文件换为 ELF 来进行执行。

- 创建用户态进程 
    * 加入sepc , sstatus , sscratch(存储内核态sp)与用户态页表pgd.
```c
struct thread_struct {
    uint64_t ra;
    uint64_t sp;                     
    uint64_t s[12];
    uint64_t sepc, sstatus, sscratch; 
};

struct task_struct {
    uint64_t state;
    uint64_t counter;
    uint64_t priority;
    uint64_t pid;

    struct thread_struct thread;
    uint64_t *pgd;  // 用户态页表
};
```
- 
    * 修改 `task_init()`
        * 对于每个进程，初始化我们刚刚在 `thread_struct` 中添加的三个变量，具体而言：
            * 将 `sepc` 设置为 `USER_START`
            * 配置 `sstatus` 中的 `SPP`（使得 sret 返回至 U-Mode）、`SUM`（S-Mode 可以访问 User 页面）
            * 将 `sscratch` 设置为 U-Mode 的 sp，其值为 `USER_END` （将用户态栈放置在 user space 的最后一个页面）
        * 对于每个进程，创建属于它自己的页表：
            * 为了避免 U-Mode 和 S-Mode 切换的时候切换页表，我们将内核页表 `swapper_pg_dir` 复制到每个进程的页表中
            * 对于每个进程，分配一块新的内存地址，将 `uapp` 二进制文件内容**拷贝**过去，之后再将其所在的页面映射到对应进程的页表中
        * 设置用户态栈，对每个用户态进程，其拥有两个栈：
            - 用户态栈：我们可以申请一个空的页面来作为用户态栈，并映射到进程的页表中
            - 内核态栈；在 lab3 中已经设置好了，就是 `thread.sp`

```
                PHY_START                                                                PHY_END
                     new allocated memory            allocated space end
                   │         │                                │                                                 │
                   ▼         ▼                                ▼                                                 ▼
       ┌───────────┬─────────┬────────────────────────────────┬─────────────────────────────────────────────────┐
 PA    │           │         │ uapp (copied from _sramdisk)   │                                                 │
       └───────────┴─────────┴────────────────────────────────┴─────────────────────────────────────────────────┘
                             ▲                                ▲
       ┌─────────────────────┘                                │
       │            (map)                                     │
       │                        ┌─────────────────────────────┘
       │                        │
       │                        │
       ├────────────────────────┼───────────────────────────────────────────────────────────────────┬────────────┐
 VA    │           UAPP         │                                                                   │u mode stack│
       └────────────────────────┴───────────────────────────────────────────────────────────────────┴────────────┘
       ▲                                                                                                         ▲
       │                                                                                                         │

   USER_START                                                                                                USER_END

```

- 修改 `__switch_to`
    * 在前面新增了 sepc、sstatus、sscratch 之后，需要将这些变量在切换进程时保存在栈上，因此需要更新 `__switch_to` 中的逻辑，同时需要增加切换页表的逻辑。在切换了页表之后，需要通过 `sfence.vma` 来刷新 TLB 和 ICache。
    * 切换页表的逻辑分为两步，一个是写 `satp`，一个是刷新 TLB，我们用 `task_struct` 上的 `pgd` 保存了下一个用户进程的页表的虚拟地址，需要通过一些变换得到 PPN，加上 MODE 写入 `satp`。

- 更新中断处理逻辑
    * 与 ARM 架构不同的是，RISC-V 中只有一个栈指针寄存器 `sp`，因此需要我们来完成用户栈与内核栈的切换。
    * 由于我们的用户态进程运行在 U-Mode 下，使用的运行栈也是用户栈，因此当触发异常时，我们首先要对栈进行切换（从用户栈切换到内核栈）。同理，当我们完成了异常处理，从 S-Mode 返回至 U-Mode 时，也需要进行栈切换（从内核栈切换到用户栈）。

- 修改 `__dummy`
    * 在我们初始化线程时，`thread_struct.sp` 保存了内核态栈 sp，`thread_struct.sscratch` 保存了用户态栈 sp，因此在 `__dummy` 进入用户态模式的时候，我们需要切换这两个栈，只需要交换对应的寄存器的值即可。

- 修改 `_traps`
    * 同理，在 `_traps` 的首尾我们都需要做类似的操作，进入 trap 的时候需要切换到内核栈，处理完成后需要再切换回来。
    * 注意如果是内核线程（没有用户栈）触发了异常，则不需要进行切换。（内核线程的 `sp` 永远指向的内核栈，且 `sscratch` 为 0）

- 修改 `trap_handler`
    * `uapp` 使用 `ecall` 会产生 Environment Call from U-mode，而且处理系统调用的时候还需要寄存器的值，因此我们需要在 `trap_handler()` 里面进行捕获。修改 `trap_handler()` 如下：

```c
void trap_handler(uint64_t scause, uint64_t sepc, struct pt_regs *regs) {
    ...
}
```

在 `_traps` 中我们将寄存器的内容**连续**的保存在内核栈上，因此我们可以将这一段看做一个叫做 `pt_regs` 的结构体。我们可以从这个结构体中取到相应的寄存器的值（比如 `syscall` 中我们需要从 a0 ~ a7 寄存器中取到参数）。这个结构体中的值也可以按需添加，同时需要在 `_traps` 中存入对应的寄存器值以供使用，示例如下图：

```
    High Addr ───►  ┌─────────────┐
                    │   sstatus   │
                    │             │
                    │     sepc    │
                    │             │
                    │     x31     │
                    │             │
                    │      .      │
                    │      .      │
                    │      .      │
                    │             │
                    │     x1      │
                    │             │
                    │     x0      │
 sp (pt_regs)  ──►  ├─────────────┤
                    │             │
                    │             │
                    │             │
                    │             │
                    │             │
                    │             │
                    │             │
                    │             │
                    │             │
    Low  Addr ───►  └─────────────┘
```

- 添加系统调用
    * 本次实验要求的系统调用函数原型以及具体功能如下：
        * 64 号系统调用 `#!c sys_write(unsigned int fd, const char* buf, size_t count)` 该调用将用户态传递的字符串打印到屏幕上，此处 `fd` 为标准输出即 `1`，`buf` 为用户需要打印的起始地址，`count` 为字符串长度，返回打印的字符数；
        * 172 号系统调用 `sys_getpid()` 该调用从 `current` 中获取当前的 pid 放入 a0 中返回，无参数
    * 增加 `syscall.c`, `syscall.h` 文件，并在其中实现 `getpid()` 以及 `write()` 逻辑
    * 系统调用的返回参数放置在 `a0` 中（不可以直接修改寄存器，应该修改 regs 中保存的内容）
    * 针对系统调用这一类异常，我们需要手动完成 `sepc + 4`（ sepc 记录的是触发异常的指令地址， 由于系统调用这类异常处理完成之后， 我们应该继续执行后续的指令，因此需要我们手动修改 sepc 的地址，使得 sret 之后 程序继续执行）。

- 调整时钟中断
    * 修改 head.S 以及 start_kernel：
        * 在之前的 lab 中，在 OS boot 之后，我们需要等待一个时间片，才会进行调度，我们现在更改为 OS boot 完成之后立即调度 uapp 运行
            * 即在 `start_kernel()` 中，`test()` 之前调用 `schedule()`
        * 将 `head.S` 中设置 sstatus.SIE 的逻辑注释掉，确保 schedule 过程不受中断影响

- 添加 ELF 支持
    * ELF Header
        * ELF 文件中包含了将程序加载到内存所需的信息。当我们通过 readelf 来查看一个 ELF 可执行文件的时候，我们可以读到被包含在 ELF Header 中的信息：
        * 其中包含了两种将程序分块的粒度，Segment 和 Section，我们以 segment 为粒度将程序加载进内存中。可以看到，给出的样例程序包含了三个 segment，这里我们只关注 Type 为 LOAD 的 segment，LOAD 表示它们需要在开始运行前被加载进内存中，这是我们在初始化进程的时候需要执行的工作。
        * 而 section 代表了更细分的语义，比如 `.text` 一般包含了程序的指令，`.rodata` 是只读的全局变量等，大家可以自行 Google 来学习更多相关内容。

- ELF 文件解析
    - 原文

### 思考

1. 我们在实验中使用的用户态线程和内核态线程的对应关系是怎样的？（一对一，一对多，多对一还是多 对多）

是多对一的。 

2. 为什么 Phdr 中，p_filesz 和 p_memsz 是不一样大的？

p_filesz 可加载段会多包含一个 .bss 段，这些是未初始化的数据，所以没有必要放在磁盘上。因此它只会在 ELF 文件加载到内存中时才占用空间；所以p_memsz会大于等于p_filesz。

3. 为什么多个进程的栈虚拟地址可以是相同的？用户有没有常规的方法知道自己栈所在的物理地址？ 

因为每个进程的页表都不同，映射到的物理地址也不同的即使虚拟地址相同，也不会相互干扰。照理来说是没有常规方法的，用户既不知道映射规则，也不知道页表信息。


### 思考
## Lab 6: RV64 缺页异常处理以及 fork 机制

### 实验目的
* 通过 **vm_area_struct** 数据结构实现对 task **多区域**虚拟内存的管理。
* 在 **Lab5** 实现用户态程序的基础上，添加缺页异常处理 **Page Fault Handler**。
* 为 task 加入 **fork** 机制，能够支持通过 **fork** 创建新的用户态 task 。

### 基础知识

* `vm_area_struct` 是Linux虚拟内存管理的基本单元
    * `vm_start`：（第1列）该段虚拟内存区域的开始地址
    * `vm_end`：（第2列）该段虚拟内存区域的结束地址
    * `vm_flags`：（第3列）该段虚拟内存区域的一组权限 (rwx) 标志
    * `vm_pgoff`：（第4列）虚拟内存映射区域在文件内的偏移量
    * `vm_file`：（第5/6/7列）分别表示：映射文件所属设备号/以及指向关联文件结构的指针/以及文件名
    * `vm_ops`：该 `vm_area` 中的一组工作函数，其中是一系列函数指针，可以根据需要进行定制
    * `vm_next/vm_prev`：同一进程的所有虚拟内存区域由**链表结构**链接起来，这是分别指向前后两个 `vm_area_struct` 结构体的指针

!!! tip 
    * 除了跟文件建立联系以外，VMA 还可能是一块匿名（anonymous）的区域。例如被标成 `[stack]` 的这一块区域，并没有对应的文件。
    * 原本的 Linux 使用链表对一个进程内的 VMA 进行管理。如今程序体量可能非常巨大，所以现在的 Linux 已经用虚拟地址为索引来建立红黑树了。
    * 内存中的内容若由磁盘中的文件映射, 当内存的 VMA 产生缺页异常，说明文件对应的页不在操作系统的 buffer pool 中，或是由于 buffer pool 调度策略被换出到磁盘上了。
    * 这时操作系统会用驱动读取硬盘上的内容，放入 buffer pool，然后修改当前进程的页表来让其能够用原来的地址访问文件内容

- 若程序访问未由内存管理单元（MMU）映射到虚拟内存的页面，或访问权限不足，则会由计算机硬件引发的缺页异常**page fault**。
    * 处理缺页异常是操作系统内核的一部分，当处理正常缺页异常时，操作系统将尝试建立新的映射关系到虚拟内存, 使其可访问.
    * 如果在非法访问内存的情况下，即发现触发 page fault 的VA（Bad Address）不在当前进程的 `vm_area_struct` 链表中定义的允许访问的 VA 范围内，或访问位置的权限不满足时，缺页异常处理将终止该程序的继续运行。

- Demand paging: 只在需要时，才将页面放入内存中。
    * 好处: 仅加载执行进程所需的页面，从而节省内存空间。
    * 在 Lab4 的代码中，我们在 `task_init` 的时候创建了用户栈，`load_program` 的时候拷贝了 load segment，并通过 `create_mapping` 在页表中创建了映射。
    * 本次实验中使用 demand paging 的方式，也就是在初始化 task 的时候不进行任何的映射（除了内核栈以及页表以外也不需要开辟其他空间），而是在发生缺页异常的时候检测到是记录在 vma 中的合法地址后，再分配页面并进行映射。

- 可通过解析 `scause` 寄存器的值，识别如下三种不同的 page fault
- 处理 page fault 的方式
    - 处理缺页异常时可能所需的信息如下：

    * 触发 page fault 时访问的虚拟内存地址。当触发 page fault 时，`stval` 寄存器被被硬件自动设置为该出错的 VA 地址
    * 导致 page fault 的类型，保存在 `scause` 寄存器中
        * Exception Code = 12: page fault caused by an instruction fetch 
        * Exception Code = 13: page fault caused by a read  
        * Exception Code = 15: page fault caused by a write 
    * 发生 page fault 时的指令执行位置，保存在 `sepc` 中
    * 当前进程合法的 VMA 映射关系，保存在 `vm_area_struct` 链表中
    * 发生异常的虚拟地址对应的 PTE (page table entry) 中记录的信息

总的说来，处理缺页异常需要进行以下步骤：

* 捕获异常
* 寻找当前 task 中导致产生了异常的地址对应的 VMA
    * 如果当前访问的虚拟地址在 VMA 中没有记录，即是不合法的地址，则运行出错（本实验不涉及）
    * 如果当前访问的虚拟地址在 VMA 中存在记录，则需要判断产生异常的原因：
        * 如果是匿名区域，那么开辟一页内存，然后把这一页映射到产生异常的 task 的页表中
        * 如果不是，则访问的页是存在数据的（如代码），需要从相应位置读取出内容，然后映射到页表中
* 返回到产生了该缺页异常的那条指令，并继续执行程序

- Fork 将进行了该系统调用的 task 完整地复制一份，并加入 Ready Queue。这样在下一次调度发生时，调度器就能够发现多了一个 task。从这时候开始，新的 task 就可能被正式从 Ready 调度到 Running而开始执行了。需留意，fork 具有以下特点：
    * Fork 通过复制当前进程创建一个新的进程，新进程称为子进程，而原进程称为父进程
    * 子进程和父进程在不同的内存空间上运行
    * Fork 成功时，父进程返回子进程的 PID，子进程返回 `0`；失败时，父进程返回 `-1`
    * 创建的子 task 需要深拷贝 `task_struct`，调整自己的页表、栈和 CSR 寄存器等信息，复制一份在用户态会用到的内存信息（用户态的栈、程序的代码和数据等），并且将自己伪装成是一个因为调度而加入了 Ready Queue 的普通程序来等待调度。在调度发生时，这个新 task 就像是原本就在等待调度一样，被调度器选择并调度。

!!! tip
    - Linux 的另一个重要系统调用是 `exec`，它的作用是将进行了该系统调用的 task 换成另一个 task 。这两个系统调用一起，支撑起了 Linux 处理多任务的基础。
    * 当我们在 shell 里键入一个程序的目录时，shell（比如 zsh 或 bash）会先进行一次 fork，这时候相当于有两个 shell 正在运行。然后其中的一个 shell 根据 fork 的返回值（是否为 0），发现自己和原本的 shell 不同，再调用 exec 来把自己给换成另一个程序，这样 shell 外的程序就得以执行了。

### 代码目录

### 代码框架

- vma结构
```c
struct vm_area_struct {
    struct mm_struct *vm_mm;    // 所属的 mm_struct
    uint64_t vm_start;          // VMA 对应的用户态虚拟地址的开始
    uint64_t vm_end;            // VMA 对应的用户态虚拟地址的结束
    struct vm_area_struct *vm_next, *vm_prev;   // 链表指针
    uint64_t vm_flags;          // VMA 对应的 flags
    // struct file *vm_file;    // 对应的文件（目前还没实现，而且我们只有一个 uapp 所以暂不需要）
    uint64_t vm_pgoff;          // 如果对应了一个文件，那么这块 VMA 起始地址对应的文件内容相对文件起始位置的偏移量
    uint64_t vm_filesz;         // 对应的文件内容的长度
};

struct mm_struct {
    struct vm_area_struct *mmap;
};

struct task_struct {
    uint64_t state;    
    uint64_t counter; 
    uint64_t priority; 
    uint64_t pid;    

    struct thread_struct thread;
    uint64_t *pgd;
    struct mm_struct mm;
};
```

### 运行流程

### 实验精简

* `find_vma` 函数：实现对 `vm_area_struct` 的查找
    * 根据传入的地址 `addr`，遍历链表 `mm` 包含的 VMA 链表，找到该地址所在的 `vm_area_struct`
    * 如果链表中所有的 `vm_area_struct` 都不包含该地址，则返回 `NULL`

* `do_mmap` 函数：实现 `vm_area_struct` 的添加
    * 新建 `vm_area_struct` 结构体，根据传入的参数对结构体赋值，并添加到 `mm` 指向的 VMA 链表中

* 修改 task_init

> 在初始化一个 task 时我们既不分配内存，又不更改页表项来建立映射。回退到用户态进行程序执行的时候就会因为没有映射而发生 page fault，进入我们的 page fault handler 后，我们再分配空间（按需要拷贝内容）进行映射。
> 
> 例如，我们原本要为用户态虚拟地址映射一个页，需要进行如下操作：
> 
> 1. 使用 `kalloc` 或者 `alloc_page` 分配一个页的空间
> 2. 对这个页中的数据进行填充
> 3. 将这个页映射到用户空间，供用户程序访问。并设置好对应的 U, W, X, R 权限，最后将 V 置为 1，代表其有效。
> 
> 而为了减少 task 初始化时的开销，我们这样对一个 **Segment** 或者**用户态的栈**建立映射的操作只需改成分别建立一个 VMA 即可，具体的分配空间、填充数据的操作等后面再来完成。

所以我们需要修改 `task_init` 函数代码，更改为 demand paging：

* 删除（注释）掉之前实验中对用户栈、代码 load segment 的映射操作（alloc 和 create_mapping）
* 调用 `do_mmap` 函数，建立用户 task 的虚拟地址空间信息，在本次实验中仅包括两个区域:
    * 代码和数据区域：该区域从 ELF 给出的 Segment 起始用户态虚拟地址 `phdr->p_vaddr` 开始，对应文件中偏移量为 `phdr->p_offset` 开始的部分
    * 用户栈：范围为 `[USER_END - PGSIZE, USER_END)`，权限为 `VM_READ | VM_WRITE`，并且是匿名的区域（`VM_ANON`）

```
SET [PID = 1 PRIORITY = 7 COUNTER = 7]

switch to [PID = 1 PRIORITY = 7 COUNTER = 7]
[trap.c,129,trap_handler] [S] Unhandled Exception: scause=12, sepc=0x100e8, stval=0x100e8
```
> 可以看到，发生了缺页异常的 `sepc` 是 `0x100e8`，说明我们在 `sret` 来执行用户态程序的时候，第一条指令就因为 `V-bit` 为 0 表征其映射的地址无效而发生了异常，并且发生的异常是 12 号 Insturction Page Fault。

- `do_page_fault` 用以同时处理三种不同的 page fault。

函数的具体逻辑为：

1. 通过 `stval` 获得访问出错的虚拟内存地址（Bad Address）
2. 通过 `find_vma()` 查找 bad address 是否在某个 vma 中
    - 如果不在，则出现非预期错误，可以通过 `Err` 宏输出错误信息
    - 如果在，则根据 vma 的 flags 权限判断当前 page fault 是否合法
        - 如果非法（比如触发的是 instruction page fault 但 vma 权限不允许执行），则 `Err` 输出错误信息
        - 其他情况合法，需要我们按接下来的流程创建映射
3. 分配一个页，接下来要将这个页映射到对应的用户地址空间
4. 通过 `(vma->vm_flags & VM_ANON)` 获得当前的 VMA 是否是匿名空间
    - 如果是匿名空间，则直接映射即可
    - 如果不是，则需要根据 `vma->vm_pgoff` 等信息从 ELF 中读取数据，填充后映射到用户空间

- 实现 fork 系统调用
    - 修改 proc 相关代码，使其只初始化一个进程，其他进程保留为 NULL 等待 fork 创建
    - 添加系统调用处理
        - Fork 在 Linux 中的系统调用是 `SYS_CLONE`，其调用号为 220，所以需要在合适的位置加上 `#!c #define SYS_CLONE 220`（包括 `user/` 下的 `syscall.h`）。
        - 然后在系统调用的处理函数中，检测到 `regs->a7 == SYS_CLONE` 时，调用 `do_fork` 函数来完成 fork 的工作。
- do_fork
    - 创建一个新进程：
        - 拷贝内核栈（包括了 `task_struct` 等信息）
        - 创建一个新的页表
            - 拷贝内核页表 `swapper_pg_dir`
            - 遍历父进程 vma，并遍历父进程页表
                - 将这个 vma 也添加到新进程的 vma 链表中
                - 如果该 vma 项有对应的页表项存在（说明已经创建了映射），则需要深拷贝一整页的内容并映射到新页表中
    - 将新进程加入调度队列
    - 处理父子进程的返回值
        - 父进程通过 `do_fork` 函数直接返回子进程的 pid，并回到自身运行
        - 子进程通过被调度器调度后（跳到 `thread.ra`），开始执行并返回 0

- COW: copy on write
    - COW 的核心是，再将 `do_fork` 中分配页面、拷贝内容的操作后移，移动到出现写操作的时候再进行拷贝，这样如果是只读的页面就可以在父子进程之间共享，免去拷贝的开销。
        - 因为要共享页面，所以我们要稍微更改 `mm.c` 中的 buddy system，为每个页添加一个引用计数 refcnt

接下来在我们 `do_fork` 创建页面、拷贝内容、创建页表的时候，只需要：

- 将物理页的引用计数加一
- 将父进程的该地址对应的页表项的 `PTE_W` 位置 0
    - 注意因为修改了页表项权限，所以全部修改完成后需要通过 `sfence.vma` 刷新 TLB
- 为子进程创建一个新的页表项，指向父进程的物理页，且权限不带 `PTE_W`

这样在父子进程想要写入的时候，就会触发 page fault，然后再由我们在 page fault handler 中进行 COW。在 handler 中，我们只需要判断，如果发生了写错误，且 vma 的 `VM_WRITE` 位为 1，而且对应地址有 pte（进行了映射）但 pte 的 `PTE_W` 位为 0，那么就可以断定这是一个写时复制的页面，我们只需要在这个时候拷贝一份原来的页面，重新创建一个映射即可。

!!! tip "关于引用计数"
    拷贝了页面之后，别忘了将原来的页面引用计数减一。这样父子进程想要写入的时候，都会触发 COW，并拷贝一个新页面，都拷贝完成后，原来的页面将自动 free 掉。

    进一步的，父进程 COW 后，子进程再进行写入的时候，也可以在这时判断引用计数，如果计数为 1，说明这个页面只有一个引用，那么就可以直接将 pte 的 `PTE_W` 位再置 1，这样就可以直接写入了，免去一次额外的复制。

### 思考

