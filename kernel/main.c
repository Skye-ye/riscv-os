#include "defs.h"

// start() jumps here in supervisor mode on all CPUs.
void main() {
  trapinit();    // trap vectors
  consoleinit(); // console initialization
  printfinit();  // initialize printf
  printf("\n");
  printf("kernel is booting\n");
  printf("\n");
  kinit();        // physical page allocator
  kvminit();      // create kernel page table
  kvminithart();  // turn on paging
  procinit();     // process table
  trapinithart(); // install kernel trap vector
  plicinit();     // set up interrupt controller
  plicinithart(); // ask PLIC for device interrupts
  userinit();
  scheduler();
}
