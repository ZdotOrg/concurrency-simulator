# Concurrency Simulator

A C program that simulates processes as threads and solves the Dining Philosophers problem two ways, using POSIX threads (pthreads) and semaphores. Built for an Operating Systems course.

## Features

- **Thread-based process simulation:** reads process arrival and burst times from a file and runs each process as its own pthread
- **Performance metrics:** turnaround time, waiting time, throughput, speedup, and CPU utilization
- **Dining Philosophers, two solutions:**
  - **Mutex with lock ordering:** each philosopher picks up the lower-numbered fork first, which breaks the circular wait
  - **Semaphores:** a `table` semaphore caps diners at 4, so at least one philosopher can always get both forks
- **Thread-safe logging:** a mutex-protected `safe_print` keeps output from different threads from interleaving mid-line
- **Clean shutdown:** all threads are joined, and mutexes and semaphores are destroyed before exit

## Build and run

```bash
gcc project.c -o project -pthread
./project processes.txt
```

On Windows (MinGW):

```
gcc project.c -o project.exe -pthread
.\project.exe processes.txt
```

If no file is given, the program looks for `processes.txt` in the current directory.

**Platform note:** the code uses POSIX APIs. It runs on Linux, WSL, and MinGW. The semaphore version will not work on macOS, which does not support unnamed semaphores (`sem_init`).

## Input format

One process per line: `pid arrival_time burst_time`. An optional header line is skipped automatically.

```
PID Arrival_Time Burst_Time
1 0 1
2 0 10
3 5 1
4 5 1
5 5 1
```

Up to 10 processes are read.

## Sample output

Process simulation (each process runs as a thread):

```
Loaded 5 processes from file

[Process 1] Started (Burst Time: 1 seconds)
[Process 2] Started (Burst Time: 10 seconds)
[Process 1] Finished
[Process 3] Started (Burst Time: 1 seconds)
...
[Process 2] Finished

All processes completed.
```

Dining Philosophers (mutex version, trimmed):

```
[Philosopher 3] Hungry, waiting for forks 2 and 3...
[Philosopher 3] Picked up fork 2
[Philosopher 3] Picked up fork 3
[Philosopher 3] Eating (meal 1/3)...
[Philosopher 3] Released fork 3
[Philosopher 3] Released fork 2
...
[Philosopher 5] Finished all meals, leaving table.

All philosophers have finished dining.
```

## How deadlock is prevented

**The problem:** five philosophers share five forks, and each needs two to eat. If all five grab their left fork at once, each waits forever for the right one. That is deadlock.

**Mutex version (lock ordering):** every philosopher locks the lower-numbered fork first. Philosopher 5 needs forks 4 and 0, so it locks fork 0 first. It can never hold fork 4 while waiting on fork 0, so the circular wait cannot form.

**Semaphore version (limit diners):** the `table` semaphore starts at 4, so a fifth philosopher must wait until someone finishes. With at most 4 seated and 5 forks available, at least one philosopher can always pick up both forks, eat, and release them.

## Roadmap (planned, not yet implemented)

- Scheduling algorithms: FCFS, SJF, Round Robin (configurable quantum), and Priority, with a side-by-side comparison of average waiting and turnaround time
- Text-based Gantt charts for each algorithm
- A deliberately broken Dining Philosophers mode with a watchdog that detects the deadlock
- Command-line mode selection (`broken`, `mutex`, `semaphore`)
- Makefile

## Tech

C, POSIX Threads (pthreads), POSIX Semaphores, mutexes
