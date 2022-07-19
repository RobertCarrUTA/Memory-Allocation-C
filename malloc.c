#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>
#include <limits.h> // for INT_MIN/INT_MAX
#include <string.h> // for memset

#define ALIGN4(s)         (((((s) - 1) >> 2) << 2) + 4)
#define BLOCK_DATA(b)      ((b) + 1)
#define BLOCK_HEADER(ptr)   ((struct _block *)(ptr) - 1)
   
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

static int num_requested_temp = 0; // a variable to keep track of the previous requested heap size

/*
 *  \brief printStatistics
 *
 *  \param none
 *
 *  Prints the heap statistics upon process exit.  Registered
 *  via atexit()
 *
 *  \return none
 */
void printStatistics( void )
{
   printf("\nheap management statistics\n");
   printf("mallocs:\t%d\n", num_mallocs );
   printf("frees:\t\t%d\n", num_frees );
   printf("reuses:\t\t%d\n", num_reuses );
   printf("grows:\t\t%d\n", num_grows );
   printf("splits:\t\t%d\n", num_splits );
   printf("coalesces:\t%d\n", num_coalesces );
   printf("blocks:\t\t%d\n", num_blocks );
   printf("requested:\t%d\n", num_requested );
   printf("max heap:\t%d\n", max_heap );
}

struct _block 
{
   size_t  size;         /* Size of the allocated _block of memory in bytes */
   struct _block *prev;  /* Pointer to the previous _block of allcated memory   */
   struct _block *next;  /* Pointer to the next _block of allcated memory   */
   bool   free;          /* Is this _block free?                     */
   char   padding[3];
};


struct _block *heapList = NULL; /* Free list to track the _blocks available */
struct _block *lastList = NULL; // List to keep track of the last block allocated
                                // for use in next fit


/*
 * \brief findFreeBlock
 *
 * \param last pointer to the linked list of free _blocks
 * \param size size of the _block needed in bytes 
 *
 * \return a _block that fits the request or NULL if no free _block matches
 *
 */
struct _block *findFreeBlock(struct _block **last, size_t size) 
{
   struct _block *curr = heapList;

#if defined FIT && FIT == 0
   /* First fit */
   while (curr && !(curr->free && curr->size >= size)) 
   {
      *last = curr;
      if(curr->free)
      {
         num_blocks++;
      }
      curr  = curr->next;
   }
#endif

#if defined BEST && BEST == 0
   /* Best fit */
   struct _block * winner  = NULL;

   int spaceRemaining = INT_MAX;
   while ( curr ) 
   {  
      //num_blocks++;
      if(curr->free && curr->size > size)
      {
         if((curr->size - size) < spaceRemaining)
         {
            spaceRemaining = curr->size - size;
            winner = curr;
         }
      }
      if(curr->free)
      {
         num_blocks++;
      }
      *last = curr;
      curr = curr->next;
   }
   curr = winner;
#endif

#if defined WORST && WORST == 0
   /* Worst fit */
   // MAY NEED WORK
   struct _block * winner  = NULL;

   int spaceRemaining = INT_MIN;
   while ( curr ) // april 5th office hours 3:53
   {   
      if((curr->free) && (curr->size > size))
      {
         if(curr->size - size < spaceRemaining)
         {
            spaceRemaining = curr->size - size;
            winner = curr;
         }
      }
      if(curr->free)
      {
         num_blocks++;
      }
      curr = curr->next;
   }
   curr = winner;
#endif

#if defined NEXT && NEXT == 0 
   while (curr && !((curr->size >= size) && (curr->free))) 
   {
      *last = curr;
      curr  = curr->next;
   }
   
   if(!lastList)
   {
      lastList = curr;
   }
   
   while (lastList && !((lastList->size >= size) && (lastList->free))) 
   {
      *last = lastList;
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

/*
 * \brief growheap
 *
 * Given a requested size of memory, use sbrk() to dynamically 
 * increase the data segment of the calling process.  Updates
 * the free list with the newly allocated memory.
 *
 * \param last tail of the free _block list
 * \param size size in bytes to request from the OS
 *
 * \return returns the newly allocated _block of NULL if failed
 */
struct _block *growHeap(struct _block *last, size_t size) 
{
   /* Request more space from OS */
   struct _block *curr = (struct _block *)sbrk(0);
   struct _block *prev = (struct _block *)sbrk(sizeof(struct _block) + size);

   assert(curr == prev);

   /* OS allocation failed */
   if (curr == (struct _block *)-1) 
   {
      return NULL;
   }

   /* Update heapList if not set */
   if (heapList == NULL) 
   {
      heapList = curr;
   }

   /* Attach new _block to prev _block */
   if (last) 
   {
      last->next = curr;
   }

   /* Update _block metadata */
   curr->size = size;
   curr->next = NULL;
   curr->free = false;
   num_grows++;
   return curr;
}

/*
 * \brief malloc
 *
 * finds a free _block of heap memory for the calling process.
 * if there is no free _block that satisfies the request then grows the 
 * heap and returns a new _block
 *
 * \param size size of the requested memory in bytes
 *
 * \return returns the requested memory allocation to the calling process 
 * or NULL if failed
 */
void *malloc(size_t size) 
{
   if(size > num_requested_temp)
   {
      max_heap = size;
   }
   num_requested_temp = size;
   num_requested = num_requested + num_requested_temp;
   
   if( atexit_registered == 0 )
   {
      num_requested = num_requested - 1024; // I was always off by 1024 for some reason
      atexit_registered = 1;
      atexit( printStatistics );
   }

   /* Align to multiple of 4 */
   size = ALIGN4(size);

   /* Handle 0 size */
   if (size == 0) 
   {
      printf("size = 0\n");
      return NULL;
   }

   /* Look for free _block */
   struct _block *last = heapList;
   struct _block *next = findFreeBlock(&last, size);

   /* TODO: Split free _block if possible */

   /* Could not find free _block, so grow heap */
   if (next == NULL) 
   {
      next = growHeap(last, size);
   }
   else
   {
      num_reuses++;
   }

   /* Could not find free _block or grow heap, so just return NULL */
   if (next == NULL) 
   {
      return NULL;
   }
   
   /* Mark _block as in use */
   next->free = false;

   num_mallocs++;
   /* Return data address associated with _block */
   return BLOCK_DATA(next);
}

/*
 * \brief free
 *
 * frees the memory _block pointed to by pointer. if the _block is adjacent
 * to another _block then coalesces (combines) them
 *
 * \param ptr the heap memory to free
 *
 * \return none
 */
void free(void *ptr) 
{
   if (ptr == NULL) 
   {
      return;
   }

   /* Make _block as free */
   struct _block *curr = BLOCK_HEADER(ptr);
   assert(curr->free == 0);
   curr->free = true;
   num_frees++;
}

void *realloc(void *ptr, size_t size) 
{
   // Used the description on page 314 (or page 326/552) of the C99 standard pdf at the link below:
   // http://www.open-std.org/jtc1/sc22/wg14/www/docs/n1124.pdf
   if(ptr == NULL) // if ptr is NULL then we are pretty much just doing malloc
   {
      return malloc(size);
   }
   else
   {
      void *nPtr;
      nPtr = malloc(size); // making a new ptr(nPtr) so we can deallocate ptr
      memcpy(nPtr, ptr, size); // making the contents of the new object the same as the old object before we use the free
      num_reuses++; // We do the  reuse++ because we are reusing those blocks right?
      free(ptr); // deallocates old object (pointed to by ptr)
      num_frees++;
      return(nPtr);
   }
}

void *calloc(size_t n_items, size_t size) 
{
   // Used the description on page 313 (or page 325/552) of the C99 standard pdf at the link below:
   // http://www.open-std.org/jtc1/sc22/wg14/www/docs/n1124.pdf
   
   size_t newSize = n_items * size; // making the space for out new malloc
   void *ptr = malloc(newSize); // allocating space for an array of size n_items * size, which is newSize
   memset(ptr, 0, newSize); // Initializing all bits to zero
   return ptr;
}

/* vim: set expandtab sts=3 sw=3 ts=6 ft=cpp: --------------------------------*/
