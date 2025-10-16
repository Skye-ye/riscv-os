#ifndef DEFS_H
#define DEFS_H

#include "riscv.h"
#include "proc.h"
#include "trap.h"

struct buf;
struct context;
struct file;
struct inode;
struct pipe;
struct proc;
struct spinlock;
struct sleeplock;
struct stat;
struct superblock;

// console.c
void consoleinit(void);
void consoleintr(int);
void consputc(int);

// kalloc.c
void *kalloc(void);
void kfree(void *);
void kinit(void);

// plic.c
void plicinit(void);
void plicinithart(void);
int plic_claim(void);
void plic_complete(int);

// printf.c
int printf(char *, ...) __attribute__((format(printf, 1, 2)));
void panic(char *) __attribute__((noreturn));
void printfinit(void);

// proc.c
int cpuid(void);
void kexit(int);
int kfork(void);
int growproc(int);
void proc_mapstacks(pagetable_t);
pagetable_t proc_pagetable(struct proc *);
void proc_freepagetable(pagetable_t, uint64);
int kkill(int);
int killed(struct proc *);
void setkilled(struct proc *);
struct cpu *mycpu(void);
struct proc *myproc();
void procinit(void);
void scheduler(void) __attribute__((noreturn));
void sched(void);
void sleep(void *, struct spinlock *);
void userinit(void);
int kwait(uint64);
void wakeup(void *);
void yield(void);
int either_copyout(int user_dst, uint64 dst, void *src, uint64 len);
int either_copyin(void *dst, int user_src, uint64 src, uint64 len);
void procdump(void);

// spinlock.c
void acquire(struct spinlock *);
int holding(struct spinlock *);
void initlock(struct spinlock *, char *);
void release(struct spinlock *);
void push_off(void);
void pop_off(void);

// string.c
int memcmp(const void *, const void *, uint);
void *memmove(void *, const void *, uint);
void *memset(void *, int, uint);
char *safestrcpy(char *, const char *, int);
int strlen(const char *);
int strncmp(const char *, const char *, uint);
char *strncpy(char *, const char *, int);

// swtch.S
void swtch(struct context *, struct context *);

// trap.c
extern uint ticks;
extern struct spinlock tickslock;
void trapinit(void);
void trapinithart(void);
void prepare_return(void);
int register_interrupt(int, interrupt_handler_t, void *, char *);
void unregister_interrupt(int, interrupt_handler_t, void *);
void enable_interrupt(int);
void disable_interrupt(int);
void set_irq_priority(int, int);
int get_irq_priority(int);
void enter_interrupt(void);
void exit_interrupt(void);
int in_interrupt(void);
int interrupt_depth(void);
void print_irq_stats(void);
uint64 get_irq_count(int);

// uart.c
void uartinit(void);
int uartintr(void *);
void uartwrite(char[], int);
void uartputc_sync(int);
int uartgetc(void);

// vm.c
void kvminit(void);
void kvminithart(void);
void kvmmap(pagetable_t, uint64, uint64, uint64, int);
int mappages(pagetable_t, uint64, uint64, uint64, int);
pagetable_t uvmcreate(void);
uint64 uvmalloc(pagetable_t, uint64, uint64, int);
uint64 uvmdealloc(pagetable_t, uint64, uint64);
int uvmcopy(pagetable_t, pagetable_t, uint64);
void uvmfree(pagetable_t, uint64);
void uvmunmap(pagetable_t, uint64, uint64, int);
void uvmclear(pagetable_t, uint64);
pte_t *walk(pagetable_t, uint64, int);
uint64 walkaddr(pagetable_t, uint64);
int copyout(pagetable_t, uint64, char *, uint64);
int copyin(pagetable_t, char *, uint64, uint64);
int copyinstr(pagetable_t, char *, uint64, uint64);
int ismapped(pagetable_t, uint64);
uint64 vmfault(pagetable_t, uint64, int);

// kerneltest.c
void kerneltest(void);

// number of elements in fixed-size array
#define NELEM(x) (sizeof(x) / sizeof((x)[0]))

#endif // DEFS_H