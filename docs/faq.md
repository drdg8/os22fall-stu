# 常见问题及解答
- [常见问题及解答](#常见问题及解答)
  - [1 为什么我把 Linux 源码放在共享文件夹或 wsl2 的 `/mnt` 下编译不出来？](#1-为什么我把-linux-源码放在共享文件夹或-wsl2-的-mnt-下编译不出来)
  - [2 为什么 QEMU & GDB 使用 `si` 单指令调试遇到模式切换时无法正常执行？](#2-为什么-qemu--gdb-使用-si-单指令调试遇到模式切换时无法正常执行)
  - [3 为什么我不能在 GDB 中使用 `next` 或者 `finish` ?](#3-为什么我不能在-gdb-中使用-next-或者-finish-)
  - [4 为什么我在内核中添加了 debug 信息，但是还是没法使用 `next` 或者 `finish` ?](#4-为什么我在内核中添加了-debug-信息但是还是没法使用-next-或者-finish-)
  - [5  为什么我在 `start_kernel` 处不能正常使用断点？](#5--为什么我在-start_kernel-处不能正常使用断点)
  - [6 为什么 Lab1 中提示 `riscv64-elf-unknown-gcc: No such file or directory` ?](#6-为什么-lab1-中提示-riscv64-elf-unknown-gcc-no-such-file-or-directory-)
  - [7 为什么 Lab1 中我的 C 语言函数的参数无法正确传入？](#7-为什么-lab1-中我的-c-语言函数的参数无法正确传入)
  - [8 为什么我把 `puti` 的参数类型替换成 `uint64` 还是只能打印出 32bits 的值？](#8-为什么我把-puti-的参数类型替换成-uint64-还是只能打印出-32bits-的值)

首先需要明确的是，本次实验中的所有操作都不应该经由 Windows 中的文件系统，请直接在 **虚拟机或 Linux 物理机** 中直接完成。

## 1 为什么我把 Linux 源码放在共享文件夹或 wsl2 的 `/mnt` 下编译不出来？

这种情况下，Linux 在使用 Windows 上的文件系统。请使用 `wget` 等工具将 Linux 源码下载至容器内目录**而非共享目录或 `/mnt` 目录下的任何位置**，然后执行编译。

## 2 为什么 QEMU & GDB 使用 `si` 单指令调试遇到模式切换时无法正常执行？

在遇到诸如 `mret`, `sret` 等指令造成的模式切换时，`si` 指令会失效，可能表现为程序开始不停跑，影响对程序运行行为的判断。

一个解决方法是在程序**预期跳转**的位置打上断点，断点不会受到模式切换的影响，比如：

```bash
(gdb) i r sepc    
sepc        0x8000babe
(gdb) b * 0x8000babe
Breakpoint 1 at 0x8000babe
(gdb) si    # 或者使用 c
Breakpoint 1, 0x000000008000babe in _never_gonna_give_you_up ()
...
```

这样就可以看到断点被触发，可以继续调试了。

## 3 为什么我不能在 GDB 中使用 `next` 或者 `finish` ?

这两条命令都依赖在内核中添加的调试信息，可以通过 `menuconfig` 进行配置添加。我们在实验中没有对这部分内容作要求，可以自行 Google 探索。

## 4 为什么我在内核中添加了 debug 信息，但是还是没法使用 `next` 或者 `finish` ?

可能你在配置内核时已经添加了调试信息，但是并没有在 **QEMU运行的其他部分** 添加。例如 SRAM 中对 `march` 进行配置的过程，以及 opensbi 中的所有部分，都缺少调试信息。所以才无法按照函数的层级进行调试。我们在实验中没有对这部分内容作要求，可以自行 Google 探索。

## 5  为什么我在 `start_kernel` 处不能正常使用断点？

在以下版本中，这个断点能够正常被打上并触发

```
GNU gdb (Ubuntu 12.0.90-0ubuntu1) 12.0.90
QEMU emulator version 6.2.0 (Debian 1:6.2+dfsg-2ubuntu6.3)
RISC-V GNU Toolchain (Ubuntu 11.2.0-16ubuntu1) 11.2.0
linux-6.0-rc5/linux-5.19.9
```

## 6 为什么 Lab1 中提示 `riscv64-elf-unknown-gcc: No such file or directory` ?

我们更新了工具链，请使用 `git pull` 来更新仓库信息，然后使用 `make clean` 清除原先的编译产物。

## 7 为什么 Lab1 中我的 C 语言函数的参数无法正确传入？

确认自己是否在 `head.S` 里的 `_start` 函数中正确设置了 `sp`，正常情况下它的值应该是 `0x8020XXXX`。未设置 `sp` 会使栈上的值不正确且无法写入。

## 8 为什么我把 `puti` 的参数类型替换成 `uint64` 还是只能打印出 32bits 的值？

**强烈建议**把 `uint64` 的定义替换为 `unsigned long long` 而不是现在使用的 `unsigned long`，因为前者在32位和64位的平台上的长度都是64位，而后者在32位的平台上是32位的。
