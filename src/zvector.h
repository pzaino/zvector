#ifndef SRC_ZVECTOR_H_
#define SRC_ZVECTOR_H_

/*
 *    Name: Vector
 * Purpose: Library to use Dynamic Arrays (Vectors) in C Language
 *  Author: Paolo Fabio Zaino
 *  Domain: General
 * License: Copyright by Paolo Fabio Zaino, all right reserved
 *          Distributed under MIT license
 *          
 */

// First library included is zvector_checks,
// so we know on which platform and which features
// we can use:
#include "zvector_checks.h"

// Include vector configuration header
#include "zvector_config.h"

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
zvect_index vect_size(vector);
void vect_clear(vector);

#ifdef THREAD_SAFE
// Vector Thread Safe functions:

void vect_lock(vector v);

void vect_unlock(vector v);
#endif

// Vector Data Storage functions:

/*
 * vect_push and vect_pop are used to use the
 * vector as a dynamic stack.
 * 
 * vect_push(v, 3)      pushes the element 3 at 
 *                      the top of the vector
 *                      as a stack would do.
 * 
 * vect_pop(v)          pops (returns) the element 
 *                      at the top of the vector as
 *                      a regular pop would do with
 *                      a stack. Remember when you
 *                      use vect_pop the element you
 *                      receive is also removed from
 *                      the vector!
 */
void vect_push(vector, const void *);
void *vect_pop(vector);

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
void vect_add_at(vector, const void *, zvect_index);
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
void *vect_get_at(vector, zvect_index);
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
void vect_put_at(vector, const void *, zvect_index);
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
void *vect_remove_at(vector, zvect_index);
void *vect_remove_front(vector);

// Vector Data manipoulation functions:

#ifdef ZVECT_DMF_EXTENSIONS
// Data Manipoulation Functions extensions:

/*
 * vect_swap is a function that allows you to swap two
 * items in the same vector.
 * You just pass the vector and the index of both the 
 * two items.
 */
void vect_swap(vector, zvect_index, zvect_index);

#endif

#ifdef ZVECT_SFMD_EXTENSIONS
// Single Function Multiple Data extensions:

/*
 * vect_apply allows you to apply a C function to 
 * each item in the vector, so you just pass the vector, 
 * the function you want to execute against each item on
 * a vector and make sure such function is declared and
 * defined to accept a "void *" pointer which will be the
 * pointer to the single item in the vector passed to your
 * function. The function has no return value because it's
 * not necessary if you receive the pointer to the item.
 * You can simply update the content of the memory pointed
 * by the item passed and that is much faster than having
 * to deal with return values especially when you'll be
 * using complex data structures as item of the vector.
 */
void vect_apply(vector, void (*f1)(void *));

/*
 * vect_apply_if is a function that will apply "f1" C function
 * to each and every items in vector v1, IF the return value of
 * function f2 is true. So it allows what is known as conditional
 * application. f2 will receive an item from v1 as first parameter
 * and an item from v2 (at the same position of the item in v1) as
 * second parameter. So, for example, if we want to increment all
 * items in v1 of 10 if they are smaller then the corresponded item
 * in v2 then we can simply use:
 * 
 * vect_apply_if(v1, v2, increment_item, is_item_too_small);
 * 
 * and make sure we have defined 'increment_item' and
 * 'is_item_too_small' as:
 * 
 * void increment_item(void *item1)
 * {
 *  int *value = (int *)item1;
 *  *value +=10;
 * }
 * 
 * bool is_item_too_small(void *item1, void *item2)
 * {
 *  if (*((int *)item1) < *((int *)item2))
 *      return true;
 *  return false;
 * }
 */
void vect_apply_if(vector v1, vector v2, void (*f1)(void *), bool (*f2)(void *, void *));

// Operations with multiple vectors:

/*
 * vect_copy is a function that allows you to copy one
 * vector to another. Please note: only vectors with the
 * same data size (the parameter you've passed during the
 * creation of both the vectors) can be copied on in the
 * other!
 * 
 * vect_copy(v1, v2, 3, 5)      will copy all the items in
 *                              vector v2, from the 4th item
 *                              till the 9th (3 + 5, remember
 *                              vector items start from 0) in 
 *                              the vector v1. So at the end
 *                              of the process you'll have such
 *                              items copied at the end of v1.
 */
void vect_copy(vector v1, vector v2, zvect_index start, zvect_index max_elements);

/*
 * vect_move is a function that allows you to move items from
 * one vector to another. It will also re-organise the source 
 * vector and (obviously) expand the destination vector if 
 * needed. Please note: only vectors of the same data size
 * can be moved one in the other!
 * 
 * vect_move(v1, v2, 2, 2)      will move items in v2 from the
 *                              the 3rd item in v2 till the 5th
 *                              at the end of v1. 
 */ 
void vect_move(vector v1, vector v2, zvect_index start, zvect_index max_elements);

/*
 * vect_merge is a function that merges together 2 vectors
 * and, at the end of it, the source vector will be destroyed.
 * 
 * vect_merge(v1, v2)           will merge vector v2 to v1 and then
 *                              destroy v2. So at the end of the job
 *                              v1 will contains the old v1 items + 
 *                              all v2 items.
 */  
void vect_merge(vector v1, vector v2);

#endif  // ZVECT_SFMD_EXTENSIONS

#endif  // SRC_ZVECTOR_H_
