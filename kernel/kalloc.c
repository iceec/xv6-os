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


//jinliashi huoqu j de suo



extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
  int size;
} kmem[NCPU];



void * find_page(int number,int i,int j)
{
       struct run *r;
   
        struct run * p=kmem[j].freelist;
        struct run * start=kmem[j].freelist;
        struct run * temp=0;
        int k=0;
        for(;k<number;++k)
        {   temp=p;
            p=p->next;
        }

        if(kmem[j].size<k)
            panic("no wat in search");
        
        kmem[j].size-=k;
        temp->next=0;   //cut lianxi 
        kmem[j].freelist=p;  //xin de p
      release(&kmem[j].lock);

      acquire(&kmem[i].lock);
        struct run *now_next=kmem[i].freelist;
        kmem[i].freelist=start;
        temp->next=now_next;
        kmem[i].size+=k;

        r=kmem[i].freelist;
        kmem[i].freelist=r->next;
        --kmem[i].size;
      release(&kmem[i].lock);

    return (void *)r;

}

void * search_page(int i)
{
  void * result=0;
  acquire(&kmem[i].lock);
  if(kmem[i].size!=0)
  {
    printf("%d\n",kmem[i].size);
    panic("no way");
  }
  release(&kmem[i].lock);

  int flags=0;

  for(int j=0;j<NCPU;++j)
  {
    if(j==i)
      continue;

    acquire(&kmem[j].lock);
    if(kmem[j].size>=30)
    {
     flags=1;
     result=find_page(30,i,j);
      break;
    }
    else
    {
      release(&kmem[j].lock);
    }
  }

  if(flags==0)
  {
    for(int j=0;j<NCPU;++j)
  {
      if(j==i)
        continue;
      
      acquire(&kmem[j].lock);

      if(kmem[j].size>=1)
      {
        result=find_page(1,i,j);
        break;
      }
      else
        release(&kmem[j].lock);
  }
}

return result;


}











void
kinit()
{
  for (int i = 0; i < NCPU; i++)
  {
    char temp[10]="kmem";
    temp[4]=i+'0';
    temp[5]=0;
    initlock(&kmem[i].lock, temp);

    // may be not need lock
    //set init size is 0
    acquire(&kmem[i].lock);
      kmem[i].size=0;
    release(&kmem[i].lock);
  }

  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(int i=0; p + PGSIZE <= (char*)pa_end; p += PGSIZE,++i)
  {
    if(i==NCPU)
        i=0;

  struct run *r;

  if(((uint64)p % PGSIZE) != 0 || (char*)p < end || (uint64)p >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(p, 1, PGSIZE);

  r = (struct run*)p;

  acquire(&kmem[i].lock);
  r->next = kmem[i].freelist;
  kmem[i].freelist = r;
  ++kmem[i].size;
  release(&kmem[i].lock);
    //kfree(p);  bukeyi l 
  }
    
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

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  push_off();

  int i=cpuid();

  acquire(&kmem[i].lock);
  r->next = kmem[i].freelist;
  kmem[i].freelist = r;
  ++kmem[i].size;
  release(&kmem[i].lock);
  pop_off();
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  push_off();
  int i=cpuid();
  

  acquire(&kmem[i].lock);
  r = kmem[i].freelist;
  if(r)
  {
    kmem[i].freelist = r->next;
    --kmem[i].size;
  }
  release(&kmem[i].lock);

  if(r)
  {
    memset((char*)r, 5, PGSIZE); // fill with junk
    pop_off();
  }
  else
  {
    void *p=0;
    p=search_page(i);
    pop_off();
    if(p)
      memset((char*)p, 5, PGSIZE); 

    return p;
  }
  return (void*)r;
}
