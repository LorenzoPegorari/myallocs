/*
 * MIT License
 *
 * Copyright (c) 2026 Lorenzo Pegorari (@LorenzoPegorari)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
/** @file test.c */


// C Standard Library
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "myallocs.h"


/* Manipulate these macros how you like! Their effect is described in the
   `run_memory_data_test()` function! */
#define ALLOC_MAX       10
#define ALLOC_BLOCK_MIN 1
#define ALLOC_BLOCK_MAX 10000
#define REALLOC_MAX     2
#define REALLOC_CYCLES  1000


void rand_unique_vals_in_range(int *array, int n, int m);
void run_memory_data_test(FILE *file1, FILE *file2);


int main(void) {
    FILE  *file1;
    FILE  *file2;

    // Seed with current time.
    srand(time(NULL));

    // Open files
    file1 = fopen("data1.txt", "w");
    if (file1 == NULL) {
        perror("fopen() error");
        exit(EXIT_FAILURE);
    }
    file2 = fopen("data2.txt", "w");
    if (file2 == NULL) {
        perror("fopen() error");
        exit(EXIT_FAILURE);
    }

    run_memory_data_test(file1, file2);

    // Close files.
    fclose(file1);
    fclose(file2);

    exit(EXIT_SUCCESS);
}


/**
 * Given an `array`, fills it with `m` random UNIQUE values between 0 and `n`.
 * Knuth algorithm for random unique numbers.
 */
void rand_unique_vals_in_range(int *array, int n, int m) {
    int in, im;

    for (in = im = 0; in < n && im < m; ++in) {
        int rn = n - in;
        int rm = m - im;
        if (rand() % rn < rm)
            array[im++] = in;
    }
}


/**
 * Very simple (and dumb) function that does the following:
 *   - allocate ALLOC_MAX blocks of memory, where each of them has a size
 *     between ALLOC_BLOCK_MIN and ALLOC_BLOCK_MAX.
 *     Then, take REALLOC_MAX of these blocks of memory randomly chosen, free
 *     them and reallocate them again with a random size between
 *     ALLOC_BLOCK_MIN and ALLOC_BLOCK_MAX. Do this REALLOC_CYCLES times.
 *     Finally, free all allocated memory blocks.
 *     Print inside `file1` comma-separated data, where each line contains:
 *         "<current-cycle>,<memory-fragmentation-percentage>"
 *     <current-cycle> = 0 -------------------> first allocations
 *     <current-cycle> = [1..REALLOC_CYCLES] -> frees and re-allocs
 *     <current-cycle> = REALLOC_CYCLES + 1 --> final frees
 *   - print to stdout the <throughput>, calculated as:
 *         <number-of-allocs-and-frees>/<time-unit>
 */
void run_memory_data_test(FILE *file1, FILE *file2) {
    void   *allocs[ALLOC_MAX];
    int     idx_to_realloc[REALLOC_MAX];
    size_t  tot_allocated;
    size_t  tot_used;
    clock_t temp;
    clock_t tot_time = 0;

    // First allocate ALLOC_MAX blocks of memory, keeping ptrs inside `allocs`.
    for (int i = 0; i < ALLOC_MAX; ++i) {
        int size = (rand() % (ALLOC_BLOCK_MAX - ALLOC_BLOCK_MIN)) + ALLOC_BLOCK_MIN;
        temp = clock();
        allocs[i] = mymalloc(size);
        tot_time += clock() - temp;
        if (allocs[i] == NULL) {
            perror("mymalloc() error");
            exit(EXIT_FAILURE);
        }
    }

    // Print initial cycle situation.
    tot_allocated = get_total_allocated();
    tot_used = get_total_used();
    fprintf(file1, "0,%.2f\n",
            100 - (((double)tot_used) / ((double)tot_allocated) * 100.0));
    fprintf(file2, "0,%zu\n", tot_allocated);

    // Reallocate REALLOC_CYCLES times.
    for (int j = 0; j < REALLOC_CYCLES; ++j) {
        rand_unique_vals_in_range(idx_to_realloc, ALLOC_MAX, REALLOC_MAX);

        for (int i = 0; i < REALLOC_MAX; ++i) {
            int size;

            // Free
            temp = clock();
            myfree(allocs[idx_to_realloc[i]]);
            tot_time += clock() - temp;

            // Allocate again.
            size = (rand() % (ALLOC_BLOCK_MAX - ALLOC_BLOCK_MIN)) + ALLOC_BLOCK_MIN;
            temp = clock();
            allocs[idx_to_realloc[i]] = mymalloc(size);
            tot_time += clock() - temp;
            if (allocs[i] == NULL) {
                perror("mymalloc() error");
                exit(EXIT_FAILURE);
            }
        }

        // Print current cycle situation.
        tot_allocated = get_total_allocated();
        tot_used = get_total_used();
        fprintf(file1, "%d,%.2f\n", j + 1,
                100 - (((double)tot_used) / ((double)tot_allocated) * 100.0));
        fprintf(file2, "%d,%zu\n", j + 1, tot_allocated);
    }

    // Free all blocks of memory.
    for (int i = 0; i < ALLOC_MAX; ++i){
        temp = clock();
        myfree(allocs[i]);
        tot_time += clock() - temp;
    }

    // Print final cycle situation.
    tot_allocated = get_total_allocated();
    tot_used = get_total_used();
    fprintf(file1, "%d,%.2f\n", REALLOC_CYCLES + 1,
            100 - (((double)tot_used) / ((double)tot_allocated) * 100.0));
    fprintf(file2, "%d,%zu\n", REALLOC_CYCLES + 1, tot_allocated);

    printf("Throughput: %.3f mymallocs and frees per clock_t\n",
           (double)(ALLOC_MAX * 2 + REALLOC_CYCLES * REALLOC_MAX * 2) / (double)tot_time);
}
