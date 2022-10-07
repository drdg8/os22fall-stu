#include "print.h"
#include "sbi.h"

void puts(char *s) {
    // unimplemented
    for(int i = 0; s[i] != '\0'; i++){
        sbi_ecall(0x1, 0x0, s[i], 0, 0, 0, 0, 0);
    }
}

void puti(uint64 x) {
    // unimplemented
    char s[64];
    int len;
    // reverse
    for (len = 0; x > 0; len++){
        s[len] = x % 10;
        x /= 10;
    }
    // print
    for (int i = len - 1; i >= 0; i--){
        sbi_ecall(0x1, 0x0, s[i] + '0' , 0, 0, 0, 0, 0);
    }
}
