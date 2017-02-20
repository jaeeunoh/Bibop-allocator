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
  freelist_t* freelist;
  struct header* next;
} header_t; 


// USE ONLY IN CASE OF EMERGENCY
bool in_malloc = false;           // Set whenever we are inside malloc.
bool use_emergency_block = false; // If set, use the emergency space for allocations
char emergency_block[1024];       // Emergency space for allocating to print errors

// List of header pointers 
header_t* headerPointerList[8];

/**
 * Allocate space on the heap.
 * \param size  The minimium number of bytes that must be allocated
 * \returns     A pointer to the beginning of the allocated space.
 *              This function may return NULL when an error occurs.
 */
 void* xxmalloc(size_t size) {
  int logbase = logbaserounder (size); 
  int headerlistIndex = logbaserounder (size) - 4; 
  
  //return (void*)-1;
  
  // Before we try to allocate anything, check if we are trying to print an error or if
  // malloc called itself. This doesn't always work, but sometimes it helps.
  /*
  if(use_emergency_block) {
    return emergency_block;
  } else if(in_malloc) {
    use_emergency_block = true;
    puts("ERROR! Nested call to malloc. Aborting.\n");
    exit(2);
  }

  
  // If we call malloc again while this is true, bad things will happen.
  in_malloc = true;

  */
  
  // Round the size up to the next multiple of the page size
  //size = ROUND_UP(size, PAGE_SIZE);



  
  if (headerPointerList[headerlistIndex] == NULL) {
    header_t* header = (header_t*) allocatePage(size);

    void* freePointer = header->freelist;
    header->freelist = header->freelist->next; 
    
    headerPointerList[headerlistIndex] = header;

    return freePointer;

  } else {
    header_t* headerPointer = headerPointerList[headerlistIndex];
    header_t* pagePointer = headerPointer; 
    while (pagePointer != NULL) {
      if (pagePointer->freelist == NULL) {
        if (pagePointer->next == NULL) {
          header_t* header = allocatePage(size);
          pagePointer->next = header;
          
          freelist_t* freeSpace = header->freelist;
          header->freelist = header->freelist->next;
          return freeSpace;
          
        } else {
          pagePointer = pagePointer->next;
        }
      } else {
        freelist_t* freeSpace = pagePointer->freelist;
        pagePointer->freelist = pagePointer->freelist->next;
        return freeSpace;
      }
    }
    
  }
  
  // Done with malloc, so clear this flag
 // in_malloc = false;
}


void* allocatePage (size_t size) {
  int logbase = logbaserounder (size); 
  int headerlistIndex = logbaserounder (size) - 4; 
  
  // Request memory from the operating system in page-sized chunks
  void* p = mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
  intptr_t base = (intptr_t) p;
  intptr_t offset = ROUND_UP(sizeof(header_t), exponent(logbase));

  // Initializing header 
  header_t* header = (header_t*) p;
  header->size = size;
  header->next = NULL;
  header->freelist = NULL; 

  for (; offset < PAGE_SIZE; offset += exponent(logbase)) {
    char buff[256];
    size_t expo = exponent(logbase);
    snprintf(buff, 256, "expo %i and logbase is %i\n", expo, logbase);
    fputs(buff, stderr);

    puts("Start of loop __________________________"); 

    freelist_t* obj = (freelist_t*) (base + offset);
    obj->next = NULL; 
    
    char buf[256];
    snprintf(buf, 256, "obj is %p\theader is %p\n", obj, header);
    fputs(buf, stderr);
    snprintf(buf, 256, "obj->next is %p\thead->next is %p\n", obj->next, header->next);
    fputs(buf, stderr);
    snprintf(buf, 256, "head->freelist is %p\n", header->freelist);
    fputs(buf, stderr);

    puts("this is where obj->next happens **********************************"); 
    obj->next = header->freelist;
    snprintf(buf, 256, "obj->next is %p\thead->next is %p\n", obj->next, header->next);
    fputs(buf, stderr);
    header->freelist = obj;

    snprintf(buf, 256, "head->free should be object  %p\n", header->freelist);
    fputs(buf, stderr);

    snprintf(buf, 256, "head->free->next is %p\n", header->freelist->next);
    fputs(buf, stderr);
  }

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
  void* temp = ptr;

  size_t intTmp = (size_t) temp;
  temp -= (intTmp - pageStart);
  header_t* headertemp = (header_t*) temp;
  size_t objectSize = headertemp->size;
  
  size_t objectStart = roundDown ((size_t) ptr, objectSize);
  size_t intPtr = (size_t) ptr;
  ptr -= (intPtr - objectStart);
  freelist_t* freeptr = (freelist_t*) ptr;

  freeptr->next = headertemp->freelist;
  headertemp->freelist = freeptr; 
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

  if ((leading + following + 1) == 8*sizeof(size_t)) {
    return following;
  } else if ((leading + following + 1) < 8*sizeof(size_t)) {
    return 8*sizeof(size_t) - leading;
  }
}

size_t exponent (size_t n) {
  return 1 <<n;
}

size_t roundDown (size_t x, size_t y) {
  size_t temp = x % y;
  return (temp - 1)*x;
} 
