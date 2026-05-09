#ifndef _MYALLOCS_H_
#define _MYALLOCS_H_


// C Standard Library
#include <stddef.h>


/**
 * Allocate `size` bytes of memory.
 * Return pointer to the allocated memory, NULL if the given `size` is 0, and
 * NULL (with `errno` set to error value) if an error occurred.
 */
void *mymalloc(size_t size);


/**
 * Free a block of memory previously allocated by `mymalloc`, `myrealloc` or
 * `mycalloc`.
 */
void myfree(void *ptr);


#ifdef MYALLOCS_DEBUG
/**
 * TODO.
 */
size_t get_total_allocated(void);


/**
 * TODO.
 */
size_t get_total_used(void);
#endif


#endif
