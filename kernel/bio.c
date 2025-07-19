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
  struct buf buf[NBUF]; 
  struct spinlock big_lock;
  struct spinlock lock[NBUCKET];
  struct buf head[NBUCKET];
} bcache;

int hash(uint key) {
  return key % NBUCKET;
}

void
binit(void)
{
  struct buf *b;

  initlock(&bcache.big_lock, "bcache_big_lock");
  // 初始化桶链表 
  for (int i = 0; i < NBUCKET; i++) {
    initlock(&bcache.lock[i], "bcache_bucket");
    bcache.head[i].prev = &bcache.head[i];
    bcache.head[i].next = &bcache.head[i];
  }
  // 初始化buf 
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    b->next = bcache.head[0].next;
    b->prev = &bcache.head[0];
    initsleeplock(&b->lock, "buffer");
    bcache.head[0].next->prev = b;
    bcache.head[0].next = b;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;

  int v = hash(blockno);
  int min_tick = __UINT32_MAX__;
  struct buf* target_buf = 0;

  acquire(&bcache.lock[v]);

  // Is the block already cached?
  // 命中
  for(b = bcache.head[v].next; b != &bcache.head[v]; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.lock[v]);
      acquiresleep(&b->lock);
      return b;
    }
  }
  release(&bcache.lock[v]);

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  acquire(&bcache.big_lock);
  acquire(&bcache.lock[v]);
  for (b = bcache.head[v].next; b != &bcache.head[v]; b = b->next) {
    if (b->dev == dev && b->blockno == blockno) {
      b->refcnt++;
      release(&bcache.lock[v]);
      release(&bcache.big_lock);
      acquiresleep(&b->lock);
      return b;
    }
  }
  for (b = bcache.head[v].next; b != &bcache.head[v]; b = b->next) {
    if (b->refcnt == 0 && (target_buf == 0 || b->tick < min_tick)) {
      min_tick = b->tick;
      target_buf = b;
    }
  }
  
  if (target_buf) {
    target_buf->dev = dev;
    target_buf->blockno = blockno;
    target_buf->refcnt++;
    target_buf->valid = 0;
    release(&bcache.lock[v]);
    release(&bcache.big_lock);
    acquiresleep(&target_buf->lock);
    return target_buf;
  }

  // 从其他桶找空闲块
  for (int i = hash(v + 1); i != v; i = hash(i + 1)) {
    acquire(&bcache.lock[i]);
    for (b = bcache.head[i].next; b != &bcache.head[i]; b = b->next) {
      if (b->refcnt == 0 && (target_buf == 0 || b->tick < min_tick)) {
        min_tick = b->tick;
        target_buf = b;
      }
    }
    if (target_buf) {
      target_buf->dev = dev;
      target_buf->refcnt++;
      target_buf->valid = 0;
      target_buf->blockno = blockno;
      // 从原桶中删除
      target_buf->next->prev = target_buf->prev;
      target_buf->prev->next = target_buf->next;
      release(&bcache.lock[i]);
      //加锁
      target_buf->next = bcache.head[v].next;
      target_buf->prev = &bcache.head[v];
      bcache.head[v].next->prev = target_buf;
      bcache.head[v].next = target_buf;
      release(&bcache.lock[v]);
      release(&bcache.big_lock);
      acquiresleep(&target_buf->lock);
      return target_buf;
    }
    release(&bcache.lock[i]);
  }

  release(&bcache.lock[v]);
  release(&bcache.big_lock);

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
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  int v = hash(b->blockno);
  
  acquire(&bcache.lock[v]);

  b->refcnt--;
  if (b->refcnt == 0) {
    // LRU记录辅助
    b->tick = ticks;
  }
  
  release(&bcache.lock[v]);
}

void
bpin(struct buf *b) {
  int v = hash(b->blockno);
  acquire(&bcache.lock[v]);
  b->refcnt++;
  release(&bcache.lock[v]);
}

void
bunpin(struct buf *b) {
  int v = hash(b->blockno);
  acquire(&bcache.lock[v]);
  b->refcnt--;
  release(&bcache.lock[v]);
}


