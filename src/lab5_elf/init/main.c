#include "printk.h"
#include "sbi.h"
#include "defs.h"

extern void test();

int start_kernel() {
    // printk("2022");
    printk(" Hello RISC-V\n");

    // printk("%d", csr_read(sstatus));
    // printk(" sstatus \n");
    // 
    // printk("%d", csr_read(sie));
    // printk(" sie \n");

    schedule();
    test(); // DO NOT DELETE !!!

	return 0;
}
