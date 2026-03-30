#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include <sched.h> // For CPU affinity (pinning to one core)

#define BILLION 1000000000L
#define STEPS 20000000 // 20 million hops to average out noise

/*
 * THE GOAL: Find the "Memory Wall."
 * We test sizes from 8KB (fits in L1) to 128MB (Main RAM).
 * 
 * ANTI-OPTIMIZATION STRATEGIES USED:
 * 1. Pointer Chasing: Each element is a pointer to the next. The CPU must 
 *    wait for the current load to finish before it knows the next address.
 * 2. Shuffling: Prevents the hardware prefetcher from "guessing" the next address.
 * 3. Volatile: Forces the compiler to actually perform the memory read.
 * 4. CPU Affinity: Pins the program to Core 0 so the OS doesn't move it 
 *    to a "cold" cache on another core mid-test.
 */

void run_test(size_t size_bytes) {
    size_t count = size_bytes / sizeof(void *);
    void **buffer = (void **)malloc(size_bytes);

    if (!buffer) return;

    // 1. INITIALIZE: Create a linear chain of pointers
    for (size_t i = 0; i < count - 1; i++) {
        buffer[i] = &buffer[i + 1];
    }
    buffer[count - 1] = &buffer[0]; // Close the loop

    // 2. SHUFFLE: Randomize the chain to break spatial locality
    // This ensures that buffer[i] points to a random buffer[j]
    // using the Fisher-Yates shuffle logic.
    for (size_t i = 0; i < count; i++) {
        size_t j = i + rand() % (count - i);
        void *temp = buffer[i];
        buffer[i] = buffer[j];
        buffer[j] = temp;
    }

    // 3. WARM-UP: Run the loop for a moment to "wake up" the CPU 
    // and trigger Turbo Boost / High Performance clock states.
    void **ptr = &buffer[0];
    for (int i = 0; i < 1000000; i++) {
        ptr = (void **)*ptr;
    }

    // 4. MEASURE: Perform the pointer chase
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    for (int i = 0; i < STEPS; i++) {
        // The 'volatile' cast prevents the compiler from optimizing 
        // the loop away even if we don't use the 'ptr' value.
        ptr = (void **)(*(volatile void **)ptr);
    }

    clock_gettime(CLOCK_MONOTONIC, &end);

    // 5. OUTPUT: Calculate nanoseconds per individual hop
    double total_ns = (double)(end.tv_sec - start.tv_sec) * BILLION + (end.tv_nsec - start.tv_nsec);
    double latency = total_ns / STEPS;

    if (size_bytes < 1024 * 1024)
        printf("%7zu KB | Latency: %6.2f ns\n", size_bytes / 1024, latency);
    else
        printf("%7zu MB | Latency: %6.2f ns\n", size_bytes / (1024 * 1024), latency);

    free(buffer);
}

int main() {
    // PIN TO CORE 0: Avoids "Context Switching" noise.
    cpu_set_t set;
    CPU_ZERO(&set);
    CPU_SET(0, &set);
    sched_setaffinity(0, sizeof(cpu_set_t), &set);

    srand(time(NULL));

    printf("Memory Latency Discovery (Pointer Chasing)\n");
    printf("------------------------------------------\n");

    // Test a wide range of sizes to find the L1, L2, and L3 boundaries.
    size_t test_sizes[] = {
        16 * 1024,      // 16 KB (Likely L1)
        32 * 1024,      // 32 KB (Likely L1 boundary)
        64 * 1024,      // 64 KB (Likely L1 boundary)
        128 * 1024,     // 128 KB (Likely L2)
        256 * 1024,     // 256 KB (Likely L2)
        512 * 1024,     // 512 KB (Likely L2 boundary)
        1 * 1024 * 1024, // 1 MB (Likely L3)
        2 * 1024 * 1024, // 2 MB (Likely L3)
        4 * 1024 * 1024, // 4 MB (Likely L3)
        8 * 1024 * 1024, // 8 MB (Likely L3 boundary)
        16 * 1024 * 1024, // 16 MB (Likely L3 boundary)
        32 * 1024 * 1024, // 32 MB (Main RAM)
        64 * 1024 * 1024 // 64 MB (Deep Main RAM)
    };

    for (int i = 0; i < 13; i++) {
        run_test(test_sizes[i]);
    }

    return 0;
}

