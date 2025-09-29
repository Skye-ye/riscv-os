# Lab 2 实验报告 - Printf 与 Console 实现

## 一、实验目标

在 Lab 1 最小内核的基础上，实现以下功能：
1. 格式化输出函数 `printf()`
2. 控制台抽象层 `console.c`
3. 特殊字符处理（退格、清屏等）
4. 系统崩溃处理函数 `panic()`

## 二、系统设计

### 2.1 架构设计

#### 新增模块层次结构

```
应用层 (main.c)
    ↓ printf(), panic()
格式化输出层 (printf.c)
    ↓ consputc()
控制台抽象层 (console.c)
    ↓ uartputc()
硬件驱动层 (uart.c)
    ↓
硬件层 (UART 16550a)
```

#### 模块职责划分

| 模块 | 文件 | 主要功能 | 对外接口 |
|------|------|----------|----------|
| **格式化输出** | `printf.c` | 格式化字符串、数字转换 | `printf()`, `panic()` |
| **控制台抽象** | `console.c` | 字符处理、特殊控制 | `consputc()`, `clear_screen()` |
| **UART 驱动** | `uart.c` | 硬件寄存器操作 | `uartputc()`, `uartinit()` |

### 2.2 关键数据结构

#### 2.2.1 变参处理 (stdarg.h)

```c
typedef __builtin_va_list va_list;
#define va_start(ap, last) __builtin_va_start(ap, last)
#define va_arg(ap, type) __builtin_va_arg(ap, type)
#define va_end(ap) __builtin_va_end(ap)
```

这些宏利用 GCC 内建功能实现变参函数：
- `va_list`: 变参列表类型
- `va_start`: 初始化变参列表
- `va_arg`: 获取下一个参数
- `va_end`: 清理变参列表

#### 2.2.2 Panic 状态变量

```c
volatile int panicking = 0;  // 正在打印 panic 信息
volatile int panicked = 0;   // 已进入 panic 状态，永久自旋
```

使用 `volatile` 确保：
- 编译器不会优化掉对这些变量的访问
- 多核环境下其他核心能看到状态变化
- 防止编译器重排序相关代码

### 2.3 核心算法设计

#### 2.3.1 整数转字符串算法

```c
static void printint(long long xx, int base, int sign) {
  char buf[20];  // 最多 20 位数字（64位整数）
  int i = 0;
  unsigned long long x;
  
  // 处理符号
  if (sign && (sign = (xx < 0)))
    x = -xx;
  else
    x = xx;
  
  // 逐位转换（从低位到高位）
  do {
    buf[i++] = digits[x % base];
  } while ((x /= base) != 0);
  
  // 添加负号
  if (sign)
    buf[i++] = '-';
  
  // 反向输出（从高位到低位）
  while (--i >= 0)
    consputc(buf[i]);
}
```

**算法特点:**
- 时间复杂度: O(log_base(n))
- 空间复杂度: O(log_base(n))
- 支持任意进制（2-16）
- 支持有符号/无符号数

#### 2.3.2 格式化字符串解析

```c
int printf(char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  
  for (i = 0; (cx = fmt[i] & 0xff) != 0; i++) {
    if (cx != '%') {
      consputc(cx);  // 普通字符直接输出
      continue;
    }
    
    // 解析格式说明符
    i++;
    c0 = fmt[i + 0] & 0xff;
    c1 = fmt[i + 1] & 0xff;
    c2 = fmt[i + 2] & 0xff;
    
    // 根据格式说明符处理
    if (c0 == 'd') {
      printint(va_arg(ap, int), 10, 1);
    } else if (c0 == 'l' && c1 == 'l' && c2 == 'd') {
      printint(va_arg(ap, uint64), 10, 1);
      i += 2;
    }
    // ... 其他格式
  }
  
  va_end(ap);
  return 0;
}
```

**支持的格式说明符:**

| 格式 | 类型 | 示例 |
|------|------|------|
| `%d` | int | `printf("%d", 123)` → "123" |
| `%ld` | long | `printf("%ld", 123L)` → "123" |
| `%lld` | long long | `printf("%lld", 123LL)` → "123" |
| `%u` | unsigned int | `printf("%u", 123U)` → "123" |
| `%lu` | unsigned long | `printf("%lu", 123UL)` → "123" |
| `%llu` | unsigned long long | `printf("%llu", 123ULL)` → "123" |
| `%x` | hex (int) | `printf("%x", 255)` → "ff" |
| `%lx` | hex (long) | `printf("%lx", 255L)` → "ff" |
| `%llx` | hex (long long) | `printf("%llx", 255LL)` → "ff" |
| `%p` | pointer | `printf("%p", ptr)` → "0x80000000" |
| `%c` | char | `printf("%c", 'A')` → "A" |
| `%s` | string | `printf("%s", "hello")` → "hello" |
| `%%` | literal % | `printf("%%")` → "%" |

### 2.4 设计决策

#### 决策 1: 为什么需要 console 抽象层？

**问题:** 为什么不直接在 printf 中调用 `uartputc()`？

**方案对比:**

```c
// 方案 A: 直接调用 UART
printf() → uartputc() → 硬件

// 方案 B: 通过 console 抽象
printf() → consputc() → uartputc() → 硬件
```

**选择方案 B 的原因:**

1. **解耦合**: printf 不依赖具体硬件
2. **可扩展**: 未来可支持多个输出设备（VGA、网络等）
3. **特殊处理**: console 层可处理退格、清屏等特殊字符

#### 决策 2: 为什么使用 `volatile` 修饰 panic 变量？

```c
volatile int panicking = 0;
volatile int panicked = 0;
```

**原因:**

1. **防止优化**: 编译器可能将这些变量优化到寄存器中
2. **多核可见性**: 确保一个核心的修改对其他核心立即可见
3. **中断安全**: 中断处理程序可能检查这些变量

**示例 - 无 volatile 的问题:**
```c
// 不加 volatile
int panicked = 0;

void panic(char *s) {
  panicked = 1;
  for(;;) ;  // 编译器可能优化掉 panicked = 1
}

// 其他代码
if (!panicked) {  // 可能读取到旧值
  uart_puts("Still running\n");
}
```

#### 决策 3: 为什么 printint 使用缓冲区反向输出？

**问题:** 数字转字符串为什么要先存到缓冲区再反向输出？

**算法对比:**

```c
// 方案 A: 递归直接输出（不需要缓冲区）
void print_recursive(int x, int base) {
  if (x >= base)
    print_recursive(x / base, base);
  consputc(digits[x % base]);
}

// 方案 B: 缓冲区反向输出（当前实现）
void printint(long long x, int base, int sign) {
  char buf[20];
  // 先存入缓冲区...
  // 再反向输出...
}
```

**选择方案 B 的原因:**

| 对比项 | 方案 A (递归) | 方案 B (缓冲区) |
|--------|--------------|----------------|
| 栈使用 | O(log n) 深度 | O(1) 固定 |
| 性能 | 函数调用开销大 | 循环开销小 |
| 代码复杂度 | 简单 | 中等 |
| 内核安全性 | 栈溢出风险 |  安全 |

## 三、实现过程

### 3.1 实现步骤

#### 步骤 1: 实现基础 printf 框架

**目标:** 创建支持基本格式化的 printf

**实现:**

1. 创建 `printf.c` 文件
2. 实现格式字符串解析循环
3. 添加基本格式说明符 (`%d`, `%s`, `%c`)

**测试代码:**
```c
void main() {
  printf("Hello, %s!\n", "world");
  printf("Number: %d\n", 42);
  printf("Char: %c\n", 'A');
}
```

**预期输出:**
```
Hello, world!
Number: 42
Char: A
```

#### 步骤 2: 实现整数转换函数

**目标:** 支持不同进制和长度的整数

**关键代码:**
```c
static char digits[] = "0123456789abcdef";

static void printint(long long xx, int base, int sign) {
  char buf[20];
  int i = 0;
  unsigned long long x;
  
  if (sign && (sign = (xx < 0)))
    x = -xx;
  else
    x = xx;
  
  do {
    buf[i++] = digits[x % base];
  } while ((x /= base) != 0);
  
  if (sign)
    buf[i++] = '-';
  
  while (--i >= 0)
    consputc(buf[i]);
}
```

**测试:**
```c
printf("Dec: %d\n", 123);      // 十进制
printf("Hex: %x\n", 255);      // 十六进制
printf("Neg: %d\n", -456);     // 负数
printf("Long: %lld\n", 2023302111369LL);  // 长整数
```

#### 步骤 3: 添加指针格式化

**目标:** 实现 `%p` 格式说明符

**实现:**
```c
static void printptr(uint64 x) {
  consputc('0');
  consputc('x');
  for (int i = 0; i < 16; i++, x <<= 4)
    consputc(digits[x >> 60]);
}
```

**测试:**
```c
void *stack = (void *)0x80001000;
printf("Stack pointer: %p\n", stack);
// 输出: Stack pointer: 0x0000000080001000
```

#### 步骤 4: 实现 console 抽象层

**目标:** 创建设备无关的控制台接口

**实现 `console.c`:**
```c
void consputc(int c) {
  if (c == BACKSPACE) {
    uartputc('\b');
    uartputc(' ');
    uartputc('\b');
  } else {
    uartputc(c);
  }
}

void clear_screen(void) {
  printf("\033[2J\033[H");  // ANSI 清屏序列
}

void consoleinit(void) {
  uartinit();
}
```

**ANSI 转义序列说明:**
- `\033[2J`: 清除整个屏幕
- `\033[H`: 移动光标到左上角 (1,1)

#### 步骤 5: 实现 panic 函数

**目标:** 创建系统崩溃处理机制

**实现:**
```c
volatile int panicking = 0;
volatile int panicked = 0;

void panic(char *s) {
  panicking = 1;
  printf("panic: ");
  printf("%s\n", s);
  panicked = 1;
  for (;;)
    ;  // 永久自旋
}
```

**使用示例:**
```c
void main() {
  if (some_error) {
    panic("Critical error occurred");
  }
}
```

#### 步骤 6: 更新 main.c 测试

**综合测试代码:**
```c
void main() {
  consoleinit();
  printfinit();
  
  printf("Hello, world!\n");
  
  long long id = 2023302xxxxx;
  printf("My ID is %lld\n", id);
  
  clear_screen();
  
  panic("main");  // 测试 panic
}
```

## 四、运行演示

![Screeshot](images/output.png)