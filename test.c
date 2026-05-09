// C Standard Library
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "myallocs.h"


#define ALLOC_MAX       10
#define ALLOC_BLOCK_MIN 1
#define ALLOC_BLOCK_MAX 10000
#define REALLOC_MAX     2
#define REALLOC_CYCLES  1000


void rand_unique_vals_in_range(int *array, int n, int m);
void run_memory_data_test(const char *data_filename);


int main(void) {
    // Seed with current time.
    srand(time(NULL));

    run_memory_data_test("data.txt");

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
 * TODO
 */
void run_memory_data_test(const char *data_filename) {
    void  *allocs[ALLOC_MAX];
    int    idx_to_realloc[REALLOC_MAX];
    size_t tot_allocated;
    size_t tot_used;
    FILE  *file;

    // Open data file
    file = fopen(data_filename, "w");
    if (file == NULL) {
        perror("fopen() error");
        exit(EXIT_FAILURE);
    }

    // First allocate ALLOC_MAX blocks of memory, keeping ptrs inside `allocs`.
    for (int i = 0; i < ALLOC_MAX; ++i) {
        int size = (rand() % (ALLOC_BLOCK_MAX - ALLOC_BLOCK_MIN)) + ALLOC_BLOCK_MIN;
        allocs[i] = mymalloc(size);
        if (allocs[i] == NULL) {
            perror("mymalloc() error");
            fclose(file);
            exit(EXIT_FAILURE);
        }
    }

    // Print initial cycle situation.
    tot_allocated = get_total_allocated();
    tot_used = get_total_used();
    fprintf(file, "0,%.2f\n",
            100 - (((double)tot_used) / ((double)tot_allocated) * 100.0));

    // Reallocate REALLOC_CYCLES times.
    for (int j = 0; j < REALLOC_CYCLES; ++j) {
        rand_unique_vals_in_range(idx_to_realloc, ALLOC_MAX, REALLOC_MAX);

        for (int i = 0; i < REALLOC_MAX; ++i) {
            int size;

            // Free
            myfree(allocs[idx_to_realloc[i]]);

            // Allocate again.
            size = (rand() % (ALLOC_BLOCK_MAX - ALLOC_BLOCK_MIN)) + ALLOC_BLOCK_MIN;
            allocs[idx_to_realloc[i]] = mymalloc(size);
            if (allocs[i] == NULL) {
                perror("mymalloc() error");
                fclose(file);
                exit(EXIT_FAILURE);
            }
        }

        // Print current cycle situation.
        tot_allocated = get_total_allocated();
        tot_used = get_total_used();
        fprintf(file, "%d,%.2f\n", j + 1,
                100 - (((double)tot_used) / ((double)tot_allocated) * 100.0));
    }

    // Free all blocks of memory.
    for (int i = 0; i < ALLOC_MAX; ++i)
        myfree(allocs[i]);

    // Print final cycle situation.
    tot_allocated = get_total_allocated();
    tot_used = get_total_used();
    fprintf(file, "%d,%.2f\n", REALLOC_CYCLES + 1,
            100 - (((double)tot_used) / ((double)tot_allocated) * 100.0));

    // Close data file.
    fclose(file);
}
