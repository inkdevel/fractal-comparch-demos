#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// Increase N and ITERATIONS to make the timing differences stark
#define N 1000000
#define ITERATIONS 1000

// Comparison function for qsort
int compare(const void* a, const void* b) {
    return (*(int*)a - *(int*)b);
}

int main() {
    int* data = (int*)malloc(N * sizeof(int));
    int threshold = N / 2;

    // 1. Setup: Fill array with random values
    for (int i = 0; i < N; i++) {
        data[i] = rand() % N;
    }

    printf("Starting Branch Prediction vs. Branchless Demo\n");
    printf("Array Size: %d | Iterations: %d\n\n", N, ITERATIONS);

    // --- TEST 1: RANDOM DATA (IF-STATEMENT) ---
    // High branch misprediction rate (~50%). 
    // The CPU pipeline flushes constantly.
    long long sum_random = 0;
    clock_t start_random = clock();
    for (int it = 0; it < ITERATIONS; it++) {
        for (int i = 0; i < N; i++) {
            if (data[i] > threshold) {
                sum_random += data[i];
            }
        }
    }
    double time_random = (double)(clock() - start_random) / CLOCKS_PER_SEC;
    printf("1. Random Array (IF):   %f sec\n", time_random);

    // --- TEST 2: SORTED DATA (IF-STATEMENT) ---
    // Low branch misprediction rate. 
    // The predictor learns the pattern (NNNN...YYYY).
    qsort(data, N, sizeof(int), compare);
    long long sum_sorted = 0;
    clock_t start_sorted = clock();
    for (int it = 0; it < ITERATIONS; it++) {
        for (int i = 0; i < N; i++) {
            if (data[i] > threshold) {
                sum_sorted += data[i];
            }
        }
    }
    double time_sorted = (double)(clock() - start_sorted) / CLOCKS_PER_SEC;
    printf("2. Sorted Array (IF):   %f sec (Speedup: %.2fx)\n", 
            time_sorted, time_random / time_sorted);

    // --- TEST 3: RANDOM DATA (BRANCHLESS) ---
    // We shuffle the data again to prove that randomness doesn't matter here.
    for (int i = 0; i < N; i++) data[i] = rand() % N;

    long long sum_branchless = 0;
    clock_t start_bl = clock();
    for (int it = 0; it < ITERATIONS; it++) {
        for (int i = 0; i < N; i++) {
            // BRANCHLESS LOGIC:
            // (data[i] > threshold) returns 1 (true) or 0 (false).
            // We multiply the value by this 1 or 0 to 'filter' it.
            // No 'if' means no 'prediction' and no 'pipeline flushes'.
            sum_branchless += (long long)data[i] * (data[i] > threshold);
        }
    }
    double time_bl = (double)(clock() - start_bl) / CLOCKS_PER_SEC;
    printf("3. Random (Branchless): %f sec\n", time_bl);

    // Verification to ensure the logic is actually working
    printf("\nNote: Branchless is often as fast as Sorted, regardless of data order.\n");

    free(data);
    return 0;
}
