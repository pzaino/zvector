/*
 *    Name: ZVector (Header)
 * Purpose: Library to use Dynamic Arrays (Vectors) in C Language
 *  Author: Paolo Fabio Zaino
 *  Domain: General
 * License: Copyright by Paolo Fabio Zaino, all rights reserved
 *          Distributed under MIT license
 *          
 */

#ifndef SRC_ZVECTOR_H_
#define SRC_ZVECTOR_H_
#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

// Requires standard C libraries:
#include <stdarg.h>

// First library included is zvector_checks,
// so we know on which platform and which features
// we can use:
#include "zvector_checks.h"

// Include vector configuration header
#include "zvector_config.h"

// Declare required structs:
typedef struct _vector *vector;

// Declare required enums:
enum {
    ZV_NONE       = 0,      // Set or Reset all vector's properties to 0.
    ZV_SAFE_WIPE  = 1 << 0, // Sets the bit to have a vector with automatic
                            // Safe Wipe.
    ZV_ENCRYPTED  = 1 << 1, // Sets the bit to have automatic data encryption
                            // decryption when stored.
    ZV_AUTOSHRINK = 1 << 2, // Sets the bit to have automatic vector shrinking.
    ZV_BYREF      = 1 << 3  // Sets the bit to have the vector storing items by
                            // reference and not copying them in a per default.
};

/*****************************
 ** Public API declaration: **
 *****************************/

// Vector construction/Destruction and memory control:

/*
 * vect_create creates and returns a new vector
 * of the specified "capacity", with a storage area that
 * can store items of "item_size" size and, if we want to 
 * have an automatic secure erasing enabled (ZV_SAFE_WIPE
 * ), we can simply pass ZV_SAFE_WIPE (or other flags too)
 * afer item_size. Flags syntax is the usual C flag sets:
 * ZV_SAFE_WIPE | ZV_AUTOSHRINK etc.
 */
vector vect_create(size_t capacity, size_t item_size, uint32_t flags);

/*
 * vect_destroy destrois the specified vector and, if 
 * secure_wipe is enabled, also ensure erasing each single
 * value in the vector before destroying it.
 */ 
void vect_destroy(vector);

/* 
 * vect_shrink is useful when operating on systems with 
 * small amount of RAM and it basically allow to shrink
 * the vector capacity to match the actual used size, to
 * save unused memory locations. 
 */
void vect_shrink(vector);

/* 
 * vect_set_wipefunct allows you to pass ZVector a pointer to a custom
 * function (of your creation) to securely wipe data from the vector v
 * when automatic safe wipe is called.
 */
void vect_set_wipefunct(vector v, void (*f1)(const void *item, size_t size));

// Vector state checks:

/*
 * vect_is_empty returns true if the vector is empty
 * and false if the vector is NOT empty.
 */ 
bool vect_is_empty(vector);

/*
 * vect_size returns the actual size (the number of)
 * USED slots in the vector storage.
 */ 
zvect_index vect_size(vector);

/* 
 * vect_clear clears out a vector and also resizes it
 * to its initial capacity.
 */
void vect_clear(vector);

#if ( ZVECT_THREAD_SAFE == 1 )
// Vector Thread Safe functions:

void vect_lock(vector v);

void vect_unlock(vector v);
#endif

/////////////////////////////////////////////////////
// Vector Data Storage functions:

/*
 * vect_push and vect_pop are used to use the
 * vector as a dynamic stack.
 * 
 * int i = 3;
 * vect_push(v, &i)     pushes the element 3 at 
 *                      the top of the vector
 *                      as a stack would do.
 */
void vect_push(vector, const void *);

/*
 * vect_pop(v)          pops (returns) the element 
 *                      at the top of the vector as
 *                      a regular pop would do with
 *                      a stack. Remember when you
 *                      use vect_pop the element you
 *                      receive is also removed from
 *                      the vector!
 */
void *vect_pop(vector);

/*
 * vect_add adds a new item into the vector and,
 * if required, will also reorganize the vector.
 * 
 * int i = 3;
 * vect_add(v, &i)       will add the new item 3
 *                       into the vector at the end
 *                       of the v vector.
 * 
 * int i = 4;
 * vect_add_at(v, &i, 2) will add the new item 4
 *                       at position 2 in the v
 *                       vector and move all the
 *                       items from the original 2nd
 *                       onward of a position to make 
 *                       space for the new item 4.
 * 
 * int i = 5;
 * vect_add_front(v, &i) will add the new item 5
 *                       at the beginning of the vector 
 *                       v and will also move all the
 *                       existsing elements of one
 *                       position in the vector to make
 *                       space for the new item 5 at the
 *                       beginning of v.
 */
void vect_add(vector, const void *);
void vect_add_at(vector, const void *, zvect_index);
void vect_add_front(vector, const void *);

/*
 * vect_get returns an item from the specified vector
 *
 * vect_get(v)          will return the last element in
 *                      the v vector (but will not touch
 *                      the element as it happens in
 *                       vect_pop(v)).
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
 * 
 * int i = 3;
 * vect_put(v, &i)       will replace the last element
 *                       in the vector with 3.
 * 
 * int i = 4;
 * vect_put_at(v, &i, 2) will replace the 3rd element
 *                       (2 + 1, as vector's 1st item
 *                       starts at v[0]) with the
 *                       item 4.
 * 
 * int i = 5;
 * vect_put_front(v, &i) will replace the 1st element
 *                       of the vector with the item 
 *                       5.
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
 * vect_remove(v)       will remove and return the
 *                      last item in the vector.
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

/* 
 * vect_delete deletes an item from the vector
 * and reorganise the vector. It does not return
 * the item like remove.
 * 
 * vect_delete(v)       will delete and the last 
 *                      item in the vector.
 * 
 * vect_delete_at(v, 3) will delete the 3rd item in
 *                      the vector.
 *
 * vect_delete_front(v) will delete the 1st item in
 *                      the vector.
 */
void vect_delete(vector);
void vect_delete_at(vector, zvect_index);
void vect_delete_range(vector v, zvect_index first_elemeny, zvect_index last_element);
void vect_delete_front(vector);

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
