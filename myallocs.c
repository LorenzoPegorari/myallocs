#define _DEFAULT_SOURCE  // Feature Test Macro Requirements for glibc 2.19

// C Standard Library
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

// POSIX standard
#include <pthread.h>
#include <unistd.h>

#include "myallocs.h"


/*
 * The idea is to add to every allocated memory block an header, that contains
 * the size of the block, and a flag that indicates if the block is marked as
 * free.
 *
 * The header is kept completely hidden from the caller.
 *
 * linked list TODO
 *
 * Align to 16
 *
 * head and tail
 *
 * pthread lock
 */


typedef char ALIGN[32];

union header_tag {
    struct {
        size_t            size;
        bool              is_free;
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
 * size is bigger than the given `size`. The policy used is the "FIRST FIT".
 * Return the found block, or NULL if no block is free and big enough.
 */
static header_t *get_free_block(size_t size) {
    header_t *curr = head;

    while (curr) {
        if (curr->s.is_free == true && curr->s.size >= size)
            return curr;
        curr = curr->s.next;
    }

    return NULL;
}


void *mymalloc(size_t size) {
    header_t *header;
    size_t    total_size;
    void     *block;

    // If given `size` is 0, return NULL. `errno` NOT set.
    if (size == 0)
        return NULL;

    // Check if we have a free block of memory big enough. If so, return that.
    pthread_mutex_lock(&global_mymallocs_mutex);
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
        pthread_mutex_unlock(&global_mymallocs_mutex);
        return NULL;
    }

    /* If we don't have a free and big enough block of memory already
       allocated, let's extend the heap by allocating it using the syscall
       `sbrk` (NOTE: `sbrk` is probably the simplest, but not the best choice
       to create a modern memory allocator!). */
    total_size = sizeof(header_t) + size;
    block = sbrk((intptr_t)total_size);
    if (block == (void*)-1) {
        // Syscall `sbrk` returned an error!
        pthread_mutex_unlock(&global_mymallocs_mutex);
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
    header->s.next = NULL;
    if (head == NULL)
        head = header;
    if (tail != NULL)
        tail->s.next = header;
    tail = header;
    pthread_mutex_unlock(&global_mymallocs_mutex);
    return (void *)(header + 1);
}


void myfree(void *ptr) {
    header_t *header;
    void     *programbreak;

    // If given `ptr` is NULL, return. `errno` NOT set.
    if (ptr == NULL)
        return;

    // Take the given memory block `ptr`, and obtain its `header`.
    pthread_mutex_lock(&global_mymallocs_mutex);
    header = (header_t *)ptr - 1;

    programbreak = sbrk((intptr_t)0);
    if ((char *)ptr + header->s.size == programbreak) {
        size_t total_size;

        if (head == tail)
            head = tail = NULL;
        else {
            header_t *tmp;

            tmp = head;
            while (tmp) {
                if (tmp->s.next == tail) {
                    tmp->s.next = NULL;
                    tail = tmp;
                }
                tmp = tmp->s.next;
            }
        }
        total_size = sizeof(header_t) + header->s.size;
        sbrk(0 - total_size);
#ifdef MYALLOCS_DEBUG
        total_allocated -= total_size;
#endif
        pthread_mutex_unlock(&global_mymallocs_mutex);
        return;
    }
    header->s.is_free = true;
    pthread_mutex_unlock(&global_mymallocs_mutex);
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
