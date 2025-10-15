#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "trap.h"
#include "defs.h"

// start() jumps here in supervisor mode on all CPUs.
void main() {
  trapinit();     // trap vectors
  trapinithart(); // install kernel trap vector
  consoleinit();  // console initialization
  printfinit();   // initialize printf
  printf("\n");
  printf("kernel is booting\n");
  printf("\n");
  kinit();        // physical page allocator
  kvminit();      // create kernel page table
  kvminithart();  // turn on paging
  plicinit();     // set up interrupt controller
  plicinithart(); // ask PLIC for device interrupts
  kerneltest();   // run kernel tests
  panic("main");
}
