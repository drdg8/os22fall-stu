// arch/riscv/kernel/vm.c
#include "defs.h"
#include "types.h"

// for 39 bits: max 8 fff fff fff
// for 30 bits: max 3f fff fff

/* early_pgtbl: 用于 setup_vm 进行 1GB 的 映射。 */
// here the page entry is 64 bits
unsigned long early_pgtbl[512] __attribute__((__aligned__(0x1000)));

void setup_vm(void) {
    /* 
    1. 由于是进行 1GB 的映射 这里不需要使用多级页表 
    2. 将 va 的 64bit 作为如下划分： | high bit | 9 bit | 30 bit |
        high bit 可以忽略
        中间9 bit 作为 early_pgtbl 的 index
        低 30 bit 作为 页内偏移 这里注意到 30 = 9 + 9 + 12， 即我们只使用根页表， 根页表的每个 entry 都对应 1GB 的区域。 
    3. Page Table Entry 的权限 V | R | W | X 位设置为 1
    */

    // here the flag 10 bits
    unsigned long flag = 0b1111;
    // here the early_pgtbl is physical address
    // PA = VA
    early_pgtbl[PHY_START >> 30] = (PHY_START >> 30 << 28) + flag;
    // you need to flash high bit
    // PA + PA2VA_OFFSET = VA
    early_pgtbl[VM_START << 25 >> 25 >> 30] = (PHY_START >> 30 << 28) + flag;
    // printk("%lx, early_pgtbl[%lx]: %lx\n\n", early_pgtbl, VM_START << 25 >> 25 >> 30, early_pgtbl[VM_START << 25 >> 25 >> 30]);

}

/* swapper_pg_dir: kernel pagetable 根目录， 在 setup_vm_final 进行映射。 */
unsigned long swapper_pg_dir[512] __attribute__((__aligned__(0x1000)));

extern char _stext[];
extern char _etext[];
extern char _srodata[];
extern char _erodata[];
extern char _sdata[];

// extern uint64 _stext;
// extern uint64 _etext;
// extern uint64 _srodata;
// extern uint64 _erodata;
// extern uint64 _sdata;

void setup_vm_final(void) {
    memset(swapper_pg_dir, 0x0, PGSIZE);

    // No OpenSBI mapping required

    uint64 va;
    uint64 *pa_swapper_pg_dir = (uint64 *)((uint64)swapper_pg_dir - PA2VA_OFFSET);
    printk("_stext: 0x%lx\n", _stext);
    printk("_etext: 0x%lx\n", _etext);
    printk("_srodata: 0x%lx\n", _srodata);
    printk("_erodata: 0x%lx\n", _erodata);
    printk("_sdata: 0x%lx\n", _sdata);
    printk("_end: 0x%lx\n", VM_START + PHY_SIZE);
    printk("swapper_pg_dir: 0x%lx\n", swapper_pg_dir);
    printk("pa_swapper_pg_dir: 0x%lx\n\n\n", pa_swapper_pg_dir);

    // mapping kernel text X|-|R|V
    for(va = _stext; va < _etext; va += PGSIZE){
        // printk("va: %lx\n", va);
        create_mapping(pa_swapper_pg_dir, va, va - PA2VA_OFFSET, PGSIZE, 0b1011);
    }
    // create_mapping(swapper_pg_dir, _stext,  _stext - PA2VA_OFFSET, PGSIZE, 0b1011);
    // while (1);

    // mapping kernel rodata -|-|R|V
    for(va = _srodata; va < _erodata; va += PGSIZE){
        create_mapping(pa_swapper_pg_dir, va, va - PA2VA_OFFSET, PGSIZE, 0b0011);
    }
    // create_mapping(pa_swapper_pg_dir, _srodata,  _srodata - PA2VA_OFFSET, PGSIZE, 0b0011);
    
    // mapping other memory -|W|R|V
    for(va = _sdata; va < VM_START + PHY_SIZE; va += PGSIZE){
        // printk("va: %lx\n", va);
        create_mapping(pa_swapper_pg_dir, va, va - PA2VA_OFFSET, PGSIZE, 0b0111);
    }

    // set satp with swapper_pg_dir
    uint64 val = ((uint64)8 << 60) + ((uint64)(pa_swapper_pg_dir) >> 12);
    csr_write(satp, val);
    printk("satp: %lx\n", csr_read(satp));

    // flush TLB
    asm volatile("sfence.vma zero, zero");

    // flush icache
    asm volatile("fence.i");
    printk("...set_up_final done!\n");
    return;
}

// it just get pgtbl 2/1, not 0
// + is super than <<
uint64 *get_next_pgtbl_base(uint64 *pgtbl, uint64 VPN){
    uint64 next_pgtbl;
    // V = 1 
    // no priority control
    if(pgtbl[VPN] % 2){
        next_pgtbl = pgtbl[VPN] >> 10 << 12;
    }else{
        next_pgtbl = kalloc() - PA2VA_OFFSET;
        // pgn = address >> 12
        pgtbl[VPN] = (next_pgtbl >> 12 << 10) + 1;
    }
    return (uint64 *)next_pgtbl;
}

/* 创建多级页表映射关系 */
// 这是一个只支持sz = PGSIZE的
create_mapping(uint64 *pgtbl2, uint64 va, uint64 pa, uint64 sz, int perm) {
    /*
    pgtbl 为根页表的基地址
    va, pa 为需要映射的虚拟地址、物理地址
    sz 为映射的大小
    perm 为映射的读写权限

    创建多级页表的时候可以使用 kalloc() 来获取一页作为页表目录
    可以使用 V bit 来判断页表项是否存在
    */
    // for sz < PGSIZE
    uint64 VPN2 = va << 25 >> 25 >> 30;
    uint64 VPN1 = va << 34 >> 34 >> 21;
    uint64 VPN0 = va << 43 >> 43 >> 12;
    uint64 VOFF = va << 52 >> 52;
    uint64 PPN = pa >> 12;
    uint64 POFF = pa << 52 >> 52;
    if (sz != PGSIZE){
        printk("ERROR: sz != PGSIZE\n");
    }
    if (VOFF != POFF){
        printk("ERROR: VOFF != POFF\n");
    }

    uint64 *pgtbl1 = get_next_pgtbl_base(pgtbl2, VPN2);
    // printk("%lx, pgtbl2[%lx]: %lx, pn = %lx\n", pgtbl2, VPN2, pgtbl2[VPN2], pgtbl2[VPN2] >> 10 << 12);
    uint64 *pgtbl0 = get_next_pgtbl_base(pgtbl1, VPN1);
    // printk("%lx, pgtbl1[%lx]: %lx, pn = %lx\n", pgtbl1, VPN1, pgtbl1[VPN1], pgtbl1[VPN1] >> 10 << 12);
    // map PPN
    pgtbl0[VPN0] = (PPN << 10) + perm;
    // printk("pgtbl0 = %lx, PPN = %lx\n", pgtbl0, pgtbl0[VPN0]>>10<<12);
    // printk("%lx, pgtbl0[%lx]: %lx, pn = %lx\n\n", pgtbl0, VPN0, pgtbl0[VPN0], pgtbl0[VPN0] >> 10 << 12);
}
