# Small-operating-system a small operating system，base on X86 architecture.

# 完成的功能模块有

## 终端命令行演示模块
可通过预置命令，实现在终端实现类似shell功能。如下图所示，在终端中输入的命令为 `mem` 表示获得系统的物理内存空间大小。

![image](https://github.com/Buildings-Lei/Small-operating-system-/blob/main/images/mem.gif)

## 内核态与用户态的分离
该系统主要的代码分为两部分，一部分为内核态代码，另一部分为用户态代码。如下图所示，从0x7c00 到 0x12000为内核态代码范围。用户态代码为0x12000到0x70000。从0x70000 到 0x90000则是内核态的堆空间的范围。其中内核态代码的保护采用的是分页机制中页表只读属性实现对内核态代码保护。内核态的堆空间采用的是分段机制的方式进行保护。

![image](https://github.com/Buildings-Lei/Small-operating-system-/blob/main/images/memory.png)

## 多任务的切换模块
通过TSS和内核态空间保存的寄存器的值，进行任务的切换。每一个任务在内核态中都有一块内存保存任务切换时各个寄存器的值，通过TSS实现栈的切换，实现多个任务之间的切换。目前采用的分时复用的切换策略。在任务互斥模块中将会演示消费者和生产者的代码，将会有任务切换演示。

## 任务之间的互斥锁模块
实现了互斥锁的功能，用户要加锁，可通过系统调用，陷入内核，完成对锁的持有。释放锁采用的发布订阅模式，会把因为这把锁而等待的任务全部进行唤醒，进入就绪队列当中。

![image](https://github.com/Buildings-Lei/Small-operating-system-/blob/main/images/demo1.gif)

## 动态内存的分配模块

采用定长和动长想结合的策略进行分配，若能分配空间，最少分配32k的大小。有点内存碎片了，后续再改。

## 键盘输入模块

外部中断设置，8259A的设置。级联的方式，键盘的中断向量是 0x21 ，系统功能调用是 0x80 ，时间中断是 0x20 。


# 环境

> Ubuntu 10.10 

> gcc 4.4.5

> NASM version 2.08.01

> bochs 2.4.5



