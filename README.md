*README.md is still under construction*
# Memory Allocation in C

## Table of Contents
- Memory Allocation in C
  * [About the Project](#about-the-project)
  * [Compile Instructions](#compile-instructions)
  * [Meanings of the Allocations](#meanings-of-the-allocations)

## About the Project
For this program, I implemented my own algorithms for First Fit, Next Fit, Best Fit, and Worst Fit. Additionally, I have implemented my own malloc, calloc, realloc, free, and growHeap functions.


## Compile Instructions
These compile instructions are for a Ubuntu Operating System

*Compile Instructions will be added soon*
 
 ## Meanings of the Allocations
* **Best Fit**: Processes are allocated to the smallest unallocated block of memory in which they can fit, when the allocator is trying to determine which allocation is best.
* **Worst Fit**: During the memory management process, a process is allocated the largest unallocated block of memory available to it. It ensures that the biggest hold is created after the allocation, thus increasing the chances that another process will be able to make use of the remaining space compared to best fit. 
* **First Fit**: Several holes may exist in memory, so the operating system begins by allocating memory from the first space it encounters that is large enough to meet a request in order to minimize the amount of time it spends analyzing the available spaces.
* **Next Fit**: The next fit is a modified version of the first fit. It starts by finding the first free partition (like first fit), but when it calls again it continues from where it left off, not from the beginning.
* **realloc**: Attempts to resize the memory block pointed to by ptr that was previously allocated by malloc or calloc.
* **malloc**: This function attempts to allocate and return a pointer to the memory requested.
* **calloc**: Provides space for an array of nmemb objects, each with a size of size. All bits in the space are set to zero at the beginning.
* **free**: Attempts to deallocate previously allocated memory from a Calloc, malloc, or realloc.
* **growHeap**: Attempts to use sbrk() to dynamically increase the data segment of the calling process based on the requested memory size.
