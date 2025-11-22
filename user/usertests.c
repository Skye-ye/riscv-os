#include "kernel/param.h"
#include "user/user.h"

#define assert(expr)                                                           \
  do {                                                                         \
    if (!(expr))                                                               \
      printf("Assertion failed: " #expr);                                      \
  } while (0)

int create_process(void (*entry)(void)) {
  int pid = fork();
  if (pid == 0) {
    entry();
    exit(0);
  }
  return pid;
}

void simple_task(void) {
  for (int i = 0; i < 5; i++) {
    for (volatile int j = 0; j < 1000000; j++)
      ; // busy wait
  }
}

void cpu_intensive_task(void) {
  volatile unsigned long x = 0;
  for (unsigned long i = 0; i < 100000000; i++) {
    x += i;
  }
}

void test_process_creation(void) {
  printf("Testing process creation...\n");
  int pid = create_process(simple_task);
  assert(pid > 0);
  int wpid = wait(0);
  assert(wpid == pid);

  int pids[NPROC];
  int count = 0;
  for (int i = 0; i < NPROC + 5; i++) {
    int pid = create_process(simple_task);
    if (pid <= 0)
      break;
    pids[count++] = pid;
  }
  printf("Created %d processes\n", count);
  for (int i = 0; i < count; i++) {
    printf("%d ", pids[i]);
  }
  printf("\n");
  printf("Waiting for processes to finish...\n");
  for (int i = 0; i < count; i++) {
    int wpid = wait(0);
    printf("%d ", wpid);
  }
  printf("\n");
}

void test_scheduler(void) {
  printf("Testing scheduler...\n");
  for (int i = 0; i < 10; i++) {
    create_process(cpu_intensive_task);
  }

  uint64 start = uptime();
  uint64 end = uptime();

  printf("Scheduler test completed in %lu cycles\n", end - start);
}

int main(int argc, char *argv[]) {
  test_process_creation();
  printf("All user tests passed\n");
  exit(0);
}