// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"




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

struct{

struct spinlock lock;
int count[(PHYSTOP-KERNBASE)/PGSIZE];
} kcount;

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  initlock(&kcount.lock,"kcount");

  acquire(&kcount.lock);
  for(int i=0;i<(PHYSTOP-KERNBASE)/PGSIZE;++i)
      kcount.count[i]=1;
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
  if(kcount.count[PACOUNT(p_a)]<1){
    panic("kfree should >=1\n");
  }
  ans=--kcount.count[PACOUNT(p_a)];
  //  if(PACOUNT(p_a)==1304)
  //   {
  //     printf("kfree %d\n",kcount.count[PACOUNT(p_a)]);
  //   }
  release(&kcount.lock);
    

if(ans==0)
{
  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);
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
    if(kcount.count[PACOUNT(pa)]!=0){
      panic("kalloc should be 1");
    }
    kcount.count[PACOUNT(pa)]=1;
    // if(PACOUNT(pa)==1304)
    // {
    //   printf("kalloc %d\n",kcount.count[PACOUNT(pa)]);
    // }
    release(&kcount.lock);
  }
    
  return (void*)r;
}

void increase_count(uint64 pa)
{
   acquire(&kcount.lock);
    if(kcount.count[PACOUNT(pa)]<1){
      panic("increase");
    }
    ++kcount.count[PACOUNT(pa)];
    // if(PACOUNT(pa)==1304)
    // {
    //   printf("increase %d\n",kcount.count[PACOUNT(pa)]);
    // }
    release(&kcount.lock);
}

int decrease_count(uint64 pa)
{
  int ans;
   acquire(&kcount.lock);
    if(kcount.count[PACOUNT(pa)]<1){
      panic("decrease");
    }
    if(kcount.count[PACOUNT(pa)]==1)
     ans=0;
    else
    {
      ans=1;
      --kcount.count[PACOUNT(pa)];
    }
    //  if(PACOUNT(pa)==1304)
    // {
    //   printf("decrease %d\n",kcount.count[PACOUNT(pa)]);
    // }
    release(&kcount.lock);
    return ans;
}

int count(uint64 pa)
{
  int ans;
  acquire(&kcount.lock);
  ans=kcount.count[PACOUNT(pa)];
  release(&kcount.lock);
  return ans;
}