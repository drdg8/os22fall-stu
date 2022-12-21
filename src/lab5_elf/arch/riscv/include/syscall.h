#include "stdint.h"

#define SYS_WRITE   64
#define SYS_GETPID  172

struct pt_regs{
    uint64_t x[32];
    uint64_t sepc;
    // uint64_t sstatus;
};

void syscall(struct pt_regs *reg);
