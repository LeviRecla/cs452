#include <stdlib.h>
#include <sys/time.h>    /* for gettimeofday */
#include <sys/mman.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <stdint.h>
#include "lab.h"

#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS 0x20
#endif

size_t btok(size_t bytes) {
    if (bytes == 0) {
        return 0;
    }
    // Include the size of the header
    bytes += sizeof(struct avail);
    size_t k = SMALLEST_K;
    size_t block_size = (size_t)1 << k;
    while (block_size < bytes) {
        k++;
        block_size <<= 1;
    }
    return k;
}

void buddy_init(struct buddy_pool *pool, size_t size) {
    if (!pool) {
        return;
    }

    size_t k = MIN_K;
    if (size == 0) {
        k = DEFAULT_K;
    } else {
        size_t actual_size = (size_t)1 << MIN_K;
        while (actual_size < size) {
            actual_size <<= 1;
            k++;
        }
    }

    pool->kval_m = k;
    pool->numbytes = (size_t)1 << k;

    // Initialize sentinel nodes
    for (size_t i = 0; i < MAX_K; i++) {
        pool->avail[i].tag = BLOCK_UNUSED;
        pool->avail[i].kval = i;
        pool->avail[i].next = &pool->avail[i];
        pool->avail[i].prev = &pool->avail[i];
    }

    pool->base = mmap(NULL, pool->numbytes, PROT_READ | PROT_WRITE,
                      MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (pool->base == MAP_FAILED) {
        perror("mmap failed");
        exit(EXIT_FAILURE);
    }

    // Set up initial free block
    struct avail *initial_block = (struct avail *)pool->base;
    initial_block->tag = BLOCK_AVAIL;
    initial_block->kval = k;
    initial_block->next = &pool->avail[k];
    initial_block->prev = &pool->avail[k];

    pool->avail[k].next = initial_block;
    pool->avail[k].prev = initial_block;
    // Sentinel stays BLOCK_UNUSED
}

struct avail *buddy_calc(struct buddy_pool *pool, struct avail *block) {
    uintptr_t offset = (uintptr_t)block - (uintptr_t)pool->base;
    size_t block_size = (size_t)1 << block->kval;
    uintptr_t buddy_offset = offset ^ block_size;
    return (struct avail *)((uintptr_t)pool->base + buddy_offset);
}

void *buddy_malloc(struct buddy_pool *pool, size_t size) {
    if (!pool || size == 0) {
        return NULL;
    }

    size_t k = btok(size);
    if (k > pool->kval_m) {
        errno = ENOMEM;
        return NULL;
    }

    size_t i = k;
    while (i <= pool->kval_m && pool->avail[i].next == &pool->avail[i]) {
        i++;
    }

    if (i > pool->kval_m) {
        errno = ENOMEM;
        return NULL;
    }

    // Split blocks until we get the correct size
    while (i > k) {
        struct avail *block = pool->avail[i].next;
        // Remove from avail[i]
        block->prev->next = block->next;
        block->next->prev = block->prev;

        i--;
        size_t block_size = (size_t)1 << i;
        struct avail *buddy1 = block;
        struct avail *buddy2 = (struct avail *)((char *)block + block_size);

        buddy1->tag = BLOCK_AVAIL;
        buddy1->kval = i;
        buddy2->tag = BLOCK_AVAIL;
        buddy2->kval = i;

        // Insert buddy1
        buddy1->next = pool->avail[i].next;
        buddy1->prev = &pool->avail[i];
        pool->avail[i].next->prev = buddy1;
        pool->avail[i].next = buddy1;

        // Insert buddy2
        buddy2->next = pool->avail[i].next;
        buddy2->prev = &pool->avail[i];
        pool->avail[i].next->prev = buddy2;
        pool->avail[i].next = buddy2;
    }

    // Allocate from avail[k]
    struct avail *block = pool->avail[k].next;
    block->tag = BLOCK_RESERVED;
    block->prev->next = block->next;
    block->next->prev = block->prev;

    return (void *)(block + 1);
}

void buddy_free(struct buddy_pool *pool, void *ptr) {
    if (!pool || !ptr) {
        return;
    }

    struct avail *block = (struct avail *)ptr - 1;
    size_t k = block->kval;
    block->tag = BLOCK_AVAIL;

    // Coalesce
    while (k < pool->kval_m) {
        struct avail *buddy = buddy_calc(pool, block);
        if (buddy->tag != BLOCK_AVAIL || buddy->kval != k) {
            break;
        }

        // Remove buddy from free list
        buddy->prev->next = buddy->next;
        buddy->next->prev = buddy->prev;

        // Merge
        if (block > buddy) {
            struct avail *temp = block;
            block = buddy;
            buddy = temp;
        }

        k++;
        block->kval = k;
    }

    // Insert coalesced block back
    block->next = pool->avail[k].next;
    block->prev = &pool->avail[k];
    pool->avail[k].next->prev = block;
    pool->avail[k].next = block;
}

void *buddy_realloc(struct buddy_pool *pool, void *ptr, size_t size) {
    if (!pool) {
        return NULL;
    }
    if (!ptr) {
        return buddy_malloc(pool, size);
    }
    if (size == 0) {
        buddy_free(pool, ptr);
        return NULL;
    }

    struct avail *block = (struct avail *)ptr - 1;
    size_t old_size = ((size_t)1 << block->kval) - sizeof(struct avail);
    if (size <= old_size) {
        return ptr;
    } else {
        void *new_ptr = buddy_malloc(pool, size);
        if (!new_ptr) {
            return NULL;
        }
        memcpy(new_ptr, ptr, old_size);
        buddy_free(pool, ptr);
        return new_ptr;
    }
}

void buddy_destroy(struct buddy_pool *pool) {
    if (!pool || !pool->base) {
        return;
    }
    munmap(pool->base, pool->numbytes);
    pool->base = NULL;
}

int myMain(int argc, char** argv) {
    // Optional test runner, currently unused.
    return 0;
}
