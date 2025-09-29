# Minimal RISC-V OS
A simple bare-metal kernel for RISC-V that runs on QEMU.

## Prerequisites
Install the RISC-V toolchain and QEMU:

### Ubuntu/Debian:
```bash
sudo apt-get install gcc-riscv64-unknown-elf qemu-system-misc
```

### macOS:
```bash
brew tap riscv/riscv
brew install riscv-tools qemu
```

## Quick Start
### Build the kernel:
```bash
make
```

### Run in QEMU:
```bash
make qemu
```
Press Ctrl-A then X to exit QEMU.

### Clean up:
```bash
make clean
``` 

### Debugging
Start QEMU in debug mode:
```bash
make qemu-gdb
```
In another terminal, run GDB:
```bash
riscv64-unknown-elf-gdb kernel/kernel
```

## Project Structure
```
kernel/
  ├── types.h      # Basic type definitions
  ├── defs.h       # Function declarations
  ├── riscv.h      # RISC-V specific definitions
  ├── memlayout.h  # Memory layout constants
  ├── entry.S      # Boot entry point (assembly)
  ├── start.c      # Startup initialization
  ├── main.c       # Main kernel code
  ├── uart.c       # Serial console driver
  ├── console.c    # Console I/O (input/output handling)
  ├── printf.c     # Formatted output (printf, panic)
  └── kernel.ld    # Linker script
```