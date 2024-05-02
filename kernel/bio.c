// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

struct {
  struct spinlock lock;
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.

} bcache;


struct{
  struct spinlock lock;
  struct buf  head;
} bcache_bucket[13];




void
binit(void)
{

  initlock(&bcache.lock, "bcache");

  // Create linked list of buffers
  // bcache.head.prev = &bcache.head;
  // bcache.head.next = &bcache.head;
  // for(b = bcache.buf; b < bcache.buf+NBUF; b++){
  //   b->next = bcache.head.next;
  //   b->prev = &bcache.head;
  //   initsleeplock(&b->lock, "buffer");
  //   bcache.head.next->prev = b;
  //   bcache.head.next = b;
  //   b->refcnt=0;
  // }

  for(int i=0;i<13;++i)
  {
   char temp[15]="bcahe_bucket";
    temp[12]=i+'0';
    temp[13]=0;
    initlock(&bcache_bucket[i].lock,temp);
    bcache_bucket[i].head.prev=&bcache_bucket[i].head;
    bcache_bucket[i].head.next=&bcache_bucket[i].head;
  }

  struct buf *j=bcache.buf;
  for(int i=0;j<bcache.buf+NBUF;++i,++j)
  {
    initsleeplock(&j->lock,"buffer");
    if(i==13)
    {
       i=0;
    }
    acquire(&bcache_bucket[i].lock);
        j->blockno=i;
        j->refcnt=0;
        j->timestamp=0;
        j->next=bcache_bucket[i].head.next;
        j->prev=&bcache_bucket[i].head;
        bcache_bucket[i].head.next->prev=j;
        bcache_bucket[i].head.next=j;
    release(&bcache_bucket[i].lock);
  }

}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;

  int number=(blockno)%13;
  int flags=0;
  acquire(&bcache_bucket[number].lock);
  for(b=bcache_bucket[number].head.next;b!=&bcache_bucket[number].head;b=b->next)
  {
      if(b->dev==dev&&b->blockno==blockno)
      {
        b->refcnt++;
        flags=1;
        break;
      }
  }
  if(flags==1)
  {
    release(&bcache_bucket[number].lock);
    acquiresleep(&b->lock);
    return b;
  }

  // in own
  uint mintime=0xffffffff;
  struct buf * result=0;
  for(b=bcache_bucket[number].head.next;b!=&bcache_bucket[number].head;b=b->next)
  {
      if(b->refcnt==0&&b->timestamp<mintime)
      {
          result=b;
          mintime=b->timestamp;
      }
  }

  if(result!=0)
  {
    result->dev = dev;
    result->blockno = blockno;
    result->valid = 0;
    result->refcnt = 1;
    release(&bcache_bucket[number].lock);
    acquiresleep(&result->lock);
    return result;
  }

  // find onther 

  acquire(&bcache.lock);
    int othernumber=14;
    uint min_time=0xffffffff;
    struct buf *p=0;

refind:
    p=0;
    min_time=0xffffffff;
  for(int i=0;i<NBUF;++i)
  {
      if(bcache.buf[i].refcnt==0&&bcache.buf[i].timestamp<min_time)
      {   
          othernumber=(bcache.buf[i].blockno)%13;
          min_time=bcache.buf[i].timestamp;
          p=&bcache.buf[i];
      }
  }
  if(p!=0)
  {
  acquire(&bcache_bucket[othernumber].lock);

  if(p->refcnt!=0)
  {
    release(&bcache_bucket[othernumber].lock);
    goto refind;
  }
  else
  {
    p->next->prev=p->prev;
    p->prev->next=p->next;
    p->prev=0;
    p->next=0;
    release(&bcache_bucket[othernumber].lock);

    p->next=bcache_bucket[number].head.next;
    p->prev=&bcache_bucket[number].head;
    bcache_bucket[number].head.next->prev=p;
    bcache_bucket[number].head.next=p;

    p->dev = dev;
    p->blockno = blockno;
    p->valid = 0;
    p->refcnt = 1;
    release(&bcache.lock);
    release(&bcache_bucket[number].lock);
    acquiresleep(&p->lock);
    return p;
  }
  }
  else
  {
    panic("bget: no buffers");
  }

  return 0;
  
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);


  int number=(b->blockno)%13;

  acquire(&bcache_bucket[number].lock);

    if(--b->refcnt==0)
    {
      b->timestamp=ticks;
      release(&bcache_bucket[number].lock);
      return ;
    }
    else
    {
      release(&bcache_bucket[number].lock);
      return ;
    }
    return ;


}

void
bpin(struct buf *b) {
  int i=(b->blockno)%13;
  acquire(&bcache_bucket[i].lock);
  b->refcnt++;
  release(&bcache_bucket[i].lock);
}

void
bunpin(struct buf *b) {
 int i=(b->blockno)%13;
  acquire(&bcache_bucket[i].lock);
  b->refcnt--;
  release(&bcache_bucket[i].lock);
}


