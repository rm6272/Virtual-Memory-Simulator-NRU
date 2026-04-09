# Virtual Memory Simulator (NRU)

## Overview

This project simulates a virtual memory system in C, including page tables, frame allocation, and page replacement using the **NRU (Not Recently Used)** algorithm. It processes a sequence of memory accesses and tracks system behavior such as page faults and memory usage.

## Features

* Page table simulation with **valid, referenced (R), and modified (M) bits**
* Implements **NRU page replacement algorithm**
* Handles **page faults** and frame allocation
* Tracks:

  * Number of reads and writes
  * Total memory accesses
  * Page fault rate
* Periodically resets R bits to simulate OS behavior

## How It Works

* Virtual addresses are translated into **VPN (virtual page numbers)**
* If a page is not in memory, a **page fault** occurs
* A free frame is used if available; otherwise, a victim page is selected using NRU
* Pages are classified into NRU classes based on (R, M) bits and replaced accordingly

## Compilation

```
gcc main.c -o vm
```

## Usage

```
./vm <input_file> <page_size> <reset_interval>
```

* `input_file`: file with memory accesses (hex address + read/write flag)
* `page_size`: page size (e.g., 32, 64, 128)
* `reset_interval`: number of accesses before clearing R bits

## Example

```
./vm input.txt 64 10
```

## Output

* Number of reads and writes
* Page fault percentage
* Final memory state (frame → VPN mapping)

## Technologies

* C
* Operating Systems concepts
* Memory management and page replacement algorithms

## Notes

* Demonstrates low-level systems design and memory management
* NRU approximates LRU using R and M bits
* Designed for understanding OS-level paging behavior
