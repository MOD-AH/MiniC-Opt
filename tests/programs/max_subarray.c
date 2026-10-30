#include <stdio.h>
#include <stddef.h>

#define MEMORY_SIZE 1024

// Global memory pool simulating the system heap
static char memory_pool[MEMORY_SIZE];

// Header structure for each memory block
// Using typedef to abstract the struct definition
typedef struct Block {
    size_t size;         // Size of the data block
    int is_free;         // 1 if free, 0 if allocated
    struct Block* next;  // Pointer to the next block
} Block;

// Pointer to the start of our memory linked list
static Block* free_list = (Block*)memory_pool;

// Initializes the memory pool on the first allocation request
void initialize_allocator() {
    free_list->size = MEMORY_SIZE - sizeof(Block);
    free_list->is_free = 1;
    free_list->next = NULL;
}

// Custom malloc: Finds a free block or splits an existing one
void* my_malloc(size_t size) {
    // Basic structural initialization check
    if (free_list->size == 0 && free_list->next == NULL) {
        initialize_allocator();
    }

    Block* current = free_list;
    
    // First-fit algorithm to find a suitable free memory block
    while (current != NULL) {
        if (current->is_free && current->size >= size) {
            
            // Check if block can be split (needs space for metadata + at least 1 byte of data)
            if (current->size >= size + sizeof(Block) + 1) {
                // Calculate pointer for the new split block
                Block* new_block = (Block*)((char*)current + sizeof(Block) + size);
                
                // Initialize the split block structures
                new_block->size = current->size - size - sizeof(Block);
                new_block->is_free = 1;
                new_block->next = current->next;
                
                // Update current allocated block
                current->size = size;
                current->next = new_block;
            }
            
            current->is_free = 0;
            // Return pointer to data section (skipping the block header)
            return (void*)((char*)current + sizeof(Block));
        }
        current = current->next;
    }
    
    return NULL; // Out of memory
}

// Custom free: Marks a block as free and merges adjacent free blocks
void my_free(void* ptr) {
    if (ptr == NULL) return;

    // Move backward to access the metadata block header
    Block* block = (Block*)((char*)ptr - sizeof(Block));
    block->is_free = 1;

    // Coalescing: Merge continuous free blocks to prevent fragmentation
    Block* current = free_list;
    while (current != NULL) {
        if (current->is_free && current->next && current->next->is_free) {
            current->size += sizeof(Block) + current->next->size;
            current->next = current->next->next;
            // Don't advance 'current' yet, see if next block can also be merged
            continue;
        }
        current = current->next;
    }
}

int main() {
    printf("Starting Custom Allocator Test...\n\n");

    // Allocate three different segments
    int* arr1 = (int*)my_malloc(5 * sizeof(int));
    char* str = (char*)my_malloc(20 * sizeof(char));
    int* arr2 = (int*)my_malloc(10 * sizeof(int));

    if (arr1 && str && arr2) {
        printf("Successfully allocated memory segments!\n");
        printf("arr1 location: %p\n", (void*)arr1);
        printf("str  location: %p\n", (void*)str);
        printf("arr2 location: %p\n\n", (void*)arr2);
    }

    // Free the middle segment to test memory fragmentation handling
    printf("Freeing 'str' to create a memory hole...\n");
    my_free(str);

    // Reallocate a smaller block that fits into the hole
    char* short_str = (char*)my_malloc(5 * sizeof(char));
    printf("short_str location (should reuse str memory): %p\n\n", (void*)short_str);

    // Free everything to test continuous block merging (coalescing)
    printf("Freeing remaining memory blocks...\n");
    my_free(arr1);
    my_free(short_str);
    my_free(arr2);

    printf("\nAll memory freed and coalesced successfully.\n");
    return 0;
}
