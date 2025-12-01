#include "kernel/types.h"
#include "user/user.h"

#define PGSIZE 4096
#define TEST_BYTES (8 * 1024 * 1024)
#define MAX_PROBE (120 * 1024 * 1024)
#define PROBE_STEP (4 * 1024 * 1024)
#define NOWRITE_ITERS 64
#define WRITE_ITERS 12

static char *region;
static int region_pages;

static void prime_region(void) {
  for (int i = 0; i < region_pages; i++) {
    region[i * PGSIZE] = (char)i;
  }
}

static void dirty_region(void) {
  for (int i = 0; i < region_pages; i++) {
    region[i * PGSIZE] ^= 1;
  }
}

static int probe_free_bytes(int *capped) {
  int step = PROBE_STEP;
  int total = 0;
  *capped = 0;

  while (1) {
    if (total + step > MAX_PROBE)
      step = MAX_PROBE - total;
    if (step < PGSIZE)
      break;

    char *p = sbrk(step);
    if (p == SBRK_ERROR) {
      if (step == PGSIZE)
        break;
      step /= 2;
      if (step < PGSIZE)
        step = PGSIZE;
      continue;
    }

    p[0] = 1;
    p[step - 1] = 1;
    total += step;

    if (total == MAX_PROBE) {
      *capped = 1;
      break;
    }
  }

  if (total && sbrk(-total) == SBRK_ERROR) {
    printf("cowtest: failed to release probe memory\n");
    exit(1);
  }

  return total;
}

static int measure_free_with_child(int child_writes, int *capped) {
  int fds[2];
  if (pipe(fds) < 0) {
    printf("cowtest: pipe failed\n");
    exit(1);
  }

  int pid = fork();
  if (pid < 0) {
    printf("cowtest: fork failed\n");
    exit(1);
  }
  if (pid == 0) {
    close(fds[1]);
    if (child_writes)
      dirty_region();
    char buf;
    if (read(fds[0], &buf, 1) < 0) {
      printf("cowtest: sync read failed\n");
      exit(1);
    }
    close(fds[0]);
    exit(0);
  }

  close(fds[0]);
  int free_bytes = probe_free_bytes(capped);
  if (write(fds[1], "x", 1) != 1) {
    printf("cowtest: sync write failed\n");
    exit(1);
  }
  close(fds[1]);
  wait(0);
  return free_bytes;
}

static void timed_forks(int iters, int child_writes) {
  uint64 start = uptime();
  int completed = 0;

  for (int i = 0; i < iters; i++) {
    int pid = fork();
    if (pid < 0) {
      printf("cowtest: fork failed after %d iterations\n", i);
      break;
    }
    if (pid == 0) {
      if (child_writes)
        dirty_region();
      exit(0);
    }
    wait(0);
    completed++;
  }

  uint64 elapsed = uptime() - start;
  uint64 avg = completed ? elapsed / completed : 0;
  printf("fork%s x%d: %lu ticks total (~%lu /fork)\n",
         child_writes ? "+write" : "+exit", completed, elapsed, avg);
}

int main(void) {
  region_pages = TEST_BYTES / PGSIZE;
  region = sbrk(TEST_BYTES);
  if (region == SBRK_ERROR) {
    printf("cowtest: failed to reserve %d KB heap\n", TEST_BYTES / 1024);
    exit(1);
  }

  prime_region();

  printf("cowtest: %d KB region (%d pages)\n", TEST_BYTES / 1024, region_pages);

  int capped = 0;
  int baseline_free = probe_free_bytes(&capped);
  printf("baseline free: %d KB%s\n", baseline_free / 1024,
         capped ? " (capped)" : "");

  timed_forks(NOWRITE_ITERS, 0);
  timed_forks(WRITE_ITERS, 1);

  int shared_capped = 0, cow_capped = 0;
  int shared_free = measure_free_with_child(0, &shared_capped);
  int cow_free = measure_free_with_child(1, &cow_capped);

  int shared_delta = baseline_free - shared_free;
  int cow_delta = baseline_free - cow_free;
  if (shared_delta < 0)
    shared_delta = 0;
  if (cow_delta < 0)
    cow_delta = 0;

  printf("free while child shares pages: %d KB%s (drop %d KB)\n",
         shared_free / 1024, shared_capped ? " (capped)" : "",
         shared_delta / 1024);
  printf("free after child writes: %d KB%s (drop %d KB)\n", cow_free / 1024,
         cow_capped ? " (capped)" : "", cow_delta / 1024);

  exit(0);
}
