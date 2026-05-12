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
/** @file myallocs.c */


#define _DEFAULT_SOURCE  // Feature Test Macro Requirements for glibc 2.19

// C Standard Library
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// POSIX standard
#include <pthread.h>
#include <unistd.h>

#include "myallocs.h"


/*
 * GENERAL IDEA:
 * The idea is to add to every allocated memory block an header that contains:
 *   - the size of the block
 *   - a flag that indicates if the block is marked as free
 *   - a pointer to the prev memory block header (doubly-linked list)
 *   - a pointer to the next memory block header (doubly-linked list)
 *
 * The header is kept completely hidden from the caller.
 *
 * The header is wrapped inside a `union`, forcing it to be aligned to `ALIGN`.
 *
 * The maximum size of memory that can be requestd with on `mymalloc()` (or
 * similar functions) call is:
 *     (SIZE_MAX / 2) - sizeof(header_t)
 */

/*
 * LIMITATIONS:
 * - This simple implementation doesn't do splitting of big memory blocks.
 * - This simple implementation doesn't do coalescing of free memory blocks.
 * - The memory blocks don't have any "magic number" (or something similar).
 *   This means that we are not checking if, for example, the passed `ptr` to
 *   `myfree()`.
 * - We allocating each memory block by itself when it is necessary. A better
 *   solution would be to allocate in pages (of maybe 4KB, a common page size
 *   in Linux systems).
 */


typedef char ALIGN[32];

union header_tag {
    struct {
        size_t            size;
        bool              is_free;
        union header_tag *prev;
        union header_tag *next;
    } s;
    ALIGN stub;
};

typedef union header_tag header_t;


header_t *head = NULL, *tail = NULL;
pthread_mutex_t global_mymallocs_mutex = PTHREAD_MUTEX_INITIALIZER;
#ifdef MYALLOCS_DEBUG
size_t total_allocated = 0;
#endif


/**
 * Check if there is an already allocated memory block that is free, and whose
 * size is bigger than the given `size`. The standard policy used is the "FIRST
 * FIT" (but it is possible to compile the code using the "BEST FIT" policy).
 * Return the found block, or NULL if no block is free and big enough.
 */
static header_t *get_free_block(size_t size) {
#ifdef MYALLOCS_BEST_FIT
    header_t *curr = head;
    header_t *best_fit = NULL;

    while (curr) {
        if (curr->s.is_free == true) {
            if (curr->s.size == size)
                return curr;
            else if (curr->s.size > size &&
                     (best_fit == NULL || curr->s.size < best_fit->s.size))
                best_fit = curr;
        }
        curr = curr->s.next;
    }

    return best_fit;
#else
    header_t *curr = head;

    while (curr) {
        if (curr->s.is_free == true && curr->s.size >= size)
            return curr;
        curr = curr->s.next;
    }

    return NULL;
#endif
}


void *mymalloc(size_t size) {
    header_t *header;
    size_t    total_size;
    void     *block;

    // If given `size` is 0, return NULL. `errno` NOT set.
    if (size == 0)
        return NULL;

    // Check if we have a free block of memory big enough. If so, return that.
    if (pthread_mutex_lock(&global_mymallocs_mutex) != 0)
        abort();
    header = get_free_block(size);
    if (header != NULL) {
        header->s.is_free = false;
        pthread_mutex_unlock(&global_mymallocs_mutex);
        return (void *)(header + 1);
    }

    /* If `size` is bigger than (SIZE_MAX / 2) minus the space for the memory
       block header, return NULL. `errno` set to EOVERFLOW. */
    if (size > (SIZE_MAX / 2) - sizeof(header_t)) {
        errno = EOVERFLOW;
        if (pthread_mutex_unlock(&global_mymallocs_mutex) != 0)
            abort();
        return NULL;
    }

    /* If we don't have a free and big enough block of memory already
       allocated, let's extend the heap by allocating it using the syscall
       `sbrk` (NOTE: `sbrk()` is probably the simplest, but not the best choice
       to create a modern memory allocator!). */
    total_size = sizeof(header_t) + size;
    block = sbrk((intptr_t)total_size);
    if (block == (void*)-1) {
        // Syscall `sbrk` returned an error!
        if (pthread_mutex_unlock(&global_mymallocs_mutex) != 0)
            abort();
        return NULL;
    }
#ifdef MYALLOCS_DEBUG
    total_allocated += total_size;
#endif

    /* If we sucessfully allocated the new block of memory, let's initialize
       its `header` (and `head` and `tail` if necessary). */
    header = (header_t *)block;
    header->s.size = size;
    header->s.is_free = false;
    header->s.prev = tail;
    header->s.next = NULL;
    if (head == NULL)
        head = header;
    if (tail != NULL)
        tail->s.next = header;
    tail = header;
    if (pthread_mutex_unlock(&global_mymallocs_mutex) != 0)
        abort();
    return (void *)(header + 1);
}


void myfree(void *ptr) {
    header_t *header;
    void     *programbreak;

    // If given `ptr` is NULL, return. `errno` NOT set.
    if (ptr == NULL)
        return;

    /* `sbrk(0)` returns the current location of the program break. It cannot
       fail.
       We use the program break to check if the block of memory to free is not
       the last memory block. If so, we simply mark that memory block as free.
       Else, it can be safely deallocated using `sbrk()`. */
    /* NOTE: its not possible to simply move back in the linked list of memory
       blocks and deallocate all the blocks that are marked as free that we
       encounter before the first not free block without checking, because we
       cannot know for certain that a third-party memory allocator didn't
       allocate memory in between the blocks in the linked list, meaning that
       we can't assume that the blocks are contiguous!
       We need to manually check for each block if it is at the program break,
       and then free it. Let's give the user the possibility of enabling this
       mechanism with the MYALLOCS_FULL_DEALLOC macro. */
    if (pthread_mutex_lock(&global_mymallocs_mutex) != 0)
        abort();
    header = (header_t *)ptr - 1;
    programbreak = sbrk((intptr_t)0);
    if ((char *)ptr + header->s.size != programbreak) {
        header->s.is_free = true;
        if (pthread_mutex_unlock(&global_mymallocs_mutex) != 0)
            abort();
        return;
    }

#ifdef MYALLOCS_FULL_DEALLOC
    do {
        size_t total_size;

        total_size = sizeof(header_t) + header->s.size;
        if (head == tail) {
            head = tail = NULL;
            if (sbrk((intptr_t)0 - (intptr_t)total_size) == (void*)-1) {
                // Syscall `sbrk` returned an error!
                pthread_mutex_unlock(&global_mymallocs_mutex);
                abort();
            }
#ifdef MYALLOCS_DEBUG
            total_allocated -= total_size;
#endif
            if (pthread_mutex_unlock(&global_mymallocs_mutex) != 0)
                abort();
            return;
        }

        header = header->s.prev;
        header->s.next = NULL;
        tail = header;
        if (sbrk((intptr_t)0 - (intptr_t)total_size) == (void*)-1) {
            // Syscall `sbrk` returned an error!
            pthread_mutex_unlock(&global_mymallocs_mutex);
            abort();
        }
#ifdef MYALLOCS_DEBUG
        total_allocated -= total_size;
#endif

        programbreak = sbrk((intptr_t)0);
    } while (header->s.is_free == true && (char *)(header + 1) + header->s.size == programbreak);
#else
    size_t total_size;

    if (head == tail)
        head = tail = NULL;
    else {
        header_t *prev;

        prev = header->s.prev;
        prev->s.next = NULL;
        tail = prev;
    }
    total_size = sizeof(header_t) + header->s.size;
    if (sbrk((intptr_t)0 - (intptr_t)total_size) == (void*)-1) {
        // Syscall `sbrk` returned an error!
        pthread_mutex_unlock(&global_mymallocs_mutex);
        abort();
    }
#ifdef MYALLOCS_DEBUG
    total_allocated -= total_size;
#endif
#endif
    if (pthread_mutex_unlock(&global_mymallocs_mutex) != 0)
        abort();
}


void *myrealloc(void *ptr, size_t size) {
    header_t *header;
    void     *ret;

    // If `ptr` is NULL, `myrealloc()` shall be equivalent to 'mymalloc(size)`.
    if (ptr == NULL) {
        return mymalloc(size);
    }

    // If `size` is 0, and `ptr` is not NULL, the block shall be freed.
    if (size == 0) {
        myfree(ptr);
        return NULL;
    }

    /* If the new `size` is less or equal to the old `header->s.size`, then
       just return that block of memory again. */
    header = (header_t *)ptr - 1;
    if (header->s.size >= size)
        return ptr;

    /* If the new `size` is more than the old `header->s.size`, then use
       `mymalloc()` to get a new block of memory, then copy the content to this
       new memory block, and finally free the old memory block. */
    ret = mymalloc(size);
    if (ret != NULL) {
        memcpy(ret, ptr, header->s.size);
        myfree(ptr);
    }
    return ret;
}


void *mycalloc(size_t nelem, size_t elsize) {
    size_t size;
    void  *block;

    // If given `nelem` or `elsize` is 0, return NULL. `errno` NOT set.
    if (nelem == 0 || elsize == 0)
        return NULL;

    /* Calculate `size`, check if overflow, and then allocate it with
       `mymalloc()`. Notice that we just have to check for multiplication
       overflow, and not worry about the `size` still being too big, as that
       will be handled by `mymalloc()`. */
    size = nelem * elsize;
	if (size / nelem != elsize) {
        errno = EOVERFLOW;
        return NULL;
    }
    block = mymalloc(size);
    if (block == NULL)
        return NULL;

    // Set all `block` bytes to 0. `memset()` cannot fail.
    (void)memset(block, 0, size);
    return block;
}


#ifdef MYALLOCS_DEBUG
size_t get_total_allocated(void) {
    return total_allocated;
}


size_t get_total_used(void) {
    header_t *curr       = head;
    size_t    total_used = 0;

    while (curr) {
        if (curr->s.is_free == false)
            total_used += curr->s.size;
        curr = curr->s.next;
    }

    return total_used;
}
#endif
