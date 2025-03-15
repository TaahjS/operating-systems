#include "alloc.h"

// Maximum number of blocks (free or allocated) we can track.
#define MAX_BLOCKS 512

// Structure for tracking memory blocks.
typedef struct {
    size_t offset;  // Offset from the beginning of the 4KB page.
    size_t size;    // Size of the block.
} block_info;

// Global pointer to our memory pool (the 4KB page).
static void *pool = NULL;

// Array to track free blocks and its count.
static block_info free_blocks[MAX_BLOCKS];
static int free_count = 0;

// Array to track allocated blocks and its count.
static block_info allocated_blocks[MAX_BLOCKS];
static int allocated_count = 0;

// Helper function to insert a free block and merge adjacent blocks.
static void insert_free_block(size_t offset, size_t size) {
    // Insert new free block at the end.
    free_blocks[free_count].offset = offset;
    free_blocks[free_count].size = size;
    free_count++;

    // Sort the free blocks by offset (simple insertion sort).
    for (int i = 1; i < free_count; i++) {
        block_info key = free_blocks[i];
        int j = i - 1;
        while (j >= 0 && free_blocks[j].offset > key.offset) {
            free_blocks[j + 1] = free_blocks[j];
            j--;
        }
        free_blocks[j + 1] = key;
    }

    // Merge contiguous free blocks.
    int i = 0;
    while (i < free_count - 1) {
        if (free_blocks[i].offset + free_blocks[i].size == free_blocks[i + 1].offset) {
            free_blocks[i].size += free_blocks[i + 1].size;
            // Shift left the remaining blocks.
            for (int j = i + 1; j < free_count - 1; j++) {
                free_blocks[j] = free_blocks[j + 1];
            }
            free_count--;
        } else {
            i++;
        }
    }
}

// Initializes the memory manager by allocating a 4KB page.
int init_alloc() {
    pool = mmap(NULL, PAGESIZE, PROT_READ | PROT_WRITE,
                MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (pool == MAP_FAILED) {
        perror("mmap failed");
        return -1;
    }
    // Initially, the entire page is free.
    free_count = 1;
    free_blocks[0].offset = 0;
    free_blocks[0].size = PAGESIZE;
    allocated_count = 0;
    return 0;
}

// Cleans up the memory manager and releases the page back to the OS.
int cleanup() {
    int ret = munmap(pool, PAGESIZE);
    if (ret != 0) {
        perror("munmap failed");
        return -1;
    }
    pool = NULL;
    free_count = 0;
    allocated_count = 0;
    return 0;
}

// Allocates a buffer of the given size (must be a multiple of MINALLOC).
char *alloc(int size) {
    // Reject invalid sizes.
    if (size <= 0 || (size % MINALLOC != 0)) {
        return NULL;
    }

    // Look for a free block that is large enough.
    for (int i = 0; i < free_count; i++) {
        if (free_blocks[i].size >= (size_t)size) {
            size_t alloc_offset = free_blocks[i].offset;

            // Record this allocation.
            allocated_blocks[allocated_count].offset = alloc_offset;
            allocated_blocks[allocated_count].size = size;
            allocated_count++;

            // Update the free block.
            if (free_blocks[i].size == (size_t)size) {
                // Exact fit; remove this free block.
                for (int j = i; j < free_count - 1; j++) {
                    free_blocks[j] = free_blocks[j + 1];
                }
                free_count--;
            } else {
                // Split the free block.
                free_blocks[i].offset += size;
                free_blocks[i].size -= size;
            }
            return (char *)pool + alloc_offset;
        }
    }
    // No suitable free block found.
    return NULL;
}

// Deallocates a previously allocated block, making it available again.
void dealloc(char *ptr) {
    if (ptr == NULL || pool == NULL) {
        return;
    }
    // Calculate the offset within the pool.
    size_t offset = ptr - (char *)pool;
    int found_index = -1;
    size_t block_size = 0;

    // Find the block in the allocated list.
    for (int i = 0; i < allocated_count; i++) {
        if (allocated_blocks[i].offset == offset) {
            found_index = i;
            block_size = allocated_blocks[i].size;
            break;
        }
    }
    if (found_index == -1) {
        // Pointer not recognized; do nothing.
        return;
    }

    // Remove the block from the allocated list.
    for (int i = found_index; i < allocated_count - 1; i++) {
        allocated_blocks[i] = allocated_blocks[i + 1];
    }
    allocated_count--;

    // Insert the freed block into the free list.
    insert_free_block(offset, block_size);
}
