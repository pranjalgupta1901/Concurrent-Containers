# Final Project: Containers 

# Summary:
- This project contains making of different containers or data structure(stack and queue) which can be accessed concurrently by multiple threads. 
- This containers(Treiber Stack and Michael and Scott Queue) should follow linearization and should be lock free.
- There are methods to reduce contention by using methods such as Elimination(for stacks) and Flat Combining(for stack and queue).
- This project also includes making of Single Global lock stack and queue which includes lock for the synchronization.  
- The concurrent containers are evaulauted based on number of threads pushing the data of a array parallely and popping as well.
- The perf too is used to used to evaluate the performace of each concurrent containers.  

The test cases are designed to evaluate three specific aspects of the system after all push and pop operations have been completed, and all threads have been joined. These aspects are:

### 1. Container Emptiness Check
After all push and pop operations, the test verifies if the container is empty. The test checks whether the container contains any elements or not. If the implementation is correct, the container should be empty after all operations, assuming the total number of pushes equals the total number of pops.

### 2. Sum Consistency Check
This test ensures that the sum of the elements in the pop array is equal to the sum of the elements in the push array. Additionally, it verifies that this sum matches the expected value, which is the sum of the input array multiplied by `(num_threads / 2)`. This check ensures that the values pushed and popped are consistent and that no data was lost or altered during the operations.

### 3. Push and Pop Count Equality
The third test checks whether the total number of push operations is equal to the total number of pop operations. If the implementation is correct, the number of elements pushed should match the number of elements popped, ensuring that no operations were skipped or duplicated.

### In summary, these tests check:
- If the container is empty after all operations.
- If the sum of popped values matches the expected sum based on the input data and the number of threads.
- If the number of push operations equals the number of pop operations.


Note: For extra credit, The flat combining as well as Spurious wake up is implemented and performed.

The code organization includes:
- `mysort.cpp`: This C++ program implements a parses the inputs commands and calls the particular container test function with the given number of threads and the input data.
- `Treiber_Stack.cpp`: This C++ program implements linearized and lock free stack called as Treiber Stack and also contains the test functions. 
- `M_and_S.cpp`: This C++ program implements linearized and lock free queue called as Michael and Scott Queue and also contains the test functions.
- `SGL.cpp`: This C++ program implements single global lock based stack and queue and also contains the test functions.
- `elimination.cpp`: This C++ program implements the Treiber Stack and SGL stack in such a way that reduces contention and also contains the test functions.
- `flat_combining.cpp`: This C++ program implements the Treiber Stack and SGL stack in such a way that reduces contention and also contains the test functions.
- `spurious_wakeup.cpp`: This C++ file contains a utility to handle spurious wakeups in condition variables and test code to demonstrate and validate its behavior in multithreaded scenarios.
- `WRITEUP.md`: Brief description of the organization, performance, files description, execution, and information related to error conditions.
- `Makefile`: Script which compiles the C++ (mysort.cpp) file.
- `README.md`: Brief description of what the assignment is all about.

# Description of Files Submitted 
## mysort.cpp
### Features
- Read integers from an input file
- Puts the data in the stack or queue is LIFO or FIFO sequence respectively using specific test functions.
- Command-line argument parsing for flexible usage
- Multi-threading support for improved performance
- Uses exactly the same number of threads in parallel as specified in the arguments(if provided). In detail, the number of threads should be provided in even number as half the number of threads given are used for push/enqueue and half for pop/dequeue.

## Treiber_stack.cpp
### Features
- Contains the push and pop functions of Treiber Stack which is lock free and linearizable.
- Contains the test fucntions which runs the threads all in parallel and uses these push and pop instructions and test other semantics of the stack.

## M_and_S_queue.cpp
### Features
- Contains the enqueue and dequeue functions of Michael and Scott Queue which is lock free and linearizable.
- Contains the test fucntions which runs the threads all in parallel and uses these enqueue and dequeue instructions and test other semantics of the stack.

## SGL.cpp
### Features
- Contains the enqueue/push and dequeue/pop functions of queue and stack which uses a single global lock.
- Contains the test fucntions which runs the threads all in parallel and uses these enqueue/push and dequeue/pop instructions and test other semantics of the queue/stack.

## Elimination.cpp
### Features
- Contains the push and pop functions of Treiber and SGL stack.
- Contains the test functions which runs the threads all in parallel and uses these enqueue/push and dequeue/pop instructions and test other semantics of the queue/stack.
- The implementation of this method reduces contention and therefore increases the efficiency.

## flat_combining.cpp
### Features
- Contains the enqueue/push and dequeue/pop functions which uses lock a global lock and a condition variable.
- Contains the test functions which runs the threads all in parallel and uses these enqueue/push and dequeue/pop instructions and test other semantics of the queue/stack.
- The implementation of this method reduces contention and therefore increases the efficiency.

## spurious_wakeup.cpp
- Implementation to manage spurious wakeups in condition variables using a while loop to re-check conditions after waking.
- Contains test code with multiple threads to demonstrate correct synchronization and behavior during notifications.

### Usage
Compilation can be done using the Makefile by running on command-line:
```
make
```
Run the program with the following command-line options:
```
mysort [--name] [-i source.txt] [-t NUM_THREADS] [-c=<container(treiber, m_and_s, treiber_eli, sgl_stack, sgl_queue, sgl_eli, fc_stack, fc_queue)>] 
```

### Command-line Options
- `--input` or `-i`: Specify the input file containing integers to sort (required)
- `--container` or `-c`: Specify which container(treiber, m_and_s, treiber_eli, sgl_stack, sgl_queue, sgl_eli, fc_stack, fc_queue) should be used.
- `--name` or `-x`: Print the author's name and exit (optional)
- `--num_threads` or `-t`: Specify the number of threads to use (optional, default is 4)

### Examples
Performing Stack or queue operations 
```
./mysort -i numbers.txt  -c stack/queue -t 4
```
Printing the name:
```
./mysort --name -i numbers.txt  -c stack/queue -t 4
```

### Input File Format
The input file should contain integers, one per line.

### Output
The output will be the form of sucess or fail and will also show the number of Enqueue/Push and Dequeue/pop operations. For example
```
Expected total operations: 300
Actual push count: 300
Actual pop count: 300
```

### Requirements
- All required command-line options should be provided for successful execution.
- **Note**: If any command-line options are incorrect or missing, the program will terminate with an error message.

## WRITEUP.md
- This document contains a brief description of the assignment, providing information about the code structure, execution instructions, and potential error conditions.
- It helps users understand how the code is organized, how to run it, what input it expects, and what output it produces.

## Makefile
- This file helps to compile the C++ code using the g++ compiler.
- Users can compile the C++ code by running the command:
```
make
```
## Code Performance
- `perf` which is a Linux Profiler tool is used to evaluate the performace of both counter and mysort as it givs detialed analysis with almost zero overhead as it uses hardware counters.

### Perf Performance evaluation for containers

- This tests results are derived using perf and the following command
```
perf stat -e task-clock,cache-references,cache-misses,branch-instructions,branch-misses,page-faults ./mysort -i test.txt -c container -t 10
```

- The test file contains 100 values and therefore total pop and push will be 500 each as 5 threads will be doing push and pop each concurrently.

| Container    | Runtime (s) | L1 cache hit (%) | Branch Pred Hit Rate (%) | Page Fault Count (#)|
|--------------|-------------|------------------|--------------------------|---------------------|
| FC_QUEUE | 0.01596 s | 65.52 % | 98.29 % | 564 |
| M_and_S_QUEUE | 0.01350 s | 68.74 % | 96.43 % | 181 |
| SGL_QUEUE | 0.01498 s | 62.74 % | 96.39 % | 172 |

- M_and_S_QUEUE is the best performer overall due to its balance of high cache hit rate, low page faults, and efficient algorithmic design.
- FC_QUEUE suffers in runtime and page faults despite excellent branch prediction, likely due to higher memory and coordination overhead.
- SGL_QUEUE is a middle performer, with acceptable trade-offs but slightly lower cache efficiency than M_and_S_QUEUE.

| Container    | Runtime (s) | L1 cache hit (%) | Branch Pred Hit Rate (%) | Page Fault Count (#)|
|--------------|-------------|------------------|--------------------------|---------------------|
| TREIBER_STACK | 0.01405 s | 65.92% % | 96.83 % | 178 |
| FC_STACK | 0.01860 s | 63.35 % | 98.38 % | 563 |
| SGL_STACK | 0.01546 s | 59.28 % | 96.21 % | 172  |
| FC_STACK | 0.01547 s | 68.46 % | 98.31 % | 565 |
| TREIBER_STACK_ELI | 0.01426 s | 67.47 % | 96.82 % | 182 |
| SGL_ELI | 0.01498 s | 66.07 % | 96.90 % | 179 |

- TREIBER_STACK and TREIBER_STACK_ELI excel in runtime and efficient memory usage, making them the best overall performers. The elimination method (ELI) provides marginal improvements in cache hit rate but adds minor overhead to runtime.
- FC_STACK performs poorly without elimination due to coordination overhead but benefits from elimination when contention increases (higher L1 cache hit rate).
- SGL_STACK and SGL_ELI are middle performers, with locking mechanisms contributing to weaker cache and branch prediction performance.

## Spurious Wake up tests results:
for thread = 4,
```
Thread 0 is continuing as the condition is met.
Thread 1 is continuing as the condition is met.
Thread 3 is continuing as the condition is met.
Thread 2 is continuing as the condition is met.
```
Another Iteration
```
Thread 2 is continuing as the condition is met.
Thread 0 is continuing as the condition is met.
Thread 1 is continuing as the condition is met.
Thread 3 is continuing as the condition is met.
```
- The output shows that all threads resumed correctly after the condition was met, even with potential spurious wakeups, as the while loop ensures condition rechecks. 
- The non-sequential order arises from the thread scheduler's nondeterministic wakeup behavior.

# Extant Bugs
- There are memory leaking issues and ABA problem is not solved.