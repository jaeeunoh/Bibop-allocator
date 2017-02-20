#define _GNU_SOURCE 1
#define _BSD_SOURCE 1

#include <assert.h>
#include <malloc.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>

// The minimum size returned by malloc
#define MIN_MALLOC_SIZE 16

// Round a value x up to the next multiple of y
#define ROUND_UP(x,y) ((x) % (y) == 0 ? (x) : (x) + ((y) - (x) % (y)))

// The size of a single page of memory, in bytes
#define PAGE_SIZE 0x1000


size_t logbaserounder (size_t n);
size_t exponent (size_t n);
size_t roundDown (size_t x, size_t y);
void* allocatePage (size_t size);


typedef struct freelist_node {
  struct freelist_node* next;
} freelist_t; 

typedef struct header {
  size_t size;
  size_t magic_number; 
  freelist_t* freelist;
  struct header* next;
} header_t; 


// USE ONLY IN CASE OF EMERGENCY
//bool in_malloc = false;           // Set whenever we are inside malloc.
//bool use_emergency_block = false; // If set, use the emergency space for allocations
//char emergency_block[1024];       // Emergency space for allocating to print errors

// List of header pointers 
header_t* headerPointerList[8];

/**
 * Allocate space on the heap.
 * \param size  The minimium number of bytes that must be allocated
 * \returns     A pointer to the beginning of the allocated space.
 *              This function may return NULL when an error occurs.
 */
 void* xxmalloc(size_t size) {
  if (size > 2048) { 
    size = ROUND_UP(size, PAGE_SIZE);
    void* p = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
  }

  int logbase = logbaserounder (size); 
  int headerlistIndex = logbase - 4; 
  
  size = exponent(logbase); 

  if (headerPointerList[headerlistIndex] == NULL) {
    header_t* header = (header_t*) allocatePage(size);

    void* freePointer = header->freelist;
    header->freelist = header->freelist->next; 
    
    headerPointerList[headerlistIndex] = header;
    return freePointer;

  } else {
    header_t* headerPointer = headerPointerList[headerlistIndex];
    header_t* headerCopy = headerPointer; 
    while (headerCopy != NULL) {
      if (headerCopy->freelist == NULL) {
        if (headerCopy->next == NULL) {
          header_t* new_header_block = (header_t*) allocatePage(size);
          headerPointer->next = new_header_block;
          
          freelist_t* freePointer = new_header_block->freelist;
          new_header_block->freelist = new_header_block->freelist->next;
          return freePointer;
          
        } else {
          headerCopy = headerCopy->next;
        }
      } else {
        freelist_t* freePointer = headerCopy->freelist;
        headerCopy->freelist = headerCopy->freelist->next;
        return freePointer;
      }
    }
    
  }
}


void* allocatePage (size_t size) {
  // Request memory from the operating system in page-sized chunks
  void* p = mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
  intptr_t base = (intptr_t) p;
  intptr_t offset = ROUND_UP(sizeof(header_t), size);

  // Initializing header 
  header_t* header = (header_t*) p;
  header->size = size;
  header->magic_number = 1048; 
  header->next = NULL;
  header->freelist = NULL; 

  for (; offset < PAGE_SIZE; offset += size) {
    freelist_t* obj = (freelist_t*) (base + offset);
    obj->next = header->freelist;
    header->freelist = obj;
  }
  puts("this is end");


  // Check for errors
  if(p == MAP_FAILED) {
    use_emergency_block = true;
    perror("mmap");
    exit(2);
  }
  
  return header;
}



/**
 * Free space occupied by a heap object.
 * \param ptr   A pointer somewhere inside the object that is being freed
 */
 void xxfree(void* ptr) {

  size_t pageStart = roundDown((size_t) ptr, PAGE_SIZE);
  header_t* headertemp = (header_t*) pageStart;
  if (headertemp->magic_number != 1048) {
    return;
  }

  size_t objectSize = headertemp->size;
  
  size_t objectStart = roundDown ((size_t) ptr, objectSize);
  freelist_t* freeptr = (freelist_t*) objectStart;

  freeptr->next = headertemp->freelist;
  headertemp->freelist = freeptr; 
  
  return;
}


/**
 * Get the available size of an allocated object
 * \param ptr   A pointer somewhere inside the allocated object
 * \returns     The number of bytes available for use in this object
 */
 size_t xxmalloc_usable_size(void* ptr) {
  // We aren't tracking the size of allocated objects yet, so all we know is that it's at least PAGE_SIZE bytes.
  //return PAGE_SIZE;
  return 16; 
}

size_t logbaserounder (size_t n) {
  size_t leading = __builtin_clzl(n);
  size_t following = __builtin_ctzl(n);

  if ((leading + following + 1) == 8 * sizeof(size_t)) {
    return following;
  } else if ((leading + following + 1) < 8 * sizeof(size_t)) {
    return 8 * sizeof(size_t) - leading;
  }
}

// Returns 2^n
size_t exponent (size_t n) {
  return 1 <<n;
}

// Round a value x down to the next multiple of y
size_t roundDown (size_t x, size_t y) {
  size_t temp = x % y;
  return x - temp; 
} 
