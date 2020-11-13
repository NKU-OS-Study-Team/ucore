# 内存管理
[`1811363 洪一帆`]()

***
## 实验内容分析

>为了完成物理内存管理，这里首先需要探测可用的物理内存资源；了解到物理内存位于什么
地方，有多大之后，就以固定页面大小来划分整个物理内存空间，并准备以此为最小内存分
配单位来管理整个物理内存，管理在内核运行过程中每页内存，设定其可用状态（free的，
used的，还是reserved的），这其实就对应了我们在课本上讲到的连续内存分配概念和原理
的具体实现；接着ucore
kernel就要建立页表，
启动分页机制，让CPU的MMU把预先建立好
的页表中的页表项读入到TLB中，根据页表项描述的虚拟页（Page）与物理页帧（Page
Frame）的对应关系完成CPU对内存的读、写和执行操作。这一部分其实就对应了我们在课
本上讲到内存映射、页表、多级页表等概念和原理的具体实现。
在代码分析上，建议根据执行流程来直接看源代码，并可采用GDB源码调试的手段来动态地
分析ucore的执行过程。内存管理相关的总体控制函数是pmm_init函数，它完成的主要工作包
括：
>1. 初始化物理内存页管理器框架pmm_manager；
>2. 建立空闲的page链表，这样就可以分配以页（4KB）为单位的空闲内存了；
>3. 检查物理内存页分配算法；
>4. 为确保切换到分页机制后，代码能够正常执行，先建立一个临时二级页表；
>5. 建立一一映射关系的二级页表；
>6. 使能分页机制；
>7. 从新设置全局段描述符表；
>8. 取消临时二级页表；
>9. 检查页表建立是否正确；
>10. 通过自映射机制完成页表的打印输出（这部分是扩展知识）

### memlayout.h

- 定义page类及其属性：标志位、空闲块
- 内存的分段与组织
- 预定义的函数，主要用于对页表进行相应的操作
- 声明一个`free_area_t`，用于标明空闲页表
```c++
/* *
* Virtual memory map:                                          Permissions
*                                                              kernel/user
*
*     4G ------------------> +---------------------------------+
*                            |                                 |
*                            |         Empty Memory (*)        |
*                            |                                 |
*                            +---------------------------------+ 0xFB000000
*                            |   Cur. Page Table (Kern, RW)    | RW/-- PTSIZE
*     VPT -----------------> +---------------------------------+ 0xFAC00000
*                            |        Invalid Memory (*)       | --/--
*     KERNTOP -------------> +---------------------------------+ 0xF8000000
*                            |                                 |
*                            |    Remapped Physical Memory     | RW/-- KMEMSIZE
*                            |                                 |
*     KERNBASE ------------> +---------------------------------+ 0xC0000000
*                            |                                 |
*                            |                                 |
*                            |                                 |
*                            ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
* (*) Note: The kernel ensures that "Invalid Memory" is *never* mapped.
*     "Empty Memory" is normally unmapped, but user programs may map pages
*     there if desired.
*
* */

 /**
 * struct Page - Page descriptor structures. Each Page describes one
 * physical page. In kern/mm/pmm.h, you can find lots of useful functions
 * that convert Page to other data types, such as phyical address.
 **/
struct Page {
    int ref;                        // page frame's reference counter
    uint32_t flags;                 // array of flags that describe the status of the page frame
    unsigned int property;          // the num of free block, used in first fit pm manager
    list_entry_t page_link;         // free list link，一个双向链表
};

/* free_area_t - maintains a doubly linked list to record free (unused) pages */
typedef struct {
    list_entry_t free_list;         // the list header
    unsigned int nr_free;           // # of free pages in this free list
} free_area_t;
```

### mmu.h
主要存放各种descriptor

- 定义状态变量、常量
- 定义门
- 定义段：基址、段大小、段权限
- 定义进程状态：栈指针、段选择器
```
// A linear address 'la' has a three-part structure as follows:
//
// +--------10------+-------10-------+---------12----------+
// | Page Directory |   Page Table   | Offset within Page  |
// |      Index     |     Index      |                     |
// +----------------+----------------+---------------------+
//  \--- PDX(la) --/ \--- PTX(la) --/ \---- PGOFF(la) ----/
//  \----------- PPN(la) -----------/
```


### pmm.h (physical memory management)


```c++
// virtual address of boot-time page directory
extern pde_t *boot_pgdir;
// physical address of boot-time page directory
uintptr_t boot_cr3;

void pmm_init(void);

// pmm_manager is a physical memory management class. A special pmm manager - XXX_pmm_manager
// only needs to implement the methods in pmm_manager class, then XXX_pmm_manager can be used
// by ucore to manage the total physical memory space.
struct pmm_manager {
    const char *name;                                 // XXX_pmm_manager's name
    void (*init)(void);                               // initialize internal description&management data structure
                                                      // (free block list, number of free block) of XXX_pmm_manager 
    void (*init_memmap)(struct Page *base, size_t n); // setup description&management data structcure according to
                                                      // the initial free physical memory space 
    struct Page *(*alloc_pages)(size_t n);            // allocate >=n pages, depend on the allocation algorithm 
    void (*free_pages)(struct Page *base, size_t n);  // free >=n pages with "base" addr of Page descriptor structures(memlayout.h)
    size_t (*nr_free_pages)(void);                    // return the number of free pages 
    void (*check)(void);                              // check the correctness of XXX_pmm_manager ，用来编写测试函数
};
```

***
## 吐槽

>这个内容感觉有点多有点散，自己组织不大起来。
>
>比如很多函数好像看到过，但是忘记是干嘛的了。
>
>再比如很多的宏定义，使得原本已经复杂的内容更加复杂难以掌握。尽管可能是为了增加可读性，但是仅限于对程序结构有良好掌握的人而言。
>
>说到底还是自己对于ucore的掌握不够。