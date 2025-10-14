#include "types.h"
#include "defs.h"
#include "riscv.h"

// start() jumps here in supervisor mode on all CPUs.
void main() {
  consoleinit();
  printfinit();
  printf("\n");
  printf("kernel is booting\n");
  printf("\n");
  kinit(); // physical page allocator
  panic("main");
}