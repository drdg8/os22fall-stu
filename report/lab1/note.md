make debug
gdb-multiarch /home/drdg8/lab/os22fall-stu/src/lab1/vmlinux

in gdb
target remote :1234
b _start 
c
n 单步调试（过程，函数直接执行
si 执行单条指令
bt
p _traps (print: 打印值及地址，简写 p)
finish
i r ra
i r stvec
x/4x <addr>
