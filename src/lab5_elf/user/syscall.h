#define SYS_WRITE   64
#define SYS_GETPID  172

struct pg_regs{
    unsigned long x[32];
    unsigned long sepc;
    // uint64_t sstatus;
};

void syscall(struct pg_regs *reg);