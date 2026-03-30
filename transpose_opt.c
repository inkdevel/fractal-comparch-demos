#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/* 
 * SIZE = 8192 floats per side. 
 * Matrix occupies 8192 * 8192 * 4 bytes = 256 MB.
 * This is large enough to exceed L3 cache (usually 16-64MB) and 
 * force the CPU to deal with main RAM and TLB limits.
 */
#define SIZE 8192   
#define BLOCK 64    // A 64x64 block of floats is 16KB, fits easily in L1 Cache.

int main() {
    // Allocate two 256MB matrices in contiguous memory.
    float *A = (float *)malloc(SIZE * SIZE * sizeof(float));
    float *B = (float *)malloc(SIZE * SIZE * sizeof(float));

    if (!A || !B) {
        printf("Memory allocation failed!\n");
        return 1;
    }

    struct timespec start, end;
    double elapsed;

    // --- TEST 1: NAIVE TRANSPOSE (Spatial Locality: DISASTROUS) ---
    /* 
     * WHY IT IS SLOW:
     * 1. THE STRIDE: B[j * SIZE + i] jumps forward by 32,768 bytes every time 'j' increments.
     * 2. CACHE WASTE: The CPU fetches a 64-byte line for B, writes ONE float (4 bytes), 
     *    and then jumps so far away that the rest of the 60 bytes in that cache line 
     *    are evicted before they can be used.
     * 3. TLB THRASHING: Most CPUs can only keep track of ~64-512 memory "pages" (4KB each) 
     *    at once in the TLB. Because the stride is 32KB, every single 'j' iteration 
     *    lands on a DIFFERENT memory page. The CPU spends more time looking up 
     *    WHERE the memory is than actually writing to it.
     */
    printf("Starting Naive Transpose...\n");
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            // Read is sequential (Good), Write is a massive 32KB jump (Bad)
            B[j * SIZE + i] = A[i * SIZE + j];
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    printf("Naive Transpose Time:    %.4f seconds\n\n", elapsed);


    // --- TEST 2: BLOCKED TRANSPOSE (Spatial Locality: OPTIMIZED) ---
    /* 
     * WHY IT IS FAST:
     * 1. TEMPORAL & SPATIAL LOCALITY: We break the 8192x8192 matrix into 64x64 "tiles."
     * 2. STAYING "LOCAL": While inside the 'ii' and 'jj' loops, the code only 
     *    touches memory within a 16KB range. This fits entirely in the L1 Data Cache.
     * 3. TLB FRIENDLY: We stay on the same few memory pages for 64 iterations 
     *    before moving to the next page. This prevents the TLB from "thrashing" 
     *    (repeatedly clearing and reloading page addresses).
     * 4. HARDWARE PREFETCH: The CPU's prefetcher can easily predict the small 
     *    jumps within the block and keep the pipeline full.
     */
    printf("Starting Blocked Transpose (Block Size: %d)...\n", BLOCK);
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    // Outer loops move between "tiles"
    for (int i = 0; i < SIZE; i += BLOCK) {
        for (int j = 0; j < SIZE; j += BLOCK) {
            
            // Inner loops perform the transpose within the 64x64 tile
            for (int ii = i; ii < i + BLOCK; ii++) {
                for (int jj = j; jj < j + BLOCK; jj++) {
                    // Both the read from A and the write to B stay within 
                    // the current "hot" cache lines and TLB entries.
                    B[jj * SIZE + ii] = A[ii * SIZE + jj];
                }
            }
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    printf("Blocked Transpose Time:  %.4f seconds\n", elapsed);

    // Clean up
    free(A);
    free(B);
    return 0;
}
