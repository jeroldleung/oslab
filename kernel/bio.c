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

#define NBUCKET 13

struct bucket{
  struct buf head;
  struct spinlock lock;
};

struct bucket table[NBUCKET];

struct {
  struct spinlock lock;
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  // struct buf head;
} bcache;

void
binit(void)
{
  struct buf *b;

  initlock(&bcache.lock, "bcache");

  // Create linked list of buffers
  for(int i = 0; i < NBUCKET; i++){
    table[i].head.prev = &table[i].head;
    table[i].head.next = &table[i].head;
    initlock(&table[i].lock, "bcache.bucket");
  }
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    b->next = table[0].head.next;
    b->prev = &table[0].head;
    initsleeplock(&b->lock, "buffer");
    table[0].head.next->prev = b;
    table[0].head.next = b;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  int key;

  key = blockno % NBUCKET;

  acquire(&table[key].lock);

  // Is the block already cached?
  for(b = table[key].head.next; b != &table[key].head; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&table[key].lock);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  for(b = table[key].head.prev; b != &table[key].head; b = b->prev){
    if(b->refcnt == 0) {
findfreebuf:
      b->dev = dev;
      b->blockno = blockno;
      b->valid = 0;
      b->refcnt = 1;
      release(&table[key].lock);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // steal free buffer from other buckets
  release(&table[key].lock);
  acquire(&bcache.lock);
  for(int i = 0; i < NBUCKET; i++){
    if(i == key)
      continue;
    acquire(&table[i].lock);
    for(b = table[i].head.prev; b != &table[i].head; b = b->prev){
      if(b->refcnt == 0) {
        acquire(&table[key].lock);
        b->next->prev = b->prev;
        b->prev->next = b->next;
        b->next = table[key].head.next;
        b->prev = &table[key].head;
        table[key].head.next->prev = b;
        table[key].head.next = b;
        release(&table[i].lock);
        release(&bcache.lock);
	goto findfreebuf;
      }
    }
    release(&table[i].lock);
  }

  panic("bget: no buffers");
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
  int key;

  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  key = b->blockno % NBUCKET;

  acquire(&table[key].lock);
  b->refcnt--;
  if (b->refcnt == 0) {
    // no one is waiting for it.
    b->next->prev = b->prev;
    b->prev->next = b->next;
    b->next = table[key].head.next;
    b->prev = &table[key].head;
    table[key].head.next->prev = b;
    table[key].head.next = b;
  }
  
  release(&table[key].lock);
}

void
bpin(struct buf *b) {
  int key = b->blockno % NBUCKET;
  acquire(&table[key].lock);
  b->refcnt++;
  release(&table[key].lock);
}

void
bunpin(struct buf *b) {
  int key = b->blockno % NBUCKET;
  acquire(&table[key].lock);
  b->refcnt--;
  release(&table[key].lock);
}


