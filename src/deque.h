#ifndef SRC_DEQUE_H
#define SRC_DEQUE_H

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define IMPLEMENT_DEQUE_STRUCT(struct_name, type)                       \
  typedef struct struct_name {                                          \
    type* data;                                                         \
    size_t cap;                                                         \
    size_t front;                                                       \
    size_t back;                                                        \
                                                                        \
    void (*destructor)(type);                                           \
  } struct_name;

#define PROTOTYPE_DEQUE(struct_name, type)                              \
  struct_name new_##struct_name(size_t);                                \
  struct_name new_destructable_##struct_name(size_t, void (*)(type));   \
  void destroy_##struct_name(struct_name*);                             \
  void empty_##struct_name(struct_name*);                               \
  bool is_empty_##struct_name(struct_name*);                            \
  size_t length_##struct_name(struct_name*);                            \
  type* as_array_##struct_name(struct_name*, size_t*);                  \
  void apply_##struct_name(struct_name*, void (*)(type));               \
  void push_front_##struct_name(struct_name*, type);                    \
  void push_back_##struct_name(struct_name*, type);                     \
  type pop_front_##struct_name(struct_name*);                           \
  type pop_back_##struct_name(struct_name*);                            \
  type peek_front_##struct_name(struct_name*);                          \
  type peek_back_##struct_name(struct_name*);                           \
  void update_front_##struct_name(struct_name*, type);                  \
  void update_back_##struct_name(struct_name*, type);                   \
  void update_and_destroy_front_##struct_name(struct_name*, type);      \
  void update_and_destroy_back_##struct_name(struct_name*, type);

#define IMPLEMENT_DEQUE(struct_name, type)                              \
                                                                        \
  void apply_##struct_name(struct_name*, void (*)(type));               \
                                                                        \
  struct_name new_##struct_name(size_t init_cap) {                      \
    struct_name ret;                                                    \
                                                                        \
    if (init_cap > 0)                                                   \
      ret.cap = init_cap;                                               \
    else                                                                \
      ret.cap = 1;                                                      \
                                                                        \
    ret.data = (type*) malloc(ret.cap * sizeof(type));                  \
                                                                        \
    if (ret.data == NULL) {                                             \
      fprintf(stderr, "ERROR: Failed to allocate struct_name"           \
              " contents");                                             \
      exit(-1);                                                         \
    }                                                                   \
                                                                        \
    ret.front = ret.back = 0;                                           \
    ret.destructor = NULL;                                              \
                                                                        \
    return ret;                                                         \
  }                                                                     \
                                                                        \
  struct_name new_destructable_##struct_name(size_t init_cap,           \
                                             void (*destructor)(type)){ \
    struct_name ret = new_##struct_name(init_cap);                      \
    ret.destructor = destructor;                                        \
    return ret;                                                         \
  }                                                                     \
                                                                        \
  void destroy_##struct_name(struct_name* deq) {                        \
    assert(deq != NULL);                                                \
                                                                        \
    if (deq->data == NULL)                                              \
      return;                                                           \
                                                                        \
    if (deq->destructor != NULL)                                        \
      apply_##struct_name(deq, deq->destructor);                        \
                                                                        \
    if (deq->data != NULL)                                              \
      free(deq->data);                                                  \
                                                                        \
    deq->data = NULL;                                                   \
    deq->cap = deq->front = deq->back = 0;                              \
  }                                                                     \
                                                                        \
  void empty_##struct_name(struct_name* deq) {                          \
    assert(deq != NULL);                                                \
    assert(deq->data != NULL);                                          \
                                                                        \
    if (deq->destructor != NULL)                                        \
      apply_##struct_name(deq, deq->destructor);                        \
                                                                        \
    deq->front = deq->back = 0;                                         \
  }                                                                     \
                                                                        \
  bool is_empty_##struct_name(struct_name* deq) {                       \
    assert(deq != NULL);                                                \
    assert(deq->data != NULL);                                          \
    return deq->front == deq->back;                                     \
  }                                                                     \
                                                                        \
  size_t length_##struct_name(struct_name* deq) {                       \
    assert(deq != NULL);                                                \
    assert(deq->data != NULL);                                          \
    return (deq->back - deq->front + deq->cap) % deq->cap;              \
  }                                                                     \
                                                                        \
  static void __reallign_##struct_name(struct_name* deq) {              \
    assert(deq != NULL);                                                \
    assert(deq->data != NULL);                                          \
                                                                        \
    if (deq->front != 0) {                                              \
      type* old_data = deq->data;                                       \
      size_t len = length_##struct_name(deq);                           \
                                                                        \
      deq->data = (type*) malloc(deq->cap * sizeof(type));              \
                                                                        \
      if (deq->data == NULL) {                                          \
        fprintf(stderr, "ERROR: Failed to reallocate struct_name"       \
                " contents");                                           \
        abort();                                                        \
      }                                                                 \
                                                                        \
      size_t i;                                                         \
                                                                        \
      for (i = 0; i < len; ++i)                                         \
        deq->data[i] = old_data[(deq->front + i) % deq->cap];           \
                                                                        \
      free(old_data);                                                   \
                                                                        \
      deq->front = 0;                                                   \
      deq->back = i;                                                    \
    }                                                                   \
  }                                                                     \
                                                                        \
  type* as_array_##struct_name(struct_name* deq, size_t* len) {         \
    assert(deq != NULL);                                                \
    assert(deq->data != NULL);                                          \
                                                                        \
    __reallign_##struct_name(deq);                                      \
                                                                        \
    type* ret = deq->data;                                              \
                                                                        \
    if (len != NULL)                                                    \
      *len = length_##struct_name(deq);                                 \
                                                                        \
    deq->data = NULL;                                                   \
    deq->cap = deq->front = deq->back = 0;                              \
                                                                        \
    return ret;                                                         \
  }                                                                     \
                                                                        \
  void apply_##struct_name(struct_name* deq, void (*func)(type)) {      \
    assert(deq != NULL);                                                \
    assert(deq->data != NULL);                                          \
                                                                        \
    size_t len = length_##struct_name(deq);                             \
                                                                        \
    for (size_t i = 0; i < len; ++i) {                                  \
      func(deq->data[(deq->front + i) % deq->cap]);                     \
    }                                                                   \
  }                                                                     \
                                                                        \
  static void __on_push_##struct_name(struct_name* deq) {               \
    if (deq->front == (deq->back + 1) % deq->cap) {                     \
      type* old_data = deq->data;                                       \
      size_t old_cap = deq->cap;                                        \
                                                                        \
      deq->cap = 2 * deq->cap;                                          \
      deq->data = (type*) malloc(deq->cap * sizeof(type));              \
                                                                        \
      if (deq->data == NULL) {                                          \
        fprintf(stderr, "ERROR: Failed to reallocate struct_name"       \
                " contents\n");                                         \
        abort();                                                        \
      }                                                                 \
                                                                        \
      size_t i;                                                         \
                                                                        \
      for (i = 0; i < old_cap - 1; ++i)                                 \
        deq->data[i] = old_data[(deq->front + i) % old_cap];            \
                                                                        \
      free(old_data);                                                   \
                                                                        \
      deq->front = 0;                                                   \
      deq->back = i;                                                    \
    }                                                                   \
  }                                                                     \
                                                                        \
  static void __on_pop_##struct_name(struct_name* deq) {                \
    if (is_empty_##struct_name(deq)) {                                  \
      fprintf(stderr, "ERROR: Cannot pop from of struct_name while it " \
              "is empty\n");                                            \
      abort();                                                          \
    }                                                                   \
  }                                                                     \
                                                                        \
  void push_front_##struct_name(struct_name* deq, type element) {       \
    assert(deq != NULL);                                                \
    assert(deq->data != NULL);                                          \
    __on_push_##struct_name(deq);                                       \
    deq->front = (deq->front + deq->cap - 1) % deq->cap;                \
    deq->data[deq->front] = element;                                    \
  }                                                                     \
                                                                        \
  void push_back_##struct_name(struct_name* deq, type element) {        \
    assert(deq != NULL);                                                \
    assert(deq->data != NULL);                                          \
    __on_push_##struct_name(deq);                                       \
    deq->data[deq->back] = element;                                     \
    deq->back = (deq->back + 1) % deq->cap;                             \
  }                                                                     \
                                                                        \
  type pop_front_##struct_name(struct_name* deq) {                      \
    assert(deq != NULL);                                                \
    assert(deq->data != NULL);                                          \
    __on_pop_##struct_name(deq);                                        \
    size_t old_front = deq->front;                                      \
    deq->front = (deq->front + 1) % deq->cap;                           \
    return deq->data[old_front];                                        \
  }                                                                     \
                                                                        \
  type pop_back_##struct_name(struct_name* deq) {                       \
    assert(deq != NULL);                                                \
    assert(deq->data != NULL);                                          \
    __on_pop_##struct_name(deq);                                        \
    deq->back = (deq->back + deq->cap - 1) % deq->cap;                  \
    return deq->data[deq->back];                                        \
  }                                                                     \
                                                                        \
  type peek_front_##struct_name(struct_name* deq) {                     \
    assert(deq != NULL);                                                \
    assert(deq->data != NULL);                                          \
    assert(!is_empty_##struct_name(deq));                               \
    return deq->data[deq->front];                                       \
  }                                                                     \
                                                                        \
  type peek_back_##struct_name(struct_name* deq) {                      \
    assert(deq != NULL);                                                \
    assert(deq->data != NULL);                                          \
    assert(!is_empty_##struct_name(deq));                               \
    return deq->data[(deq->back + deq->cap - 1) % deq->cap];            \
  }                                                                     \
                                                                        \
  void update_front_##struct_name(struct_name* deq, type element) {     \
    assert(deq != NULL);                                                \
    assert(deq->data != NULL);                                          \
    assert(!is_empty_##struct_name(deq));                               \
    deq->data[deq->front] = element;                                    \
  }                                                                     \
                                                                        \
  void update_back_##struct_name(struct_name* deq, type element) {      \
    assert(deq != NULL);                                                \
    assert(deq->data != NULL);                                          \
    assert(!is_empty_##struct_name(deq));                               \
    deq->data[(deq->back + deq->cap - 1) % deq->cap] = element;         \
  }                                                                     \
                                                                        \
  void update_and_destroy_front_##struct_name(struct_name* deq,         \
                                              type element) {           \
    assert(deq != NULL);                                                \
    assert(deq->data != NULL);                                          \
    assert(!is_empty_##struct_name(deq));                               \
                                                                        \
    size_t idx = deq->front;                                            \
                                                                        \
    if (deq->destructor != NULL)                                        \
      deq->destructor(deq->data[idx]);                                  \
                                                                        \
    deq->data[idx] = element;                                           \
  }                                                                     \
                                                                        \
  void update_and_destroy_back_##struct_name(struct_name* deq,          \
                                             type element) {            \
    assert(deq != NULL);                                                \
    assert(deq->data != NULL);                                          \
    assert(!is_empty_##struct_name(deq));                               \
                                                                        \
    size_t idx = (deq->back + deq->cap - 1) % deq->cap;                 \
                                                                        \
    if (deq->destructor != NULL)                                        \
      deq->destructor(deq->data[idx]);                                  \
                                                                        \
    deq->data[idx] = element;                                           \
  }


typedef char Type;

typedef struct Example {
  Type* data;   
  size_t cap;   
  size_t front; 
  size_t back;  

  void (*destructor)(Type); 
} Example; 
PROTOTYPE_DEQUE(Example, Type);

#endif
