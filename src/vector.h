#ifndef ZFP_VECTOR_H
#define ZFP_VECTOR_H

// Include some standard C lib header
#include <stdint.h>
#include <stdbool.h>

// Include vector configuration header
#include "config.h"

// Declare required structs:
typedef uint32_t index_int;

typedef struct _vector *vector;
typedef int32_t value_type;

// Public API declaration:

// Vector contruction/Destruction and memory control:
vector vect_create(size_t, size_t, bool);
void vect_destroy(vector);
void vect_shrink(vector);

// Vector state checks:
bool vect_is_empty(vector);
index_int vect_size(vector);
void vect_clear(vector);

// Vector Data Storage functions:
void vect_add(vector, const void *);
void vect_add_at(vector, const void *, index_int);
void vect_add_front(vector, const void *);

void *vect_get(vector);
void *vect_get_at(vector, index_int);
void *vect_get_front(vector);

void vect_put(vector, const void *);
void vect_put_at(vector, const void *, index_int);
void vect_put_front(vector, const void *);

void *vect_remove(vector);
void *vect_remove_at(vector, index_int);
void *vect_remove_front(vector);

// Vector Data manipoulation functions:
void vect_apply(vector, void (*f)(void *));
void vect_swap(vector, index_int, index_int);

#endif // ZFP_VECTOR_H
