## AUTHOR AND LICENSE
Author Name  | Email                  |
-------------|------------------------|
Joel Kenna   | joel.f.kenna@gmail.com |

### LICENSE
THIS SOFTWARE IS PROVIDED FOR USE "AS IS" WITH _**NO WARRANTY**_, EXPLICT OR IMPLICT. DO NOT REPRODUCE, FORK, COPY, OR OTHERWISE ALTER THIS CODE WITHOUT THE PERMISSION OF THE AUTHOR.

# Purpose

This application simulates an OS running a variety of process scheduling and memory management (page allocation / de-allocation) strategies. Performance, page swaps, process throughput, and other key statistics are collected, allowing different strategy combinations to be compared.

# Simulation Options

TODO

## Invocation Instructions

TODO

## Supported Memory Management Strategies
- Unlimited memory with no page swapping: 'u'
- Standard page swapping: 'p'
- Virtual memory: 'v'
- Virtual memory, evict pages from most recently scheduled process : 'c'

## Supported scheduling strategies
- First-come first-serve scheduler: TODO
- Round robin scheduler: 'r'
- Schedule shortest job with priority: 'c'
