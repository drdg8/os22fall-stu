qemu-system-riscv64 -nographic -machine virt -kernel /home/drdg8/lab/os22fall-stu/linux-6.0-rc5/arch/riscv/boot/Image
-device virtio-blk-device,drive=hd0 -append "root=/dev/vda ro console=ttyS0"
-bios default -drive file=/home/drdg8/os22fall-stu/src/lab0/rootfs.img,format=raw,id=hd0 -S -s

gdb-multiarch /home/drdg8/lab/os22fall-stu/linux-6.0-rc5/vmlinux

cd path/to/linux
make ARCH=riscv CROSS_COMPILE=riscv64-linux-gnu- defconfig
make ARCH=riscv CROSS_COMPILE=riscv64-linux-gnu- -j$(nproc)

退出 QEMU 的方法为：使用 Ctrl+A，松开后再按下 X 键即可退出 QEMU。

make ARCH=arm CROSS_COMPILE=arm-linux-gnueabi- defconfig    # 使用默认配置