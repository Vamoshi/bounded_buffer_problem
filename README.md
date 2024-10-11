# Bounded Buffer Problem with Multi-threading in C

This project implements a solution to the **Bounded Buffer Problem** (also known as the Producer-Consumer Problem) using multi-threading, synchronization primitives (mutexes, condition variables, semaphores), and barriers in C.

## Problem Overview

The **Bounded Buffer Problem** is a classic synchronization problem where multiple producer threads produce data and add it to a fixed-size buffer, while multiple consumer threads consume the data from this buffer. The buffer has a limited size, so producers must wait if it's full, and consumers must wait if it's empty. The solution requires coordination to prevent data corruption and race conditions.

## How the Code Operates

### Threads

- **Generator Threads (Producers)**: These threads produce materials (`a`, `b`, `c`) and place them into a shared buffer. There are 3 generator threads in this implementation.
- **Operator Threads (Consumers)**: These threads consume materials from the buffer and create products. They take two consecutive materials from the buffer and combine them to form a product. There are 3 operator threads in this implementation.
- **Timer Thread**: This thread keeps track of the total runtime (`ttr`) and stops the program when the runtime limit is reached. It also prints the statistics of produced materials and products.

### Shared Resources and Synchronization

To manage the shared buffer and synchronize access between threads, the code uses several synchronization primitives:

1. **Mutexes**: Used for locking shared resources such as the buffer and output array, ensuring exclusive access when threads modify these resources.
2. **Semaphores**:
   - `semAvailable`: Controls available space in the buffer.
   - `semOccupied`: Counts how many items are currently in the buffer.
   - `semTools`: Controls access to a limited number of tools needed by operator threads to process materials.
3. **Condition Variables**: Used to signal and wait for changes in the buffer state:
   - `condFull`: Signals when the buffer is full, telling the producers to wait.
   - `condEmpty`: Signals when the buffer is empty, telling the consumers to wait.

### Key Operations

1. **Generator Thread Functionality**:

   - Each generator thread waits until space is available in the buffer and then adds a material (`a`, `b`, or `c`).
   - The threads synchronize using a barrier to ensure they all produce materials simultaneously.
   - They wait when the buffer is full and resume when space becomes available.

2. **Operator Thread Functionality**:

   - Each operator thread waits until there are enough materials in the buffer, then consumes two materials to create a product (e.g., `aa`, `ab`, etc.).
   - If the output array reaches its limit, the operator thread dynamically resizes it to accommodate more products.
   - The operator threads use tools (controlled by semaphores) to process the materials and simulate a manufacturing delay using `usleep()`.

3. **Timer Thread Functionality**:
   - The timer thread runs for a specified duration (`ttr`), incrementing the time and checking if the program should continue running.
   - Once the time limit is reached, it prints the statistics: the number of each material produced, the number of products created, and the final state of the output array.

### Program Flow

- The main function initializes threads, mutexes, semaphores, and other synchronization mechanisms.
- Generator and operator threads are created and started.
- The timer thread runs concurrently to track the program's duration.
- Threads join back after completion, ensuring that all threads finish their work before the program ends.
- Resources such as mutexes, semaphores, and memory allocations are properly cleaned up before the program terminates.

## Compilation and Execution

To compile the program, use the following command:

```bash
gcc -pthread -o bounded_buffer bounded_buffer.c
```

Here’s what the command does:

- **`-pthread`**: Links the POSIX thread library, necessary for using `pthread` functions.
- **`-o bounded_buffer`**: Names the output executable file `bounded_buffer`.

### Running the Program

After compiling, run the program using:

```bash
./bounded_buffer
```

The program will execute, with generator and operator threads interacting with the buffer, while the timer thread tracks and prints the statistics when the time limit is reached.

## What the Code Solves

This program solves the **Bounded Buffer Problem** by:

- Managing a fixed-size buffer where multiple producer and consumer threads operate concurrently.
- Using synchronization techniques like semaphores, mutexes, and condition variables to prevent race conditions and ensure the proper order of execution.
- Providing a dynamic and scalable solution that supports resizing of the output array when necessary.

## Limitations and Considerations

- The program’s complexity could be simplified by reducing the number of synchronization methods (for example, relying solely on semaphores or condition variables where appropriate).
- The code might have performance overhead due to multiple condition broadcasts (`pthread_cond_broadcast`), where using `pthread_cond_signal` might be more efficient.
- The use of dynamic memory allocation for the output array is efficient but should be monitored for potential memory leaks.

---
