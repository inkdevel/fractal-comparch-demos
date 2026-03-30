#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/* 
 * N = 10,000,000 entities.
 * AoS size: 10M * 64 bytes = 640 MB (Exceeds L3 cache)
 * SoA size: 10M * 4 bytes = 40 MB (Fits in many L3 caches)
 */
#define N 10000000  
#define ITER 20     // Repeat the test to get a stable average

// --- SCENARIO 1: ARRAY OF STRUCTURES (AoS) ---
/*
 * This is the classic "Object-Oriented" layout. 
 * Each 'Entity' is a 64-byte object (the size of one CPU cache line).
 */
typedef struct {
    float x;        // 4 bytes
    float y;        // 4 bytes (The only value we want to update)
    float z;        // 4 bytes
    int id;         // 4 bytes
    float padding[12]; // 48 bytes of "dead weight" data (health, name, etc.)
} EntityAoS;

// --- SCENARIO 2: STRUCTURE OF ARRAYS (SoA) ---
/*
 * This is the "Data-Oriented" layout. 
 * Instead of grouping all data for one entity together, we group all 
 * 'y' values for ALL entities together in a single, tight array.
 */
typedef struct {
    float *y;
} EntitySoA;

int main() {
    struct timespec start, end;
    double elapsed;

    // Allocate memory on the heap
    EntityAoS *aos = (EntityAoS *)malloc(N * sizeof(EntityAoS));
    EntitySoA soa;
    soa.y = (float *)malloc(N * sizeof(float));

    if (!aos || !soa.y) {
        printf("Memory allocation failed!\n");
        return 1;
    }

    // --- TEST 1: AoS (Spatial Locality: POOR) ---
    /*
     * WHY THIS IS SLOW:
     * 1. CACHE LINE WASTE: When the CPU wants to update 'y', it fetches a 64-byte 
     *    cache line from RAM. This line contains x, y, z, and 48 bytes of padding.
     * 2. LOW UTILITY: The CPU uses 4 bytes of that line and is forced to throw 
     *    the other 60 bytes away to make room for the next entity.
     * 3. BUS TRAFFIC: To update 10 million floats, the CPU must move 640MB 
     *    of data across the system bus.
     */
    printf("Starting AoS Test (Updating Y only)...\n");
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    for (int it = 0; it < ITER; it++) {
        for (int i = 0; i < N; i++) {
            // Jump 64 bytes forward every iteration
            aos[i].y += 1.1f;
        }
    }
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    printf("AoS Total Time: %.4f seconds\n\n", elapsed);

    // --- TEST 2: SoA (Spatial Locality: PERFECT) ---
    /*
     * WHY THIS IS FAST:
     * 1. MAXIMUM UTILITY: One 64-byte cache line fetch now contains 16 
     *    consecutive 'y' values. The CPU uses every single byte it fetches.
     * 2. REDUCED TRAFFIC: The CPU only needs to move 40MB of data to perform 
     *    the same 10 million updates.
     * 3. SIMD VECTORIZATION: Modern compilers (GCC/Clang) can easily use 
     *    AVX/SSE instructions to update 8 or 16 floats in a SINGLE clock cycle 
     *    because they are sitting side-by-side in memory.
     */
    printf("Starting SoA Test (Updating Y only)...\n");
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    for (int it = 0; it < ITER; it++) {
        for (int i = 0; i < N; i++) {
            // Linear, contiguous access. Prefetcher's dream.
            soa.y[i] += 1.1f;
        }
    }
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    printf("SoA Total Time: %.4f seconds\n", elapsed);

    // Prevent compiler from removing the work via Dead Code Elimination
    // by using a volatile sink.
    volatile float sink = aos[0].y + soa.y[0];

    // Cleanup
    free(aos);
    free(soa.y);

    return 0;
}
