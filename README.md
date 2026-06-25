# GTU CSE344 - Systems Programming

This repository contains the projects and homework assignments developed for the **CSE344 - Systems Programming** course at Gebze Technical University (GTU). The projects are implemented in **C** within a **Linux/UNIX** environment, with a strong focus on POSIX standards, operating system architecture, low-level memory management, and concurrency.

## 📚 What We Learned (Overview)

Throughout the semester, the following operating system concepts were theoretically studied and practically implemented:
* Low-Level File I/O & File Systems
* Process Control & Signal Handling
* Inter-Process Communication (IPC: Pipes, FIFOs, Shared Memory, Message Queues)
* Multithreading & POSIX Threads
* Synchronization (Mutex, Semaphores, Condition Variables)
* I/O Multiplexing (select, poll, epoll)
* Client-Server Network Architecture (Socket Programming)

---

## 🗂️ Project Details and System Calls

Each assignment is designed to explore a different layer of the operating system. Below is a breakdown of the core concepts learned and the POSIX API functions utilized in each project:

### HW1: Low-Level File I/O & File Systems
Instead of relying on standard C I/O libraries (`stdio.h`), file operations were performed using direct system calls to the kernel. This project focused on file system navigation, read/write offset management, and I/O buffering.
* **Core Concepts:** File Descriptors, Inodes, I/O Buffering, File Offsets.
* **Key Functions Used:** 
  * `open()`, `read()`, `write()`, `close()`
  * `lseek()` (File positioning/repositioning)
  * `fcntl()` (File control operations and record locking)
  * `dup()`, `dup2()` (Duplicating file descriptors)

### HW2: Process Management & Signals
This project explores how the operating system creates and manages processes. A parent-child process hierarchy was established, and processes were programmed to react to asynchronous interrupts (signals) from the OS.
* **Core Concepts:** Process Creation, Zombie & Orphan Processes, Signal Handling, Process Masking.
* **Key Functions Used:**
  * `fork()` (Creating a new process)
  * `wait()`, `waitpid()` (Waiting for state changes in child processes)
  * `execve()`, `execvp()` (Executing a new program/replacing the process image)
  * `sigaction()`, `kill()`, `sigprocmask()`, `sigsuspend()` (Signal management and masking)

### HW3: Inter-Process Communication (IPC) - Pipes & FIFOs
Enabled secure data exchange between processes operating in isolated memory spaces. A synchronized communication channel was established using Unnamed and Named Pipes.
* **Core Concepts:** Unnamed Pipes, Named Pipes (FIFOs), Blocking vs. Non-Blocking I/O, Atomicity in IPC.
* **Key Functions Used:**
  * `pipe()` (Creating an unnamed pipe)
  * `mkfifo()` (Creating a named pipe / FIFO special file)
  * `unlink()` (Removing the FIFO file from the file system)

### HW4: Multithreading & Synchronization
This project delves into the fundamentals of concurrent programming. Multiple threads sharing the same memory space were created within a single process. Synchronization mechanisms were built to prevent *Race Conditions* and *Deadlocks*.
* **Core Concepts:** POSIX Threads, Critical Sections, Producer-Consumer Problem, Thread Starvation, Race Conditions.
* **Key Functions Used:**
  * `pthread_create()`, `pthread_join()`, `pthread_exit()`
  * `pthread_mutex_init()`, `pthread_mutex_lock()`, `pthread_mutex_unlock()` (Mutex locking mechanisms)
  * `pthread_cond_wait()`, `pthread_cond_signal()` (Condition variables)
  * `sem_open()`, `sem_wait()`, `sem_post()` (POSIX Semaphores)

### HW5: Socket Programming & Network Communication
Established communication across a network using the Client-Server model. This project focused on the foundational aspects of TCP/UDP socket programming and handling basic concurrent connections.
* **Core Concepts:** Network Protocols (TCP/IP), Client-Server Architecture, Port Binding, Network Byte Order.
* **Key Functions Used:**
  * `socket()`, `bind()`, `listen()`, `accept()`, `connect()`
  * `send()`, `recv()`, `sendto()`, `recvfrom()`
  * `htons()`, `htonl()`, `ntohs()`, `inet_pton()` (Byte ordering and IP conversion)

### HW6: Advanced IPC & I/O Multiplexing
Explored advanced communication mechanisms between multiple processes and non-blocking I/O operations. This assignment involved handling multiple I/O streams simultaneously without relying strictly on multithreading.
* **Core Concepts:** Message Queues (POSIX/System V), I/O Multiplexing, Non-blocking I/O, Advanced Asynchronous Event Handling.
* **Key Functions Used:**
  * `mq_open()`, `mq_send()`, `mq_receive()`, `mq_unlink()` (POSIX Message Queues)
  * `msgget()`, `msgsnd()`, `msgrcv()`, `msgctl()` (System V Message Queues)
  * `select()`, `poll()`, or `epoll_create()`, `epoll_wait()` (Multiplexing)

### Final Project: Comprehensive Concurrent Network Server
The capstone project synthesizing all course concepts into a robust, high-performance network application. Designed a multithreaded server capable of handling numerous simultaneous client requests, utilizing a custom Thread-Pool architecture, safe IPC, and strict synchronization protocols to prevent deadlocks under heavy load.
* **Core Concepts:** Daemonization, Thread-Pool Architecture, Graceful Shutdown, Deadlock Avoidance, Logging, Complex Synchronization.
* **Key Functions Used:**
  * Synthesis of Pthreads (`pthread_mutex_t`, `pthread_cond_t`) and Sockets.
  * `daemon()` or custom double-forking for background execution.
  * System logging utilities (e.g., `syslog()`).
  * Signal handlers for safe resource cleanup during termination (`SIGINT`, `SIGTERM`).

---

## 🛠️ Build and Execution Instructions

All projects include a `Makefile` and are compiled using the GCC compiler. Running the executables with Valgrind is highly recommended to ensure there are no memory leaks or invalid memory accesses.

**Example Workflow:**
```bash
# Navigate to the specific homework directory
cd HW4

# Compile the project
make

# Run the executable with required arguments (e.g., 5 threads, buffer size of 10)
./program_name 5 10

# Run with Valgrind for memory leak detection
valgrind --leak-check=full --show-leak-kinds=all ./program_name 5 10

# Clean up object and executable files
make clean