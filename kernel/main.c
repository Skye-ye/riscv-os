#include "types.h"
#include "defs.h"
#include "riscv.h"

// start() jumps here in supervisor mode on all CPUs.
void main() {
  consoleinit();
  printfinit();
  printf("Hello, world!\n");
  long long id = 2023302111369;
  printf("My ID is %lld\n", id);
  panic("main");
}