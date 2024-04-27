// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"



#define count_num ((PHYSTOP-KERNBASE)/PGSIZE)
#define pa_count(x) ((x-KERNBASE)/PGSIZE)  

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;

struct {
  struct spinlock lock;
  int conunt[count_num];  // 从kernel 代码开始的所页面的conut
// 用实际的pa 表示 一个count
} kcount;

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  initlock(&kcount.lock,"kcount");

//初始化成 1  这样紧接着kfree的时候 都能连进来
  acquire(&kcount.lock);
    for(int i=0;i<count_num;++i)
      kcount.conunt[i]=1;
  release(&kcount.lock);

  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  int ans;
  uint64 p_a=(uint64)pa;
  acquire(&kcount.lock);
  if(kcount.conunt[pa_count(p_a)]<1)
      panic("kfree count");

   ans=--kcount.conunt[pa_count(p_a)];
  release(&kcount.lock);

  if(ans==0)
  { 
  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);
  //修改结构体中的数据
  r = (struct run*)pa;
  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  release(&kmem.lock);
  }

}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r)
    kmem.freelist = r->next;
  release(&kmem.lock);

  if(r)
  {
    memset((char*)r, 5, PGSIZE); // fill with junk
    uint64 pa=(uint64)r;
    acquire(&kcount.lock);
    if(kcount.conunt[pa_count(pa)]!=0)
      panic("kcount compute worng\n");
    ++kcount.conunt[pa_count(pa)];
    release(&kcount.lock);  
  }
    
  return (void*)r;
}



void           
increase_conut(uint64 pa)
{
   acquire(&kcount.lock);
   if(kcount.conunt[pa_count(pa)]<1)
      panic("increase_count\n");
    ++kcount.conunt[pa_count(pa)];
    release(&kcount.lock);
}



int           
decrease_count(uint64 pa)
{
  int ans;
  acquire(&kcount.lock);
   if(kcount.conunt[pa_count(pa)]<1)
      panic("decrease_count\n");
    
    if(kcount.conunt[pa_count(pa)]==1)
      {
        ans=0;
      }
        
      else
      {
        --kcount.conunt[pa_count(pa)];
        ans=1;
      }
    release(&kcount.lock);
    return ans;
}