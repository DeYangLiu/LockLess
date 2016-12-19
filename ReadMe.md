# lockless data structrues and algorithms
* 有序单链表
* 二叉树
* 字典树


## 垃圾回收算法
RCU/QSBR -- many readers, only one writers
EBR/FEBR -- 分代回收
Boehm GC


## 实现考虑
### 原子指令
 intel是加上lock前缀锁总线；
 arm 是ldrex/strex。
 
### 乱序
 intel上自然对齐的操作指令不需要，atomic_thread_fence 展开为空；
 arm上展开为dmb sy。

### C11 API:
1. atomic_thread_fence(memory_order_acquire)
等待其他cpu把缓存数据写回到内存，然后当前cpu去读才安全。

2. 内存存取操作

3. atomic_thread_fence(memory_order_release)
把当前cpu的缓存数据写回到内存。

acquire 相当于对内存加锁，release相当释放内存锁。
和使用pthread互斥锁一样，注意位置要对，否则arm上会死锁。

用cas实现的_spin_lock效率不高，以后优化可以参考[linux-spinlock]。


## 测试结果
IO对严重影响结果，不要DEBUG, 重定向输出。

### Intel® Core™ i3-2328M CPU @ 2.20GHz x 4
6GB mem

time ./fine N > log
N: min -- max ms
1: 5 -- 9
2: 5 -- 23
3: 12 -- 43
4: 33 -- 101
5: 1550 -- 5668
16: 59930

time ./lazy N > log
N: min -- max ms
1: 3 -- 9
2: 4 -- 16
3: 7 -- 22
4: 13 -- 27
5: 13 -- 43
16: 126 -- 251
32: 134 -- 759

### ARMv7 x 4. BogoMIPS : 12.00
/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq = 1.6M Hz
2GB mem

time ./lazy N
N: min -- max ms
1: 30 -- 40
2: 30 -- 40
3: 30 -- 40
4: 40 -- 60
5: 50 -- 90
16: 250 -- 340
32: 690 -- 1150

time ./fine N
N: min -- max ms
1: 30 -- 40
2: 70 -- 90
3: 90 -- 130
4: 130 -- 330
5: 2000 -- 2290


## ref
[gcc-atomic] https://gcc.gnu.org/onlinedocs/gcc/_005f_005fatomic-Builtins.html
https://gcc.gnu.org/onlinedocs/gcc-4.1.2/gcc/Atomic-Builtins.html

[linux-spinlock] http://www.wowotech.net/kernel_synchronization/spinlock.html


[febr] 彭建章，顾乃杰，张旭，张颖楠，魏振伟. 快速时代回收：一种针对无锁编程的快速垃圾回收算法[J]. , 2013, 34(12): 2691-2695.
PENG Jian-zhang，GU Nai-jie，ZHANG Xu,ZHANG Ying-nan, WEI Zhen-wei. 
 Fast Epoch: a Fast Memory Reclamation Algorithm for Lock-free Programming. , 2013, 34(12): 2691-2695.
http://xwxt.sict.ac.cn/CN/article/downloadArticleFile.do?attachType=PDF&id=552

