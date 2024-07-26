## AUTHOR AND LICENSE
Author Name  | Email                  |
-------------|------------------------|
Joel Kenna   | joel.f.kenna@gmail.com |

### LICENSE
THIS SOFTWARE IS PROVIDED FOR USE "AS IS" WITH _**NO WARRANTY**_, EXPLICT OR IMPLICT. DO NOT REPRODUCE, FORK, COPY, OR OTHERWISE ALTER THIS CODE WITHOUT THE PERMISSION OF THE AUTHOR.

# Purpose

This tool simulates some of the core behaviour of an operating system - task scheduling and memory management. It is intended as a learning tool for experimenting with how different combinations of memory and scheduling strategies perform against different task sets. 

To support this aim, the tool captures and displays task throughput, turnaround time, and other key statistics, allowing the performance of different strategies to be compared. 

# Running the tool
1. Compile simulator by running ```make``` at root directory
2. Run simulator with args specifying strategies, input, memory page size etc. (see table below).

For example, the following will run a simulation w/ round-robin scheduling and virtual memory based on input from test_data.txt.

```simulator -f test_data.txt -a r -m v -s 4 -q 1```

## Argument Table

|Arg | Description                                                                                              | Type |
|----|----------------------------------------------------------------------------------------------------------|------|
|\-f | Filename of input data                                                                                   |String|
|\-a | Scheduling algorithm                                                                                     |Char  |
|\-m | Memory manager                                                                                           |Char  |
|\-s | Memory page size (if memory management strategy uses pages)                                              |Int   |
|\-q | Quantum (time a process will execute for before scheduler considers allowing another process to execute) |Int   |

## Supported scheduling strategies (possible values for -a arg)

| Strategy | Value |
|----------|-------|
|First-come first-serve scheduler | f |
|Round robin scheduler | r |
|Schedule shortest job with priority | c |

## Supported Memory Management Strategies (possible values for -m arg)

| Strategy | Value |
|----------|-------|
|Unlimited memory with no page swapping | u |
|Standard paged memory with swapping | p |
|Virtual memory | v |
|Virtual memory, preferentially evict pages from most recently scheduled process | c |


