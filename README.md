# slash: Unix Shell Implementation
**Operating Systems - Project 1**  
**Spring 2025 | Koç University**

## Contributors
- **Sinemis Toktaş**
- **Yamaç Ömür**

## Overview
This repository showcases Project 1 from the Operating Systems course completed at Koç University during Spring 2025. **slash** is a comprehensive Unix-style shell implemented in C, featuring advanced I/O operations, kernel module integration, and modern shell conveniences.

## Key Features

### **Core Functionality**
- **Custom Path Resolution:** Manual PATH searching without `execp()` functions
- **I/O Redirection:** Support for `>`, `>>`, and `<` operators
- **Piping:** Arbitrary-length command chains with `|` operator
- **Shell Scripting:** Execute `.sh` files line by line

### **Advanced Features**
- **Auto-completion:** Tab completion for executables in PATH directories
- **Command History:** Navigate 140 previous commands with arrow keys
- **Beautiful Prompt:** Rich interface showing user, hostname, and directory
- **Built-in Commands:** `exit`, `cd`, `history`, and custom `lsfd`

### **Kernel Module Integration**
- **Custom `lsfd` Command:** Analyze file descriptors for any process
- **Dual Implementation:** User-space fallback + kernel module via `/proc/lsfd`
- **Automatic Management:** Load module on startup, cleanup on exit
- **Advanced Output:** FD number, filename, size, and full path

## Usage

### **Basic Commands**
```bash
# Interactive mode
./shell-skeleton

# Script execution
./shell-skeleton script.sh

# Example session
ˢˡᵃsh ╰┈➤ ls -la | grep txt > files.out
ˢˡᵃsh ╰┈➤ lsfd 1234 fd_info.txt
ˢˡᵃsh ╰┈➤ history
```

### **Build & Run**
```bash
# Build shell
gcc -o shell-skeleton shell-skeleton.c

# Build kernel module
cd module/
make

# Run
./shell-skeleton
```

## Technical Features
- **Process Management:** Sophisticated fork/exec handling for pipe chains
- **Memory Safety:** Proper allocation/deallocation in user and kernel space
- **Terminal Control:** Raw input handling for history and auto-completion
- **Kernel Programming:** Safe navigation of kernel data structures with RCU locking

## Learning Outcomes
This project demonstrates proficiency in:
- **System Programming:** Advanced Unix system calls and process management
- **Kernel Development:** Loadable kernel modules and `/proc` filesystem
- **Operating System Concepts:** Practical implementation of OS principles
- **Memory Management:** Dynamic allocation in both user and kernel space
