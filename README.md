# fractal-comparch-demos
Comp Arch demos
------------------------------

Memory Latency & Locality Demos
A collection of C benchmarks designed to demonstrate some properties of the memory system. 

1. Cache Hierarchy Latency (The "Cliffs")
This demo illustrates the increasing time it takes for a CPU to fetch data as it moves from L1 cache to Main RAM.

* The Technique: Uses pointer-chasing in a randomized cycle. By jumping to unpredictable memory addresses, we try to defeat the hardware prefetcher and force the CPU to wait for the full round-trip latency of the memory controller.
* What it shows: As the buffer size exceeds the physical capacity of L1, L2, and L3 caches, you will see sharp "cliffs" where average access time spikes from ~1ns to over 100ns.

2. Data Layout Efficiency (AoS vs. SoA)
This demo compares the "Object-Oriented" Array of Structures (AoS) against the "Data-Oriented" Structure of Arrays (SoA).
* The Technique: We simulate updating a single property (e.g., y-position) for 10 million entities.
   * AoS: Entities are stored as 64-byte objects. Updating y requires fetching a full cache line but using only 4 bytes of it.
   * SoA: All y values are packed contiguously. One fetch grabs 16 values at once.
* What it shows: Spatial Locality. Packing relevant data together reduces memory bandwidth waste and allows the compiler to use SIMD (vectorization), typically resulting in a 5x–10x speedup.

3. Matrix Transpose (Tiling & TLB Thrashing)
A dramatic demonstration of how memory access patterns can break the Translation Lookaside Buffer (TLB).

* The Technique: Compares a Naive Transpose (row-to-column) against a Blocked (Tiled) Transpose.
   * Naive: The column-major write jumps across memory pages on every iteration, causing the TLB to "thrash" as it repeatedly fails to find virtual-to-physical address mappings.
   * Blocked: Processes the matrix in small 64x64 tiles that fit entirely within the L1 cache and a few TLB entries.
* What it shows: The massive performance penalty of "strided" memory access. The blocked version usually outperforms the naive version by an order of magnitude despite performing the same number of operations.

Compilation
To prevent the compiler from "fixing" these intentional bottlenecks via loop-interchange or other optimizations, compile with moderate optimization and native architecture flags:

gcc -O2 -march=native -o benchmark_name source_file.c



