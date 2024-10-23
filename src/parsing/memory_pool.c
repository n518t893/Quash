#include "memory_pool.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "deque.h"


typedef struct MemoryPool {
  void* pool;  
  size_t size; 
  void* next;  
} MemoryPool;

IMPLEMENT_DEQUE_STRUCT(MemoryPoolDeque, MemoryPool);
IMPLEMENT_DEQUE(MemoryPoolDeque, MemoryPool);

static MemoryPoolDeque pool_deq = { NULL, 0, 0, 0, NULL };

static MemoryPool __initialize_memory_pool(size_t size) {
  void* mem;

  if (size == 0) {
    mem = NULL;
  }
  else {
    if ((mem = malloc(size)) == NULL)
      size = 0;
  }

  return (MemoryPool) {
    mem,
    size,
    mem
  };
}


static MemoryPool __low_memory_initialize_memory_pool(size_t required_size,
                                                      size_t failed_requested_size) {
  while(true) {
    if (failed_requested_size <= required_size) {
      fprintf(stderr, "ERROR: Unable to allocate more memory to the memory pool.\n");
      exit(-1);
    }

    failed_requested_size >>= 1;

    if (failed_requested_size < required_size)
      failed_requested_size = required_size;

    
    MemoryPool ret = __initialize_memory_pool(failed_requested_size);

    if (ret.pool != NULL)
      return ret;
  }
}

static void __destroy_memory_pool(MemoryPool mp) {
  if (mp.pool != NULL)
    free(mp.pool);
  mp.pool = NULL;
}

void initialize_memory_pool(size_t size) {
  if (size == 0)
    size = 1;

  pool_deq = new_destructable_MemoryPoolDeque(10, __destroy_memory_pool);

  MemoryPool pool = __initialize_memory_pool(size);

  if (pool.pool == NULL)
    
    pool = __low_memory_initialize_memory_pool(1, size);

  push_back_MemoryPoolDeque(&pool_deq, pool);
}

void* memory_pool_alloc(size_t size) {
  assert(!is_empty_MemoryPoolDeque(&pool_deq));

  MemoryPool pool = peek_back_MemoryPoolDeque(&pool_deq);
  size_t init_size = peek_front_MemoryPoolDeque(&pool_deq).size;

  assert(pool.pool != NULL);
  assert(pool.size != 0);
  assert(pool.next != NULL);

  while (pool.next - pool.pool + size > pool.size) {
    size_t length_pool_deq = length_MemoryPoolDeque(&pool_deq);
    size_t new_pool_size = init_size * (2 << (length_pool_deq - 1));

    if (new_pool_size < size) {
      pool = __initialize_memory_pool(0);
    }
    else {
      
      pool = __initialize_memory_pool(new_pool_size);

      if (pool.pool == NULL)
        
        pool = __low_memory_initialize_memory_pool(size, new_pool_size);
    }

    push_back_MemoryPoolDeque(&pool_deq, pool);
  }

  assert(pool.next == peek_back_MemoryPoolDeque(&pool_deq).next);
  void* ret = pool.next;
  pool.next += size;

 
  update_back_MemoryPoolDeque(&pool_deq, pool);

  return ret;
}


void destroy_memory_pool() {
  destroy_MemoryPoolDeque(&pool_deq);
}


char* memory_pool_strdup(const char* str) {
  assert(str != NULL);

  size_t len = strlen(str) + 1;
  char* ret = memory_pool_alloc(len);

  strcpy(ret, str);

  return ret;
}
