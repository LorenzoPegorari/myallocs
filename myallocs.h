#ifndef _MYALLOCS_H_
#define _MYALLOCS_H_


// C Standard Library
#include <stddef.h>


/**
 * Allocate `size` bytes of memory.
 * Return pointer to the allocated memory, NULL if the given `size` is 0, and
 * NULL (with `errno` set to error value) if an error occurred.
 *   - `errno == ENOMEM` ----> if no more available memory
 *   - `errno == EAGAIN` ----> if memory available for allocation to this
 *                             process is temporarily insufficient
 *   - `errno == EOVERFLOW` -> if `size` is bigger than (SIZE_MAX / 2) minus
 *                             the space for the memory block header
 */
void *mymalloc(size_t size);


/**
 * Free a block of memory previously allocated by `mymalloc`, `myrealloc` or
 * `mycalloc`.
 */
void myfree(void *ptr);


/**
 * Re-allocate the previously allocated block in `ptr`, making the new block
 * `size` bytes long. Free `ptr` if `size` is 0.
 * Return pointer to the re-allocated memory, NULL if the given `ptr` or `size`
 * is NULL, and NULL (with `errno` set to error value) if an error occurred.
 *   - `errno == ENOMEM` ----> if no more available memory
 *   - `errno == EAGAIN` ----> if memory available for allocation to this
 *                             process is temporarily insufficient
 *   - `errno == EOVERFLOW` -> if `size` is bigger than (SIZE_MAX / 2) minus
 *                             the space for the memory block header
 */
void *myrealloc(void *ptr, size_t size);


/**
 * Allocate `nelem` elements of `elsize` bytes each, all initialized to 0.
 * Return pointer to the allocated memory, NULL if the given `nelem` or
 * `elsize` is 0, and NULL (with `errno` set to error value) if an error
 * occurred.
 *   - `errno == ENOMEM` ----> if no more available memory
 *   - `errno == EAGAIN` ----> if memory available for allocation to this
 *                             process is temporarily insufficient
 *   - `errno == EOVERFLOW` -> if `nelem * elsize` causes a multiplication
 *                             overflow, or if it is bigger than (SIZE_MAX / 2)
 *                             minus the space for the memory block header
 */
void *mycalloc(size_t nelem, size_t elsize);


#ifdef MYALLOCS_DEBUG
/**
 * Get amount of allocated memory.
 */
size_t get_total_allocated(void);


/**
 * Get amount of memory that can actually be used by the caller.
 */
size_t get_total_used(void);
#endif


#endif
