#include "print.h"
#include "sbi.h"
#include "defs.h"

extern void test();

int start_kernel() {
    puti(2022);
    puts(" Hello RISC-V\n");

    puti(csr_read(sstatus));
    puts(" sstatus \n");

    puti(csr_read(sscratch));
    puts("before change sscratch \n");

    csr_write(sscratch, 0x80200000);
    puti(csr_read(sscratch));
    puts("\nafter change sscratch \n");

    test(); // DO NOT DELETE !!!

	return 0;
}
