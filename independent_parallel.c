#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// N is set to 100 million to ensure the loop runs long enough for timing
#define N 100000000 
#define ITERATIONS 5

int main() {
    // 1. Setup: Allocate and initialize three large arrays
    double *b = (double *)malloc(N * sizeof(double));
    double *c = (double *)malloc(N * sizeof(double));
    double *d = (double *)malloc(N * sizeof(double));

    for (int i = 0; i < N; i++) {
        b[i] = 1.1f; c[i] = 2.2f; d[i] = 3.3f;
    }

    printf("Starting ILP Demo (N = %d)\n\n", N);

    // --- DEMO 1: DEPENDENT CHAIN ---
    // In this loop, 'a' is a bottleneck. Each addition must wait for 
    // the previous one to complete to get the current value of 'a'.
    // This is a 'Read-After-Write' dependency that limits the CPU's IPC.
    double total_time_dep = 0;
    double final_a_dep = 0;

    for (int run = 0; run < ITERATIONS; run++) {
        double a = 0.0f;
        clock_t start = clock();
        for (int i = 0; i < N; i++) {
            a = a + b[i]; 
            a = a + c[i]; // Waiting for 'a' from above...
            a = a + d[i]; // Waiting for 'a' from above...
        }
        clock_t end = clock();
        total_time_dep += (double)(end - start) / CLOCKS_PER_SEC;
        final_a_dep = a;
    }
    printf("Loop 1 (Dependent Chain) Avg: %f seconds\n", total_time_dep / ITERATIONS);

    // --- DEMO 2: INDEPENDENT OPERATIONS ---
    // Here, we use three accumulators. Since a1, a2, and a3 are 
    // independent, a modern superscalar CPU can perform the additions 
    // for all three in parallel within the same clock cycle.
    double total_time_indep = 0;
    double final_a_indep = 0;

    for (int run = 0; run < ITERATIONS; run++) {
        double a1 = 0.0f, a2 = 0.0f, a3 = 0.0f;
        clock_t start = clock();
        for (int i = 0; i < N; i++) {
            a1 += b[i]; // Independent of a2, a3
            a2 += c[i]; // Independent of a1, a3
            a3 += d[i]; // Independent of a1, a2
        }
        double a = a1 + a2 + a3; // Recombine once at the end
        clock_t end = clock();
        total_time_indep += (double)(end - start) / CLOCKS_PER_SEC;
        final_a_indep = a;
    }
    printf("Loop 2 (Independent Ops) Avg:  %f seconds\n", total_time_indep / ITERATIONS);

    // Final check to prevent compiler from optimizing out "unused" loops
    printf("\nResults Verified: %f vs %f\n", final_a_dep, final_a_indep);

    free(b); free(c); free(d);
    return 0;
}
