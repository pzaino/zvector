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

#ifdef _cplusplus
extern "C" {
#endif

// Requires standard C libraries:
#include <stdlib.h>
#include <stdarg.h>

// First library included is zvector_checks,
// so we know on which platform and which features
// we can use:
#include "zvector_checks.h"
#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#	pragma once
#endif

// Include vector configuration header
#include "zvector_config.h"

// Declare required structs:
typedef struct p_vector * vector;
typedef struct p_vector const * const_vector;

#if defined(ZVECT_COOPERATIVE)
// Cooperative Scheduler support
typedef struct p_cmt_state * cmt_state;
typedef struct p_cmt_state_range cmt_state_range;
#endif


// Declare required enums:

/*
 * Vector Properties Flags can be used to tell ZVector
 * which types of properties we want to enable for each
 * given vector we are creating.
 * Each vector can have multiple properties enabled at the
 * same time, you can use the typical C form to specify multiple
 * properties for the same vector:
 *
 * ZV_SEC_WIPE | ZV_BYREF
 *
 * The above will create a vector that supports passing items to
 * it by reference (instead of copying them into the vector) and
 * having Secure Wipe enabled, so that when an element is deleted
 * its reference will also be fully zeroed out before freeing it.
 */
enum ZVECT_PROPERTIES {
	ZV_NONE       = 0,      // Sets or Resets all vector's properties to 0.
	ZV_SEC_WIPE   = 1 << 0, // Sets the vector for automatic Secure Wipe of items.
	ZV_BYREF      = 1 << 1, // Sets the vector to store items by reference instead of copying them as per default.
	ZV_CIRCULAR   = 1 << 2, // Sets the vector to be a circular vector (so it will not grow in capacity automatically). Elements will be overwritten as in typical circular buffers!
	ZV_NOLOCKING  = 1 << 3, // This Property means the vector will not use mutexes, be careful using it!
};

enum ZVECT_ERR {
	ZVERR_VECTUNDEF     = -1,
	ZVERR_IDXOUTOFBOUND = -2,
	ZVERR_OUTOFMEM      = -3,
	ZVERR_VECTCORRUPTED = -4,
	ZVERR_RACECOND      = -5,
	ZVERR_VECTTOOSMALL  = -6,
	ZVERR_VECTDATASIZE  = -7,
	ZVERR_VECTEMPTY     = -8,
	ZVERR_OPNOTALLOWED  = -9
};

extern unsigned int LOG_PRIORITY;

/*****************************
 ** Public API declaration: **
 *****************************/

// Vector construction/Destruction and memory control:

/*
 * vect_create creates and returns a new vector
 * of the specified "capacity", with a storage area that
 * can store items of "item_size" size and, if we want to
 * have an automatic secure erasing enabled (ZV_SEC_WIPE
 * ), we can simply pass ZV_SAFE_WIPE (or other flags too)
 * after item_size. Flags syntax is the usual C flag sets:
 * ZV_SEC_WIPE | ZV_BYREF etc.
 */
vector vect_create(zvect_index capacity, size_t item_size, uint32_t properties);

/*
 * vect_destroy destroys the specified vector and, if
 * secure_wipe is enabled, also ensure erasing each single
 * value in the vector before destroying it.
 */
void *vect_destroy(vector);

/*
 * vect_shrink is useful when operating on systems with
 * small amount of RAM, and it basically allows to shrink
 * the vector capacity to match the actual used size, to
 * save unused memory locations.
 */
void vect_shrink(vector const v);
#define vect_shrink_to_fit(x) vect_shrink(x)

/*
 * vect_set_wipefunct allows you to pass ZVector a pointer to a custom
 * function (of your creation) to securely wipe data from the vector v
 * when automatic safe wipe is called.
 */
void vect_set_wipefunct(vector const v, void (*f1)(const void *item, size_t size));

// Vector state checks:

/*
 * vect_is_empty returns true if the vector is empty
 * and false if the vector is NOT empty.
 */
bool vect_is_empty(const_vector const v);

/*
 * vect_size returns the actual size (the number of)
 * USED slots in the vector storage.
 */
zvect_index vect_size(const_vector const v);

/*
 * vect_size returns the maximum size (the max number of)
 * slots in the vector storage.
 */
zvect_index vect_max_size(const_vector const v);

void *vect_begin(const_vector const v);
void *vect_end(const_vector const v);

/*
 * vect_clear clears out a vector and also resizes it
 * to its initial capacity.
 */
void vect_clear(vector const v);

/*
 * Vector status bits control
 */
bool vect_check_status(const_vector const v, zvect_index flag_id);

bool vect_set_status(const vector v, zvect_index flag_id);

bool vect_clear_status(const vector v, zvect_index flag_id);

bool vect_toggle_status(const vector v, zvect_index flag_id);

#if ( ZVECT_THREAD_SAFE == 1 )
// Vector Thread Safe functions:

/*
 * vect_lock_enable allows you to enable thread safe
 * code at runtime. It doesn't lock anything, it just
 * enables globally ZVector thread safe code at
 * runtime.
 *
 * Example of use:
 * vect_lock_enable;
 */
void vect_lock_enable(void);

/*
 * vect_lock_disable allows you to disable thread safe
 * code at runtime. It doesn't lock anything, it just
 * disables globally ZVector thread safe code at
 * runtime.
 *
 * Example of use:
 * vect_lock_disable;
 */
void vect_lock_disable(void);

/*
 * vect_lock allows you to lock the given vector to
 * have exclusive write access from your own thread.
 * When you lock a vector directly then ZVector will
 * NOT use its internal locking mechanism for that
 * specific vector.
 *
 * Example of use: To lock a vector called v
 * vect_lock(v);
 */
zvect_retval vect_lock(vector const v);

/*
 * vect_trylock will try to lock the given vector to
 * have exclusive write access from your own thread.
 * When you lock a vector directly then ZVector will
 * NOT use its internal locking mechanism for that
 * specific vector.
 *
 * Example of use: To lock a vector called v
 * vect_trylock(v);
 */
zvect_retval vect_trylock(vector const v);

/*
 * vect_lock allows you to unlock the given vector that
 * you have previously locked with vect_lock.
 *
 * Example of use: To unlock a vector called v
 * vect_unlock(v);
 */
zvect_retval vect_unlock(vector const v);

/* TODO:
 * implement zvect_retval vect_wait_for_signal(const vector v);
 *
 * implement zvect_retval vect_lock_after_signal(const vector v);
 */

zvect_retval vect_move_on_signal(vector const v1, vector v2,
				 const zvect_index s2,
				 const zvect_index e2,
				 zvect_retval (*f2)(void *, void *));

zvect_retval vect_send_signal(const vector v);

zvect_retval vect_broadcast_signal(const vector v);

zvect_retval vect_sem_wait(const vector v);

zvect_retval vect_sem_post(const vector v);

#endif  // ( ZVECT_THREAD_SAFE == 1 )

/////////////////////////////////////////////////////
// Vector Data Storage functions:

/*
 * vect_push and vect_pop are used to use the
 * vector as a dynamic stack.
 *
 * int i = 3;
 * vect_push(v, &i)     pushes the element  3 at  the
 *                      back of the  vector  v, which
 * 			corresponds  to  the top of a
 * 			Stack.
 */
void vect_push(vector const v, const void *item);

/*
 * vect_pop(v)          "pops" (returns) the  element
 *                      at the back of the  vector as
 *                      a regular  pop would  do with
 *                      an  element at  the top  of a
 *                      stack. Remember when  you use
 *                      vect_pop the element you
 *                      receive is also removed from
 *                      the vector!
 */
void *vect_pop(vector const v);
#define vect_pop_back(x) vect_pop(x)

/*
 * vect_add adds a new item into the vector and,
 * if required, will also reorganize the vector.
 *
 * int i = 3;
 * vect_add(v, &i)       will add the new item  3 in
 *                       the vector  v  at  the  end
 *                       (or back) of the  vector v.
 */
void vect_add(vector const v, const void *item);
#define vect_push_back(x, y) vect_add(x, y)

/*
 * int i = 4;
 * vect_add_at(v, &i, 2) will add the  new  item 4 at
 *                       position  2 in the  vector v
 *                       and move  all  the  elements
 * 			 from the original 2nd onward
 * 			 of a position to  make space
 * 			 for the new item 4.
 */
void vect_add_at(vector const v, const void *item, zvect_index index);

/*
 * int i = 5;
 * vect_add_front(v, &i) will add the new  item  5 at
 * 			 the beginning  of the vector
 *                     	 v  (or front)  and will also
 * 			 move   all   the   existing
 * 			 elements of one position  in
 * 			 the vector to make space for
 * 			 the new item 5 at  the front
 *			 of vector v.
 */
void vect_add_front(vector const v, const void *item);
#define vect_push_front(x, y) vect_add_front(x, y)

/*
 * vect_get returns an item from the specified vector
 *
 * vect_get(v)          will  return  the  ast element in
 *                      the v vector (but will not remove
 *                      the  element  as  it  happens  in
 *                      vect_pop(v)).
 */
void *vect_get(const_vector const v);
#define vect_back(v)  vect_get(v)

/*
 *
 * vect_get_at(v, 3)    will return the element at location
 *                      3 in the vector v.
 */
void *vect_get_at(const_vector const v, const zvect_index i);
#define vect_at(v, x)  vect_get_at(v, x)

/*
 * vect_get_front(v)    will return the first element in
 *                      the vector v.
 */
void *vect_get_front(const_vector const v);
#define vect_front(v)  vect_get_front(v)

/*
 *vect_put allows you to REPLACE an item
 * in the vector.
 *
 * int i = 3;
 * vect_put(v, &i)       will replace the last element
 *                       in the vector with 3.
 */
void vect_put(vector const v, const void *item);

/*
 *
 * int i = 4;
 * vect_put_at(v, &i, 2) will replace the 3rd element
 *                       (2 + 1, as vector's 1st item
 *                       starts at v[0]) with the
 *                       item 4.
 */
void vect_put_at(vector const v, const void *item, const zvect_index i);

/*
 *
 * int i = 5;
 * vect_put_front(v, &i) will replace the 1st element
 *                       of the vector with the item
 *                       5.
 */
void vect_put_front(vector const v, const void *item);

/*
 * vect_remove removes an item from the vector
 * and reorganize the vector. It also returns
 * the item remove from the vector, so you can
 * use it to simulate a stack behaviour as well.
 *
 * vect_remove(v)       will remove and return the
 *                      last item in the vector.
 */
void *vect_remove(vector const v);

/*
 * vect_remove_at(v, 3) will remove the 3rd item in
 *                      the vector and return it.
 */
void *vect_remove_at(vector const v, const zvect_index i);

/*
 * vect_remove_front(v) will remove the 1st item in
 *                      the vector and return it.
 */
void *vect_remove_front(vector const v);

/*
 * vect_delete deletes an item from the vector
 * and reorganize the vector. It does not return
 * the item like remove.
 *
 * vect_delete(v)       will delete and the last
 *                      item in the vector.
 */
void vect_delete(vector const v);

/*
 * vect_delete_at(v, 3) will delete the 3rd item in
 *                      the vector.
 */
void vect_delete_at(vector const v, const zvect_index i);

/*
 * vect_delete_range(v, 20, 30)
 *                      will delete items from item
 *                      20 to item 30 in the vector
 *                      v.
 */
void vect_delete_range(vector const v, const zvect_index first_element, const zvect_index last_element);

/*
 *
 * vect_delete_front(v) will delete the 1st item in
 *                      the vector.
 */
void vect_delete_front(vector const v);

////////////
// Vector Data manipulation functions:
////////////

#ifdef ZVECT_DMF_EXTENSIONS
// Data Manipulation Functions extensions:

/*
 * vect_swap is a function that allows you to swap two
 * items in the same vector.
 * You just pass the vector and the index of both the
 * two items to swap.
 *
 * For example to swap item 3 with item 22 on vector v
 * use:
 * vect_swap(v, 3, 22);
 */
void vect_swap(vector const v, const zvect_index s, const zvect_index e);

/*
 * vect_swap_range is a function that allows to swap
 * a range of items in the same vector.
 * You just pass the vector, the index of the first item
 * to swap, the index of the last item to swap and the
 * index of the first item to swap with.
 *
 * For example to swap items from 10 to 20 with items
 * from 30 to 40 on vector v, use:
 * vect_swap_range(v, 10, 20, 30);
 */
void vect_swap_range(vector const v, const zvect_index s1, const zvect_index e1, const zvect_index s2);

/*
 * vect_rotate_left is a function that allows to rotate
 * a vector of "i" positions to the left (or from the
 * "front" to the "end").
 *
 * For example to rotate a vector called v of 5 positions
 * to the left, use:
 * vect_rotate_left(v, 5);
 */
void vect_rotate_left(vector const v, const zvect_index i);

/*
 * vect_rotate_right is a function that allows to rotate
 * a vector of "i" positions to the right (or from the
 * "end" to the "front").
 *
 * For example to rotate a vector called v of 5 positions
 * to the right, use:
 * vect_rotate_right(v, 5);
 */
void vect_rotate_right(vector const v, const zvect_index i);

/*
 * vect_qsort allows you to sort a given vector.
 * The algorithm used to sort a vector is Quicksort with
 * 3 ways partitioning which is generally much faster than
 * traditional quicksort.
 *
 * To sort a vector you need to provide a custom function
 * that allows vect_sort to determine the order and which
 * elements of a vector are used to order it in the way
 * you desire. It pretty much works as a regular C qsort
 * function. It quite fast given that it only reorders
 * pointers to your datastructures stored in the vector.
 *
 */
#if !defined(ZVECT_COOPERATIVE)
void vect_qsort(vector const v, int (*compare_func)(const void *, const void*));
#else
void vect_qsort(vector const v, int (*compare_func)(const void *, const void*),
		cmt_state const state,
		zvect_index maxIterations);
#endif


/*
 * vect_bsearch is a function that allows to perform
 * a binary  search over the vector we pass to it to
 * find the item "key" using the comparison function
 * "f1".
 *
 * The specific algorithm used to implement vect_bsearch
 * if my own re-implementation of the Adaptive Binary
 * Search algorithm (from Igor van den Hoven) which has
 * some improvements over the original one (look at the
 * sources for more details).
 *
 * For example to search for the number 5 in a vector
 * called v using a compare function called "my_compare"
 * use:
 * int i = 5;
 * vect_bsearch(v, &i, my_compare);
 */
bool vect_bsearch(vector const v, const void *key, int (*f1)(const void *, const void *), zvect_index *item_index);

/*
 * vect_lsearch is a function that allows to perform
 * traditional linear search over an ordered or not
 * ordered vector. If the vector size is a multiple of
 * 2 the fuction optimize the traditional loop using
 * 4 unrolled consecutive searches, while if the vector
 * size is odd it uses traditional loop (for now).
 *
 * It finds the item "key" using the comparison function
 * "f1", which has to be provided by the user as a pointer
 * to a user-written comparison function that accept the
 * vector's items (as defined by the user).
 *
 * For example to search for the number 5 in a vector
 * called v using a compare function called "my_compare"
 * use:
 * int i = 5;
 * vect_lsearch(v, &i, my_compare);
 */
#if !defined(ZVECT_COOPERATIVE)
bool vect_lsearch(vector const v, const void *key,
                  int (*f1)(const void *, const void *),
                  zvect_index *item_index);
#else
bool vect_lsearch(vector const v, const void *key,
		  int (*f1)(const void *, const void *),
		  zvect_index *item_index,
		  cmt_state state,
		  zvect_index maxIterations);
#endif

/*
 * vect_add_ordered allows the insertion of new items in
 * an ordered fashion. Please note that for this to work
 * fine you should always use only ordered vectors or if
 * an empty vector use vect_add_ordered only to add new
 * values to it!
 *
 * As for any other ordered function you must provide
 * your own compare function (syntax is the usual one,
 * and it's the same as for regular CLib qsort function)
 *
 * To add item 3 to a vector called v using vect_add_ordered
 * (assuming your compare function is called my_compare),
 * use:
 *
 * vect_Add_ordered(v, 3, my_compare);
 */
void vect_add_ordered(vector const v, const void *value, int (*f1)(const void *, const void *));

#endif  // ZVECT_DMF_EXTENSIONS

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
#if !defined(ZVECT_COOPERATIVE)
void vect_apply(vector const v, void (*f1)(void *));
#else
void vect_apply(vector const v,
		void (*f)(void *),
		cmt_state const state,
		zvect_index maxIterations);
#endif

/*
 * vect_apply_if is a function that will apply "f1" C function
 * to each and every item in vector v1, IF the return value of
 * function f2 is true. So it allows what is known as conditional
 * application. f2 will receive an item from v1 as first parameter
 * and an item from v2 (at the same position of the item in v1) as
 * second parameter. So, for example, if we want to increment all
 * items in v1 of 10 if they are smaller than the corresponded item
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
#if !defined(ZVECT_COOPERATIVE)
void vect_apply_if(vector const v1, vector const v2, void (*f1)(void *), bool (*f2)(void *, void *));
#else
void vect_apply_if(vector const v1, vector const v2,
		   void (*f1)(void *),
		   bool (*f2)(void *, void *),
		   cmt_state const state,
		   zvect_index maxIterations);
#endif

void vect_apply_range(vector const v, void (*f)(void *), const zvect_index x, const zvect_index y);

// Operations with multiple vectors:

/*
 * vect_copy is a function that allows to copy a specified
 * set of elements from a vector to another.
 * Please note: only vectors with the same data size (the
 * parameter we pass during the creation of both vectors)
 * can be copied into the other!
 *
 * vect_copy(v1, v2, 3, 5)      will copy all the items in
 *                              vector v2, from the 4th item
 *                              till the 9th (3 + 5, remember
 *                              vector items start from 0) in
 *                              the vector v1. So at the end
 *                              of the process you'll have such
 *                              items copied at the end of v1.
 */
void vect_copy(vector const v1, vector const v2, zvect_index start, zvect_index max_elements);

/*
 * vect_insert is a function that allows to copy a specified
 * set of elements from a vector to another and "insert"
 * them from a specified position in the destination vector.
 * Please note: only vectors with the same data size (the
 * parameter we pass during the creation of both vectors)
 * can be copied into the other!
 *
 * vect_insert(v1, v2, 3, 5, 7) will copy all the items in
 *                              vector v2, from the 4th item
 *                              till the 9th (3 + 5, remember
 *                              vector items start from 0) in
 *                              the vector v1 from position 7.
 *                              So at the end of the process
 *                              you'll have such items "inserted"
 *                              inside v1.
 */
void vect_insert(vector const v1, vector const v2, const zvect_index s2,
                 const zvect_index e2, const zvect_index s1);

/*
 * vect_move is a function that allows to move a specified
 * set of items from one vector to another.
 * It will also re-organize the source vector and (obviously)
 * expand the destination vector if needed.
 * Please note: only vectors of the same data size can be moved
 * one into the other!
 *
 * vect_move(v1, v2, 2, 2)      will move items in v2 from the
 *                              3rd item in v2 till the 5th at
 *                              the end of v1.
 */
void vect_move(vector const v1, vector const v2, zvect_index start, zvect_index max_elements);

/*
 * vect_move_if is a function that allows to move a specified
 * set of items from one vector to another if the condition
 * returned by the function pointed by f2 function pointer
 * is true.
 * It will also re-organize the source vector and (obviously)
 * expand the destination vector if needed.
 * Please note: only vectors of the same data size can be moved
 * one into the other!
 *
 * vect_move(v1, v2, 2, 2, check_data) will move items in v2 from the
 *                              3rd item in v2 till the 5th at
 *                              the end of v1 if check_data returns
 * 				true.
 */
zvect_retval vect_move_if(vector const v1, vector v2, const zvect_index s2,
               const zvect_index e2, zvect_retval (*f2)(void *, void *));

/*
 * vect_merge is a function that merges together 2 vectors of
 * the same data size. At the end of the process, the source
 * vector will be destroyed.
 *
 * vect_merge(v1, v2)           will merge vector v2 to v1 and then
 *                              destroy v2. So at the end of the job
 *                              v1 will contain the old v1 items +
 *                              all v2 items.
 */
void *vect_merge(vector const v1, vector v2);

#endif  // ZVECT_SFMD_EXTENSIONS

#ifdef _cplusplus
}
#endif

#endif  // SRC_ZVECTOR_H_
