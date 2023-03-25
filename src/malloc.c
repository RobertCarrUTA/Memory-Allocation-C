#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>

#define ALIGN4(s)           (((((s) - 1) >> 2) << 2) + 4) // Macro definition that aligns the given value s to 4 bytes
#define BLOCK_DATA(b)       ((b) + 1)                     // Macro definition to get the data portion of a block based on the block pointer
#define BLOCK_HEADER(ptr)   ((struct _block *)(ptr) - 1)  // Macro definition to get the header portion of a block based on the data pointer

// Static variables used to keep track of statistics for the heap manager
static int atexit_registered = 0;
static int num_mallocs       = 0;
static int num_frees         = 0;
static int num_reuses        = 0;
static int num_grows         = 0;
static int num_splits        = 0;
static int num_coalesces     = 0;
static int num_blocks        = 0;
static int num_requested     = 0;
static int max_heap          = 0;

static int num_requested_temp = 0; // A variable used to keep track of the previous requested heap size

/***********************************************************************************************************************
  @brief: Prints various heap management statistics
***********************************************************************************************************************/
void printStatistics(void)
{
    printf("\nHeap management statistics:\n");
    printf("mallocs:\t%d\n",   num_mallocs);
    printf("frees:\t\t%d\n",   num_frees);
    printf("reuses:\t\t%d\n",  num_reuses);
    printf("grows:\t\t%d\n",   num_grows);
    printf("splits:\t\t%d\n",  num_splits);
    printf("coalesces:\t%d\n", num_coalesces);
    printf("blocks:\t\t%d\n",  num_blocks);
    printf("requested:\t%d\n", num_requested);
    printf("max heap:\t%d\n",  max_heap);
}

// Structure definition for a block of memory
struct _block
{
    size_t size;          // Size of the allocated _block of memory in bytes
    struct _block *prev;  // Pointer to the previous _block of allocated memory
    struct _block *next;  // Pointer to the next _block of allocated memory
    bool   free;          // Used to determine if this _block is free
    char   padding[3];    // Used to ensure proper alignment of the structure
};

struct _block *heapList = NULL; // Global variable to track the list of free blocks
struct _block *lastList = NULL; // Global variable to keep track of the last block allocated

/***********************************************************************************************************************
  @brief: findFreeBlock takes two parameters, a double pointer to _block structure and a size_t variable size. The
            function returns a pointer to a _block structure. The purpose of this function is to traverse through the
            linked list of allocated memory blocks and to find a free block that has enough space for the requested size.
            The function also updates the num_blocks counter to keep track of the number of blocks that have been
            traversed

            The function uses preprocessor macros to determine the allocation policy for selecting a free block. There
            are four allocation policies supported: First fit, Best fit, Worst fit, and Next fit. Based on the
            preprocessor macros set, the appropriate policy is used to traverse through the list of blocks to find the
            free block with enough space

            The function keeps track of the last block that was checked for the allocation policy. If the allocation
            policy is set to Next fit, the function also keeps track of the last block that was checked for allocation
            before the current block was checked. Finally, the function returns a pointer to the selected free block
***********************************************************************************************************************/
struct _block *findFreeBlock(struct _block **last, size_t size)
{
    struct _block *curr = heapList;

    // FIRST FIT
    #if defined FIT && FIT == 0

    // Search the list until we find a block that is both free and large enough
    while (curr && !(curr->free && curr->size >= size))
    {
        *last = curr;

        // If the block is free, increment the count of free blocks
        if(curr->free)
        {
            num_blocks++;
        }
        curr  = curr->next;
    }
    #endif

    // BEST FIT
    #if defined BEST && BEST == 0

    // Search the list for the block that has the least amount of leftover space after allocation
    struct _block * winner  = NULL;
    int spaceRemaining = INT_MAX;
    while ( curr )
    {
        // If the block is free and has enough space, calculate the remaining space
        if(curr->free && curr->size > size)
        {
            if((curr->size - size) < spaceRemaining)
            {
                spaceRemaining = curr->size - size;
                winner = curr;
            }
        }
        // If the block is free, increment the count of free blocks
        if(curr->free)
        {
            num_blocks++;
        }
        *last = curr;
        curr = curr->next;
    }
    curr = winner;
    #endif

    // WORST FIT
    #if defined WORST && WORST == 0

    // Search the list for the block that has the most leftover space after allocation
    struct _block * winner  = NULL;
    int spaceRemaining = INT_MIN;
    while ( curr )
    {
        // If the block is free and has enough space, calculate the remaining space
        if((curr->free) && (curr->size > size))
        {
            if(curr->size - size < spaceRemaining)
            {
                spaceRemaining = curr->size - size;
                winner = curr;
            }
        }
        // If the block is free, increment the count of free blocks
        if(curr->free)
        {
            num_blocks++;
        }
        curr = curr->next;
    }
    curr = winner;
    #endif

    // NEXT FIT
    #if defined NEXT && NEXT == 0
    while (curr && !((curr->size >= size) && (curr->free)))
    {
        *last = curr;
        curr  = curr->next;
    }

    // If we've reached the end of the list, start from the beginning next time
    if(!lastList)
    {
        lastList = curr;
    }

    while (lastList && !((lastList->size >= size) && (lastList->free)))
    {
        *last = lastList;
        // If the block is free, increment the count of free blocks
        if(lastList->free)
        {
            num_blocks++;
        }
        lastList  = lastList->next;
    }

    curr = lastList;
    #endif

    return curr;
}

/***********************************************************************************************************************
  @brief: This function is responsible for requesting more memory from the operating system to extend the size of the
            heap
            
            The function takes two arguments, a pointer to the last block in the heap and the size of the new block to
            be allocated. The function uses the sbrk() system call to request additional memory space from the OS. It
            creates two pointers to _block structures, curr, and prev. curr is set to the current location of the program
            break, which is the end of the heap. prev is set to the result of calling sbrk() with the size of the new
            block plus the size of a _block structure

            The function checks if the call to sbrk() was successful by comparing the values of curr and prev. If curr is
            equal to -1, this indicates that the request for additional memory space has failed, and the function returns
            NULL. If the heapList pointer is NULL, the function sets it to curr. If the last pointer is not NULL, the
            function updates its next field to point to the newly allocated block curr. The function then updates the
            metadata of the new block by setting its size to the requested size, its next pointer to NULL, and its free
            flag to false. Finally, the function increments the num_grows counter and returns a pointer to the new block
***********************************************************************************************************************/
struct _block *growHeap(struct _block *last, size_t size)
{
    // Request more space from OS
    struct _block *curr = (struct _block *)sbrk(0);                            // Get current break location
    struct _block *prev = (struct _block *)sbrk(sizeof(struct _block) + size); // Request more space

    assert(curr == prev); // Ensure that the requested memory was allocated

    // If the operating system allocation failed, return NULL
    if (curr == (struct _block *)-1)
    {
        return NULL;
    }

    // If heapList is NULL, set heapList to the current block
    if (heapList == NULL)
    {
        heapList = curr;
    }

    // Attach new block to previous block
    if (last)
    {
        last->next = curr;
    }

    // Update metadata for new block
    curr->size = size;
    curr->next = NULL;
    curr->free = false;
    num_grows++; // Increment the number of times heap was grown
    
    return curr; // Return the newly allocated block
}

/***********************************************************************************************************************
  @brief: This function represents the implementation of the memory allocation algorithm malloc used in C language. It
            takes in the size of memory requested as a parameter and returns a pointer to the beginning of the memory
            block allocated

            First, the function updates the statistics variables to track the number of memory allocations, reuse of
            existing memory blocks, and the total amount of memory requested. It then aligns the requested size to the
            next multiple of 4

            After that, it looks for a free block of memory of at least the requested size by iterating through the
            linked list of memory blocks starting from the head heapList. If it finds a block that is large enough,
            it marks it as used and returns a pointer to the memory block's data section

            If it doesn't find a free block, it attempts to grow the heap by requesting more memory from the operating
            system using the growHeap function. If this operation fails, it returns a NULL pointer indicating the
            failure to allocate memory

            If the requested size is 0, the function immediately returns NULL since no memory needs to be allocated
***********************************************************************************************************************/
void *malloc(size_t size)
{
    // If the requested size is larger than the current max heap size, update the max heap size
    if(size > num_requested_temp)
    {
        max_heap = size;
    }

    // Update the total requested size counters
    num_requested_temp = size;
    num_requested      = num_requested + num_requested_temp;

    // If atexit() has not been registered, register it to print statistics on program exit
    if( atexit_registered == 0 )
    {
        num_requested     = num_requested - 1024; // Subtract 1024 from total requested size (for unknown reason)
        atexit_registered = 1;
        atexit(printStatistics);
    }

    // Align the requested size to a multiple of 4 bytes
    size = ALIGN4(size);

    // Handle case of requesting 0 size
    if (size == 0)
    {
        printf("size = 0\n");
        return NULL;
    }

    // Search for a free block of sufficient size, starting from the beginning of the heap
    struct _block *last = heapList;
    struct _block *next = findFreeBlock(&last, size);

    // TODO: Split the free block if possible to minimize wasted space

    // If no free block was found, grow the heap to create a new block
    if (next == NULL)
    {
        next = growHeap(last, size);
    }
    else
    {
        // If a free block was found, update the reuse counter
        num_reuses++;
    }

    // If no free block could be found and the heap could not be grown, return NULL
    if (next == NULL)
    {
        return NULL;
    }

    // Mark the block as in use
    next->free = false;

    // Update the malloc counter
    num_mallocs++;

    // Return a pointer to the data section of the block
    return BLOCK_DATA(next);
}

/***********************************************************************************************************************
  @brief: This function frees the memory block pointed to by ptr. If ptr is NULL, the function returns without doing
            anything. Otherwise, it marks the block as free by setting the free flag in the _block metadata to true.
            The function also increments the num_frees counter
***********************************************************************************************************************/
void free(void *ptr)
{
    // Check if pointer is null
    if (ptr == NULL)
    {
        return;
    }

    // Get the block header associated with the pointer
    struct _block *curr = BLOCK_HEADER(ptr);

    assert(curr->free == 0); // Ensure the block is currently marked as used
    curr->free = true;       // Mark the block as free
    num_frees++;             // Increment the number of frees
}

/***********************************************************************************************************************
  @brief: This function reallocates a previously allocated block of memory. It first checks if ptr is NULL and if so, it
            calls malloc() to allocate a new block of memory of size bytes. If ptr is not NULL, it allocates a new block
            of memory nPtr of size bytes using malloc(), then copies the contents of the old block pointed to by ptr to
            the new block using memcpy(). It then deallocates the old block using free(), increments the num_reuses and
            num_frees counters, and returns a pointer to the new block
***********************************************************************************************************************/
void *realloc(void *ptr, size_t size)
{
    // Used the description on page 314 (or page 326/552) of the C99 standard pdf at the link below:
    //   http://www.open-std.org/jtc1/sc22/wg14/www/docs/n1124.pdf
    // If ptr is NULL, we treat it as if we are just doing a malloc of size 'size'
    if(ptr == NULL)
    {
        return malloc(size);
    }
    else
    {
        void *nPtr;              // create a new void pointer to store the new memory block
        nPtr = malloc(size);     // allocate memory for the new block
        memcpy(nPtr, ptr, size); // copy contents of the old memory block to the new block
        num_reuses++;            // increment reuse counter since we are reusing a block of memory
        free(ptr);               // deallocate old memory block
        num_frees++;             // increment free counter since we are freeing a block of memory
        return(nPtr);            // return the pointer to the new memory block
    }
}

/***********************************************************************************************************************
  @brief: This function allocates space for an array of size n_items * size and initializes all bits to zero. It first
            calculates the total size needed for the array using n_items and size. Then it allocates memory for the
            array using malloc and sets all bits to zero using memset. Finally, it returns a pointer to the allocated
            memory
***********************************************************************************************************************/
void *calloc(size_t n_items, size_t size)
{
    // Used the description on page 313 (or page 325/552) of the C99 standard pdf at the link below:
    // http://www.open-std.org/jtc1/sc22/wg14/www/docs/n1124.pdf

    size_t newSize = n_items * size;  // Calculate the total size of the memory block we need to allocate
    void *ptr      = malloc(newSize); // Allocate the memory block of size newSize
    memset(ptr, 0, newSize);          // Initialize all bits of the allocated memory block to zero
    return ptr;                       // Return the pointer to the allocated and initialized memory block
}

/* vim: set expandtab sts=3 sw=3 ts=6 ft=cpp: --------------------------------*/
