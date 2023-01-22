#include "types.h"
#include "sbi.h"
// #include <stdlib.h>

// eg
// unsigned long long s_example(unsigned long long type,unsigned long long arg0) {
//     unsigned long long ret_val;
//     __asm__ volatile (
//         "mv x10, %[type]\n"
//         "mv x11, %[arg0]\n"
//         "mv %[ret_val], x12"
//         : [ret_val] "=r" (ret_val)
//         : [type] "r" (type), [arg0] "r" (arg0)
//         : "memory"
//     );
//     return ret_val;
// }

struct sbiret sbi_ecall(int ext, int fid, uint64 arg0,
			            uint64 arg1, uint64 arg2,
			            uint64 arg3, uint64 arg4,
			            uint64 arg5){
    // struct sbiret* ret = (struct sbiret *)malloc(sizeof(struct sbiret));
    struct sbiret ret;
    // long error;
    // long value;
    __asm__ volatile(
        "lw a7, %[ext]\n"
        "lw a6, %[fid]\n"
        "ld a5, %[arg5]\n"
        "ld a4, %[arg4]\n"
        "ld a3, %[arg3]\n"
        "ld a2, %[arg2]\n"
        "ld a1, %[arg1]\n"
        "ld a0, %[arg0]\n"
        "ecall\n"
        "sd a0, %[error]\n"
        "sd a1, %[value]\n"
        : [value] "=m" (ret.value), [error] "=m" (ret.error)
        : [ext] "m" (ext), [fid] "m" (fid), [arg5] "m" (arg5), [arg4] "m" (arg4), [arg3] "m" (arg3), [arg2] "m" (arg2), [arg1] "m" (arg1), [arg0] "m" (arg0)
        : "memory"
    );
    // ret.error = error;
    // ret.value = value;
    return ret;
}

