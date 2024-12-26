#include "allocator.h"
#include <string.h>

// Metadata structure for each allocated block
typedef struct metadata {
    size_t size;          // Size of the block
    int check_free;       // Whether the block is free or not
    struct metadata *next; // Pointer to the next block in the free list
    struct metadata *prev; // Pointer to the previous block in the free list
} md;

static void *base;        // Base of the memory pool
static size_t used;       // Bytes used in the memory pool
static md *free_list = NULL;  // Head of the free list

// Initialize the allocator
void allocator_init(void *newbase) {
    base = newbase;
    used = 0;
    free_list = NULL;
}

// Reset the allocator
void allocator_reset() {
    used = 0;
    free_list = NULL;
}

// Helper function to insert a block into the free list
void insert_into_free_list(md *block) {
    block->check_free = 1;
    block->next = free_list;
    block->prev = NULL;
    if (free_list) {
        free_list->prev = block;
    }
    free_list = block;
}

// Helper function to remove a block from the free list
void remove_from_free_list(md *block) {
    if (block->prev) {
        block->prev->next = block->next;
    } else {
        free_list = block->next;
    }
    if (block->next) {
        block->next->prev = block->prev;
    }
}

// Merge adjacent free blocks
void merge_blocks(md *block) {
    // Try to merge with next block if it's free
    md *next_block = (md *)((char *)block + sizeof(md) + block->size);
    if ((char *)next_block < (char *)base + used && next_block->check_free) {
        remove_from_free_list(next_block);
        block->size += sizeof(md) + next_block->size;
    }

    // Try to merge with previous block if it's free
    if (block->prev && block->prev->check_free) {
        md *prev_block = block->prev;
        remove_from_free_list(prev_block);
        prev_block->size += sizeof(md) + block->size;
        block = prev_block;
    }
}

// Allocate a block of memory
void *mymalloc(size_t size) {
    size_t total = sizeof(md) + size;
    void *ans = NULL;
    md *current = free_list;

    // First, try to find a free block that is large enough
    while (current) {
        if (current->size >= size) {
            remove_from_free_list(current);
            current->check_free = 0;
            return (void *)(current + 1);
        }
        current = current->next;
    }

    // If no free block is found, allocate at the end of the used memory
    if (used + total > 128 * 1024 * 1024) {  // Assume memory pool is 128MiB
        return NULL;  // Out of memory
    }

    ans = (char *)base + used;
    md *metadata = (md *)ans;
    metadata->size = size;
    metadata->check_free = 0;
    used += total;
    return (void *)(metadata + 1);
}

// Free a block of memory
void myfree(void *ptr) {
    if (!ptr) return;
    md *metadata = (md *)ptr - 1;

    // Mark the block as free and insert it into the free list
    insert_into_free_list(metadata);

    // Merge the current block with adjacent free blocks if possible
    merge_blocks(metadata);
}

// Reallocate a block of memory
void *myrealloc(void *ptr, size_t size) {
    if (!size) {
        myfree(ptr);
        return NULL;
    }
    if (!ptr) {
        return mymalloc(size);
    }

    md *metadata = (md *)ptr - 1;
    size_t old_size = metadata->size;

    // If the block is at the end of the heap, grow or shrink it without moving
    void *end = (char *)base + used;
    if ((char *)ptr + old_size == end) {
        if (size > old_size) {
            used += size - old_size;  // Grow the block
        } else {
            used -= old_size - size;  // Shrink the block and adjust used
        }
        metadata->size = size;
        return ptr;
    }

    // Otherwise, allocate a new block and copy the data over
    void *new_ptr = mymalloc(size);
    if (new_ptr) {
        memcpy(new_ptr, ptr, old_size < size ? old_size : size);
        myfree(ptr);
    }
    return new_ptr;
}
