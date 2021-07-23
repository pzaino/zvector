#ifndef ZFP_VECTOR_H
#define ZFP_VECTOR_H

/*
 *    Name: Vector
 * Purpose: Library to use Dynamic Arrays (Vectors) in C Language
 *  Author: Paolo Fabio Zaino
 *  Domain: General
 * License: Copyright by Paolo Fabio Zaino, all right reserved
 *          Distributed under MIT license
 *          
 */

// Include vector configuration header
#include "config.h"

// Declare required structs:
typedef struct _vector *vector;

/*****************************
 ** Public API declaration: **
 *****************************/

// Vector contruction/Destruction and memory control:

/*
 * vect_create creates and returns a new vector
 * of the specified "capacity", with a storage area that
 * can store items of "item_size" size and if we want to 
 * have an automatic secure erasing enabled (secure_wipe
 * = true) or disabled (secure_wipe = false).
 * 
 * vect_destroy destroies the specified vector and, if 
 * secure_wipe is enabled, also ensure erasing each single
 * value in the vector before destroying it.
 * 
 * vect_shrink is useful when operating on systems with 
 * small amount of RAM and it basically allow to shrink
 * the vector capacity to match the actual used size, to
 * same unused memory locations. 
 */
vector vect_create(size_t capacity, size_t item_size, bool secure_wipe);
void vect_destroy(vector);
void vect_shrink(vector);

// Vector state checks:

/*
 * vect_is_empty returns true if the vector is empty
 * and false if the vector is NOT empty.
 * 
 * vect_size returns the actual size (the number of)
 * USED slots in the vector storage.
 * 
 * vect_clear clears out a vector and also resizes it
 * to its initial capacity.
 */
bool vect_is_empty(vector);
index_int vect_size(vector);
void vect_clear(vector);

// Vector Data Storage functions:

/*
 * vect_add adds a new item into the vector and,
 * if required, will also reorganize the vector.
 * 
 * vect_add(v, 3)       will add the new item 3
 *                      into the vector at the end
 *                      of the v vector.
 * 
 * vect_add_at(v, 4, 2) will add the new item 4
 *                      at position 2 in the v
 *                      vector and move all the
 *                      items from the original 2nd
 *                      onward of a position to make 
 *                      space for the new item 4.
 * 
 * vect_add_front(v, 5) will add the new item 5
 *                      at the beginning of the vector 
 *                      v and will also move all the
 *                      existsing elements of one
 *                      position in the vector to make
 *                      space for the new item 5 at the
 *                      beginning of v.
 */
void vect_add(vector, const void *);
void vect_add_at(vector, const void *, index_int);
void vect_add_front(vector, const void *);

/*
 * vect_get returns an item from the specified vector
 *
 * vect_get(v)          will return the last element in
 *                      the v vector.
 * 
 * vect_get_at(v, 3)    will return the element at location
 *                      3 in the vector v.
 * 
 * vect_get_front(v)    will return the first element in
 *                      the vector v. 
 */
void *vect_get(vector);
void *vect_get_at(vector, index_int);
void *vect_get_front(vector);

/* 
 *vect_put allows you to REPLACE an item
 * in the vector.
 * vect_put(v, 3)       will replace the last element
 *                      in the vector with 3.
 * 
 * vect_put_at(v, 4, 2) will replace the 3rd element
 *                      (2 + 1, as vector's 1st item
 *                      starts at v[0]) with the
 *                      item 4.
 * 
 * vect_put_front(v, 5) will replace the 1st element
 *                      of the vector with the item 
 *                      5.
 */
void vect_put(vector, const void *);
void vect_put_at(vector, const void *, index_int);
void vect_put_front(vector, const void *);

/* 
 * vect_remove removes an item from the vector
 * and reorganise the vector. It also returns
 * the item remove from the vector, so you can
 * use it to simulate a stack behaviour as well.
 * 
 * vect_remove(v)       will remove and return
 *                      the last item in the vector
 * 
 * vect_remove_at(v, 3) will remove the 3rd item in
 *                      the vector and return it.
 *
 * vect_remove_front(v) will remove the 1st item in
 *                      the vector and return it.
 */
void *vect_remove(vector);
void *vect_remove_at(vector, index_int);
void *vect_remove_front(vector);

// Vector Data manipoulation functions:

/*
 * vect_apply allows you to apply a C function to 
 * each item in the vector, so you just pass the vector, 
 * the function you want to execute against each item on
 * a vector and make sure such function is declared and
 * defined to accept a "void *" pointer which will be the
 * pointer to the single item in the vector passed to your
 * function.
 */
void vect_apply(vector, void (*f)(void *));

/*
 * vect_swap is a function that allows you to swap two
 * items in the same vector.
 * You just pass the vector and the index of both the 
 * two items.
 */
void vect_swap(vector, index_int, index_int);

#endif // ZFP_VECTOR_H
