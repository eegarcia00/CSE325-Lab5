// Physical memory allocator, intended to allocate
// memory for user processes, kernel stacks, page table pages,
// and pipe buffers. Allocates 4096-byte pages.

#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "spinlock.h"

void freerange(void *vstart, void *vend);
extern char end[]; // first address after kernel loaded from ELF file
                   // defined by the kernel linker script in kernel.ld

struct run {
  struct run *next;
};

/* Lab-5 Keaton Jones and & Erik Garcia */
struct {
  struct spinlock lock;
  int use_lock;
  struct run *freelist;
  uint freePages;    //A variable that stores the number of free pages
  uint pageRefCount[PHYSTOP >> 12]; //PHYSTOP defined in memlayout.h, top physical memory 0xE000000
                                    //PGSHIFT define in mmu.h, log2(PGSIZE), equals 12
} kmem;

// Initialization happens in two phases.
// 1. main() calls kinit1() while still using entrypgdir to place just
// the pages mapped by entrypgdir on free list.
// 2. main() calls kinit2() with the rest of the physical pages
// after installing a full page table that maps them on all cores.
void
kinit1(void *vstart, void *vend)
{
  initlock(&kmem.lock, "kmem");
  kmem.use_lock = 0;
  kmem.freePages = 0;      //Initialize to 0
  freerange(vstart, vend);
}

void
kinit2(void *vstart, void *vend)
{
  freerange(vstart, vend);
  kmem.use_lock = 1;
}

/* Lab-5 Keaton Jones and & Erik Garcia */
void
freerange(void *vstart, void *vend)
{
  char *p;
  p = (char*)PGROUNDUP((uint)vstart);
  for(; p + PGSIZE <= (char*)vend; p += PGSIZE) {
    kmem.pageRefCount[V2P(p) >> 12] = 0;  //Initialize the ref count to 0
    kfree(p);
  }
}

/* Lab-5 Keaton Jones and & Erik Garcia */
//PAGEBREAK: 21
// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(char *v)
{
  struct run *r;

  if((uint)v % PGSIZE || v < end || V2P(v) >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  //memset(v, 1, PGSIZE);

  if(kmem.use_lock)
    acquire(&kmem.lock);
  r = (struct run*)v;
   
  if(kmem.pageRefCount[V2P(v) >> 12] > 0)  //Decrement the ref count of a pg when it becomes free
  --kmem.pageRefCount[V2P(v) >> 12];

  if(kmem.pageRefCount[V2P(v) >> 12] ==0) { //Free the pg only if there are no references to the pg
    //Catch dangling references
    memset(v, 1, PGSIZE);
    kmem.freePages++;
    r->next = kmem.freelist;
    kmem.freelist = r;
  }

  if(kmem.use_lock)
    release(&kmem.lock);
}

/* Lab-5 Keaton Jones and & Erik Garcia */
// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
char*
kalloc(void)
{
  struct run *r;

  if(kmem.use_lock)
    acquire(&kmem.lock);
  r = kmem.freelist;
  if(r) {
    kmem.freelist = r->next;
    kmem.freePages--;         //Decrement the number of fre pgs by 1 on pg allocation
    kmem.pageRefCount[V2P((char*)r) >> 12] = 1; //Reference count of a pg is set to 1 when allocated
  }

  if(kmem.use_lock)
    release(&kmem.lock);
  return (char*)r;
}


/* Lab-5 Keaton Jones and & Erik Garcia */
uint
numFreePgs(void) 
{
  acquire(&kmem.lock);
  uint freePages= kmem.freePages;
  release(&kmem.lock);
  return freePages;
}


/* Lab-5 Keaton Jones and & Erik Garcia */
//Decrement Reference Count
void
decrementRefCount(uint pa)
{
  ////PHYSTOP defined in memlayout.h, top physical memory 0xE000000
  if(pa < (uint)V2P(end) || pa >= PHYSTOP)
    panic("decrementRefCount");

  acquire(&kmem.lock);
  --kmem.pageRefCount[pa >> 12];
  release(&kmem.lock);
}


/* Lab-5 Keaton Jones and & Erik Garcia */
//Increment Reference Count
void
incrementRefCount(uint pa)
{
   if(pa < (uint)V2P(end) || pa >= PHYSTOP)
    panic("incrementRefCount");

  acquire(&kmem.lock);
  ++kmem.pageRefCount[pa >> 12];
  release(&kmem.lock);
}


/* Lab-5 Keaton Jones and & Erik Garcia */
//Get Reference Count
uint
getRefCount(uint pa)
{
  if(pa < (uint)V2P(end) || pa >= PHYSTOP)
    panic("getRefCount");

  uint count;

  acquire(&kmem.lock);
  count = kmem.pageRefCount[pa >> 12];
  release(&kmem.lock);

  return count;
}
