
#include <pmm.h>
#include <list.h>
#include <string.h>
#include <default_pmm.h>


/*  In the First Fit algorithm, the allocator keeps a list of free blocks
 * (known as the free list). Once receiving a allocation request for memory,
 * it scans along the list for the first block that is large enough to satisfy
 * the request. If the chosen block is significantly larger than requested, it
 * is usually splitted, and the remainder will be added into the list as
 * another free block.
 *  Please refer to Page 196~198, Section 8.2 of Yan Wei Min's Chinese book
 * "Data Structure -- C programming language".
*/
// LAB2 EXERCISE 1: YOUR CODE
// you should rewrite functions: `default_init`, `default_init_memmap`,
// `default_alloc_pages`, `default_free_pages`.
/*
 * Details of FFMA
 * (1) Preparation:
 *  In order to implement the First-Fit Memory Allocation (FFMA), we should
 * manage the free memory blocks using a list. The struct `free_area_t` is used
 * for the management of free memory blocks.
 *  First, you should get familiar with the struct `list` in list.h. Struct
 * `list` is a simple doubly linked list implementation. You should know how to
 * USE `list_init`, `list_add`(`list_add_after`), `list_add_before`, `list_del`,
 * `list_next`, `list_prev`.
 *  There's a tricky method that is to transform a general `list` struct to a
 * special struct (such as struct `page`), using the following MACROs: `le2page`
 * (in memlayout.h), (and in future labs: `le2vma` (in vmm.h), `le2proc` (in
 * proc.h), etc).
 * (2) `default_init`:
 *  You can reuse the demo `default_init` function to initialize the `free_list`
 * and set `nr_free` to 0. `free_list` is used to record the free memory blocks.
 * `nr_free` is the total number of the free memory blocks.
 * (3) `default_init_memmap`:
 *  CALL GRAPH: `kern_init` --> `pmm_init` --> `page_init` --> `init_memmap` -->
 * `pmm_manager` --> `init_memmap`.
 *  This function is used to initialize a free block (with parameter `addr_base`,
 * `page_number`). In order to initialize a free block, firstly, you should
 * initialize each page (defined in memlayout.h) in this free block. This
 * procedure includes:
 *  - Setting the bit `PG_property` of `p->flags`, which means this page is
 * valid. P.S. In function `pmm_init` (in pmm.c), the bit `PG_reserved` of
 * `p->flags` is already set.
 *  - If this page is free and is not the first page of a free block,
 * `p->property` should be set to 0.
 *  - If this page is free and is the first page of a free block, `p->property`
 * should be set to be the total number of pages in the block.
 *  - `p->ref` should be 0, because now `p` is free and has no reference.
 *  After that, We can use `p->page_link` to link this page into `free_list`.
 * (e.g.: `list_add_before(&free_list, &(p->page_link));` )
 *  Finally, we should update the sum of the free memory blocks: `nr_free += n`.
 * (4) `default_alloc_pages`:
 *  Search for the first free block (block size >= n) in the free list and reszie
 * the block found, returning the address of this block as the address required by
 * `malloc`.
 *  (4.1)
 *      So you should search the free list like this:
 *          list_entry_t le = &free_list;
 *          while((le=list_next(le)) != &free_list) {
 *          ...
 *      (4.1.1)
 *          In the while loop, get the struct `page` and check if `p->property`
 *      (recording the num of free pages in this block) >= n.
 *              struct Page *p = le2page(le, page_link);
 *              if(p->property >= n){ ...
 *      (4.1.2)
 *          If we find this `p`, it means we've found a free block with its size
 *      >= n, whose first `n` pages can be malloced. Some flag bits of this page
 *      should be set as the following: `PG_reserved = 1`, `PG_property = 0`.
 *      Then, unlink the pages from `free_list`.
 *          (4.1.2.1)
 *              If `p->property > n`, we should re-calculate number of the rest
 *          pages of this free block. (e.g.: `le2page(le,page_link))->property
 *          = p->property - n;`)
 *          (4.1.3)
 *              Re-caluclate `nr_free` (number of the the rest of all free block).
 *          (4.1.4)
 *              return `p`.
 *      (4.2)
 *          If we can not find a free block with its size >=n, then return NULL.
 * (5) `default_free_pages`:
 *  re-link the pages into the free list, and may merge small free blocks into
 * the big ones.
 *  (5.1)
 *      According to the base address of the withdrawed blocks, search the free
 *  list for its correct position (with address from low to high), and insert
 *  the pages. (May use `list_next`, `le2page`, `list_add_before`)
 *  (5.2)
 *      Reset the fields of the pages, such as `p->ref` and `p->flags` (PageProperty)
 *  (5.3)
 *      Try to merge blocks at lower or higher addresses. Notice: This should
 *  change some pages' `p->property` correctly.
 */
free_area_t free_area;

#define free_list (free_area.free_list)
#define nr_free (free_area.nr_free)

static unsigned fixsize(unsigned size) {
  size |= size >> 1;
  size |= size >> 2;
  size |= size >> 4;
  size |= size >> 8;
  size |= size >> 16;
  return size+1;
}

typedef struct {
  struct Page* base;
  unsigned size;
  unsigned longest[1];
} buddy2;

buddy2 buddy;
#define buddy_size (buddy.size)
#define buddy_longest (buddy.longest)
#define buddy_base (buddy.base)

#define LEFT_LEAF(index) ((index) * 2 + 1)
#define RIGHT_LEAF(index) ((index) * 2 + 2)
#define PARENT(index) ( ((index) + 1) / 2 - 1)
#define IS_POWER_OF_2(x) (!((x)&((x)-1)))
#define MAX(a, b) ((a) > (b) ? (a) : b))
//#define ALLOC malloc


//struct buddy2* buddy2_new( int size ) {
//  struct buddy2 *self;
//  unsigned node_size;
//  int i;
//
//  if (size < 1 || !IS_POWER_OF_2(size))
//    return NULL;
//
////  self = (struct buddy2 *)ALLOC(2 GIT* size * sizeof(unsigned));
//  self->size = size;
//  node_size = size * 2;
//
//  for (i = 0; i < 2 * size - 1; ++i) {
//    if (IS_POWER_OF_2(i + 1))
//      node_size /= 2;
//    self->longest[i] = node_size;
//  }
//  return self;
//}




//初始化列表及将空闲数置为0，无需改动
static void
default_init(void) {
  list_init(&free_list);
  nr_free = 0;
}

static void
default_init_memmap(struct Page *base, size_t n) {
  assert(n > 0);
  struct Page *p = base;
  for (; p != base + n; p ++) {
    assert(PageReserved(p));
//        将页标记位及页属性置零
    p->flags = p->property = 0;
//        将页应用为置零
    set_page_ref(p, 0);
  }
//    将块首的页属性置为n
  base->property = n;
  SetPageProperty(base);
//    更新空闲页数量
  nr_free += n;
//    插入到空闲链表中
  list_add(&free_list, &(base->page_link));

}

static struct Page *
default_alloc_pages(size_t n) {
//  cprintf("alloc:%d\n",n);
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
//    如果选中不为空
      if (p != NULL) {
//        将原先空闲块从链表中删去
        list_del(&(p->page_link));
//        将页属性置为无效（因为已被分配）
        ClearPageProperty(p);
        if (p->property > n) {
          //获取新空闲块的起始
          struct Page *p1 = p + n;
          //计算新空闲块的大小
          p1->property = p->property - n;
          SetPageProperty(p1);
          //插入到空闲链表中
          list_add(&free_list, &(p1->page_link));
        }
//        空闲页数量减少n
        nr_free -= n;
      }
      page=p;
      break;
    }
  }
  return page;
}

static void
buddy_init_memmap(struct Page *base, size_t n) {
  assert(n > 0);
  struct Page *p = base;
  cprintf("base0:%p\n",p);
  for (; p != base + n; p ++) {
    assert(PageReserved(p));
//        将页标记位及页属性置零
    p->flags = p->property = 0;
//        将页应用为置零
    set_page_ref(p, 0);
  }
//    将块首的页属性置为n
  base->property = n;
  SetPageProperty(base);
//    更新空闲页数量
  nr_free += n;
//    插入到空闲链表中
  list_add(&free_list, &(base->page_link));

  buddy_base=base;
//  struct Page *p1 = le2page(le, page_link);
  cprintf("base1:%p\n",buddy_base);



//  buddy=buddy2_new(fixsize(n)>>1);
  unsigned node_size;
  int i;
  unsigned size;
  if (!IS_POWER_OF_2(n))
    size=fixsize(n)>>1;
  else
    size=n;
  nr_free += size;
//  self = (struct buddy2 *)ALLOC(2 * size * sizeof(unsigned));
  buddy_size = size;
  node_size = size * 2;

  for (i = 0; i < 2 * size - 1; ++i) {
    if (IS_POWER_OF_2(i + 1))
      node_size /= 2;
    buddy_longest[i] = node_size;
  }

  cprintf("base2:%p\n",buddy_base);
}

static struct Page *
buddy_alloc_pages(size_t size) {
  assert(size > 0);
  if (size > nr_free) {
    return NULL;
  }
//  list_entry_t *le = &free_list;
//  le = list_next(le);
  struct Page *p = buddy_base;

  unsigned index = 0;
  unsigned node_size;
  unsigned offset = 0;
//  if (buddy==NULL)
//    return -1;
  if (size <= 0)
    size = 1;
  else if (!IS_POWER_OF_2(size))
    size = fixsize(size);
  if (buddy_longest[index] < size)
    return NULL;

  for(node_size = buddy_size; node_size != size; node_size /= 2 ) {
    if (buddy_longest[LEFT_LEAF(index)] >= size)
      index = LEFT_LEAF(index);
    else
      index = RIGHT_LEAF(index);
  }
  nr_free -= node_size;
  buddy_longest[index] = 0;
  offset = (index + 1) * node_size - buddy_size;

  while (index) {
    index = PARENT(index);
    buddy_longest[index] =
    MAX(buddy_longest[LEFT_LEAF(index)], buddy_longest[RIGHT_LEAF(index)]);
  }
  struct Page *page = p+offset;
  cprintf("offset:%d\n",offset);
  cprintf("alloc:%p\n",page);
  return page;
}

static
void buddy_free_pages(int offset, size_t n) {
  unsigned node_size, index = 0;
  unsigned left_longest, right_longest;

  assert(offset >= 0 && offset < buddy_size);

  node_size = 1;
  index = offset + buddy_size - 1;

  for (; buddy_longest[index] ; index = PARENT(index)) {
    node_size *= 2;
    if (index == 0)
      return;
  }

  buddy_longest[index] = node_size;

  while (index) {
    index = PARENT(index);
    node_size *= 2;

    left_longest = buddy_longest[LEFT_LEAF(index)];
    right_longest = buddy_longest[RIGHT_LEAF(index)];

    if (left_longest + right_longest == node_size)
      buddy_longest[index] = node_size;
    else
      buddy_longest[index] = MAX(left_longest, right_longest);
  }
  nr_free += node_size;
}


static void
default_free_pages(struct Page *base, size_t n) {
  assert(n > 0);
  struct Page *p = base;
  for (; p != base + n; p ++) {
    assert(!PageReserved(p));
    assert(!PageProperty(p));
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
  list_add_before(&free_list, &(base->page_link));
}

static size_t
default_nr_free_pages(void) {
  return nr_free;
}

static void
basic_check(void) {
  struct Page *p0, *p1, *p2;
  p0 = p1 = p2 = NULL;
  assert((p0 = alloc_page()) != NULL);
  assert((p1 = alloc_page()) != NULL);
  assert((p2 = alloc_page()) != NULL);

  assert(p0 != p1 && p0 != p2 && p1 != p2);
  assert(page_ref(p0) == 0 && page_ref(p1) == 0 && page_ref(p2) == 0);

  assert(page2pa(p0) < npage * PGSIZE);
  assert(page2pa(p1) < npage * PGSIZE);
  assert(page2pa(p2) < npage * PGSIZE);

  list_entry_t free_list_store = free_list;
  list_init(&free_list);
  assert(list_empty(&free_list));

  unsigned int nr_free_store = nr_free;
  nr_free = 0;

  assert(alloc_page() == NULL);

  free_page(p0);
  free_page(p1);
  free_page(p2);
  assert(nr_free == 3);

  assert((p0 = alloc_page()) != NULL);
  assert((p1 = alloc_page()) != NULL);
  assert((p2 = alloc_page()) != NULL);

  assert(alloc_page() == NULL);

  free_page(p0);
  assert(!list_empty(&free_list));

  struct Page *p;
  assert((p = alloc_page()) == p0);
  assert(alloc_page() == NULL);

  assert(nr_free == 0);
  free_list = free_list_store;
  nr_free = nr_free_store;

  free_page(p);
  free_page(p1);
  free_page(p2);
}

// LAB2: below code is used to check the first fit allocation algorithm (your EXERCISE 1)
// NOTICE: You SHOULD NOT CHANGE basic_check, default_check functions!
static void
default_check(void) {
  int count = 0, total = 0;
  list_entry_t *le = &free_list;
  while ((le = list_next(le)) != &free_list) {
    struct Page *p = le2page(le, page_link);
    assert(PageProperty(p));
    count ++, total += p->property;
  }
  assert(total == nr_free_pages());

  basic_check();

  struct Page *p0 = alloc_pages(5), *p1, *p2;
  assert(p0 != NULL);
  assert(!PageProperty(p0));

  list_entry_t free_list_store = free_list;
  list_init(&free_list);
  assert(list_empty(&free_list));
  assert(alloc_page() == NULL);

  unsigned int nr_free_store = nr_free;
  nr_free = 0;

  free_pages(p0 + 2, 3);
  assert(alloc_pages(4) == NULL);
  assert(PageProperty(p0 + 2) && p0[2].property == 3);
  assert((p1 = alloc_pages(3)) != NULL);
  assert(alloc_page() == NULL);
  assert(p0 + 2 == p1);

  p2 = p0 + 1;
  free_page(p0);
  free_pages(p1, 3);
  assert(PageProperty(p0) && p0->property == 1);
  assert(PageProperty(p1) && p1->property == 3);

  assert((p0 = alloc_page()) == p2 - 1);
  free_page(p0);
  assert((p0 = alloc_pages(2)) == p2 + 1);

  free_pages(p0, 2);
  free_page(p2);

  assert((p0 = alloc_pages(5)) != NULL);
  assert(alloc_page() == NULL);

  assert(nr_free == 0);
  nr_free = nr_free_store;

  free_list = free_list_store;
  free_pages(p0, 5);

  le = &free_list;
  while ((le = list_next(le)) != &free_list) {
    assert(le->next->prev == le && le->prev->next == le);
    struct Page *p = le2page(le, page_link);
    count --, total -= p->property;
  }
  assert(count == 0);
  assert(total == 0);
}

static void
buddy_check(void){}

const struct pmm_manager default_pmm_manager = {
    .name = "default_pmm_manager",
    .init = default_init,
    .init_memmap = default_init_memmap,
    .alloc_pages = default_alloc_pages,
    .free_pages = default_free_pages,
    .nr_free_pages = default_nr_free_pages,
    .check = default_check,
};
//const struct pmm_manager default_pmm_manager = {
//    .name = "buddy_pmm_manager",
//    .init = default_init,
//    .init_memmap = buddy_init_memmap,
//    .alloc_pages = buddy_alloc_pages,
//    .free_pages = buddy_free_pages,
//    .nr_free_pages = default_nr_free_pages,
//    .check = buddy_check,
//};

