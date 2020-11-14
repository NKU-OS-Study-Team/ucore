# 内存管理
[`1811359 关明明`]()

## 练习内容
>练习1：实现 first-fit 连续物理内存分配算法（需要编程）
>在实现first fit 内存分配算法的回收函数时，要考虑地址连续的空闲块之间的合并操作。提示:
>在建立空闲页块链表时，需要按照空闲页块起始地址来排序，形成一个有序的链表。可能会
>修改default_pmm.c中的default_init，default_init_memmap，default_alloc_pages，
>default_free_pages等相关函数。请仔细查看和理解default_pmm.c中的注释。
>请在实验报告中简要说明你的设计实现过程。请回答如下问题：
>你的first fit算法是否有进一步的改进空间
## 实验分析
default_pmm.c文件中定义了默认的内存页管理器default_pmm_manager，实现其初始化、分配页、释放页等功能，具体分为以下几个函数实现：
default_init、default_init_memmap、default_alloc_pages、default_free_pages<br>
在进行实验前，需要了解页Page以及free_area_t的结构，进入memlayout.h文件:
```c++
struct Page {
    // 页引用数
    int ref;                        
    // 用于描述页状态的标志位
    uint32_t flags;                 
    // 页属性，在first-fit算法中为空闲块的大小
    unsigned int property;          
    // 页链接，用于在空闲块链表表中使用
    list_entry_t page_link;         // free list link
};

/* Flags describing the status of a page frame */
// 保留位，若为1说明为kernel保存，不可被分配或释放，
#define PG_reserved                 0       // if this bit=1: the Page is reserved for kernel, cannot be used in alloc/free_pages; otherwise, this bit=0 
// 属性位，若为1说明为空闲块的首页，即表示页属性有效
#define PG_property                 1       // if this bit=1: the Page is the head page of a free memory block(contains some continuous_addrress pages), and can be used in alloc_pages; if this bit=0: if the Page is the the head page of a free memory block, then this Page and the memory block is alloced. Or this Page isn't the head page.
// 将保留位置零或置一
#define SetPageReserved(page)       set_bit(PG_reserved, &((page)->flags))
#define ClearPageReserved(page)     clear_bit(PG_reserved, &((page)->flags))
#define PageReserved(page)          test_bit(PG_reserved, &((page)->flags))
// 将属性位置零或置一
#define SetPageProperty(page)       set_bit(PG_property, &((page)->flags))
#define ClearPageProperty(page)     clear_bit(PG_property, &((page)->flags))
#define PageProperty(page)          test_bit(PG_property, &((page)->flags))

/* free_area_t - maintains a doubly linked list to record free (unused) pages */
typedef struct {
    //链表头
    list_entry_t free_list;         // the list header
    //空闲页
    unsigned int nr_free;           // # of free pages in this free list
} free_area_t;

```
default_init:该函数初始化列表
```c++
static void
default_init(void) {
    list_init(&free_list);
    nr_free = 0;
}

```
default_init_memmap：
```c++
    assert(n > 0);
    struct Page *p = base;
    for (; p != base + n; p ++) {
        assert(PageReserved(p));
//        将页标记位及页属性置零
        p->flags = p->property = 0;
//        将页应用为置零
        set_page_ref(p, 0);
        SetPageProperty(p);
    }
//    将块首的页属性置为n
    base->property = n;
    SetPageProperty(base);
//    更新空闲页数量
    nr_free += n;
//    插入到空闲链表中
    list_add(&free_list, &(base->page_link));
```
default_alloc_pages：
```c++
static struct Page *
default_alloc_pages(size_t n) {
    assert(n > 0);
    if (n > nr_free) {
        return NULL;
    }
    struct Page *page = NULL;
//    获取链表头
    list_entry_t *le = &free_list;
//    开始遍历
    while ((le = list_next(le)) != &free_list) {
//      转换为页结构
        struct Page *p = le2page(le, page_link);
//      当前空闲满足需求，按照first-fit思想选中
        if (p->property >= n) {
            page = p;
            break;
        }
    }
//    如果选中不为空
    if (page != NULL) {
        list_del(&(page->page_link));
        if (page->property > n) {
          //获取新空闲块的起始
          struct Page *p = page + n;
          //计算新空闲块的大小
          p->property = page->property - n;
          SetPageProperty(base);
          //插入到空闲链表中
          list_add(&free_list, &(p->page_link));
        }
//        空闲页数量减少n
        nr_free -= n;
//        将页属性置为无效（因为已被分配）
        ClearPageProperty(page);
//        将原先空闲块从链表中删去
        list_del(&(page->page_link));
    }
    return page;
}
```
default_free_pages:
```c++
static void
default_free_pages(struct Page *base, size_t n) {
    assert(n > 0);
    struct Page *p = base;
    for (; p != base + n; p ++) {
        assert(!PageReserved(p) && !PageProperty(p));
//        将页标志位与引用位置零
        p->flags = 0;
        set_page_ref(p, 0);
    }
//       释放内存,设置页属性,标为有效
    base->property = n;
    SetPageProperty(base);
//      获取表头
    list_entry_t *le = list_next(&free_list);
    while (le != &free_list) {
        p = le2page(le, page_link);
        le = list_next(le);
//        如果被释放的块尾部恰好与现空闲的块相连
        if (base + base->property == p) {
//          合并空闲块,将之前的空闲块首页置为无效
            base->property += p->property;
//            p->property=0;
            ClearPageProperty(p);
//            并将之前的空闲块从空闲链表中删去
            list_del(&(p->page_link));
        }
//         如果被释放的块首部恰好与现空闲的块相连
        else if (p + p->property == base) {
//          合并空闲块,将被释放的块首页置为无效
            p->property += base->property;
//            p->property=0;
            ClearPageProperty(base);
            base = p;
//            并将之前的空闲块从空闲链表中删去
            list_del(&(p->page_link));
        }
    }
//    更新空闲页数量
    nr_free += n;
//    将新空闲块首页插入链表
    list_add(&free_list, &(base->page_link));
}
```
>请回答如下问题：
>你的first fit算法是否有进一步的改进空间？
>>可明显得出，随着低端分区不断划分而产生较多小分区，而较大的空闲分区存在高端，每次分配时查找时间开销会增大，如果通过特定的数据结构，通过所需空间大小进行预估处理从而减小在低端的遍历查>>找，应该能够提高first fit算法的性能.
