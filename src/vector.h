#ifndef ZFP_VECTOR_H
#define ZFP_VECTOR_H

#include <stdint.h>
#include <stdbool.h>

typedef uint32_t index_int;

typedef struct _vector *vector;
typedef int32_t value_type;

vector vector_create(size_t, size_t, bool);
void vector_destroy(vector);

void *vector_get(vector, index_int);
void vector_put(vector, index_int, void *);

void vector_add(vector, void *const);

void vector_add_at(vector, index_int, void *);
void *vector_remove_at(vector, index_int);

bool vector_is_empty(vector);
index_int vector_size(vector);
void vector_clear(vector);

#endif // ZFP_VECTOR_H
