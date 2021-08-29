/*
 *    Name: UTest001
 * Purpose: Unit Testing ZVector Library
 *  Author: Paolo Fabio Zaino
 *  Domain: General
 * License: Copyright by Paolo Fabio Zaino, all rights reserved
 *          Distributed under MIT license
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#if ( defined(_MSC_VER) )
 // Silly stuff that needs to be added for Microsoft compilers
 // which are still at the MS-DOS age apparently...
#define ZVECTORH "../src/zvector.h"
#else
#define ZVECTORH "zvector.h"
#endif
#include ZVECTORH

#define MAX_ITEMS 10000

// Setup tests:
char *testGrp = "001";
uint8_t testID = 1;

int compare_func(const void* a, const void* b) {
	return ( *(int*)a - *(int*)b );
}

int main() {

	printf("=== UTest%s ===\n", testGrp);
	printf("Testing basic vector functionalities\n");

	fflush(stdout);

#if ( ZVECT_THREAD_SAFE == 1 )
	vect_lock_disable();
#endif

	printf("Test %s_%d: Create a vector of 10 elements and using int for the vector data:\n",
		testGrp, testID);
	fflush(stdout);

        	vector v = vect_create(10, sizeof(int), ZV_NONE);

	printf("done.\n");
	testID++;

	fflush(stdout);

	printf("Test %s_%d: Insert %d elements and check if they are stored correctly:\n",
		testGrp, testID, MAX_ITEMS);
	fflush(stdout);

	int i;
		for (i = 0; i < MAX_ITEMS; i++)
		{
			// Let's add a new value in the vector:
			vect_add(v, &i);

			// Let's check if the vector size has grown correctly:
			assert(vect_size(v) == (zvect_index)i + 1);

			// Let's retrieve the value from the vector correctly:
			// For beginners: this is how in C we convert back a void * into the original dtata_type
			int value = *((int *)vect_get_at(v, i));

			// Let's test if the value we have retrieved is correct:
			assert(value == i);
		}

	printf("done.\n");
	testID++;

	fflush(stdout);

	printf("Test %s_%d: check if the size of the vector is now 10000:\n", testGrp, testID);
	fflush(stdout);

        	assert(vect_size(v) == MAX_ITEMS);

	printf("done.\n");
	testID++;

	fflush(stdout);

	printf("Test %s_%d: Add elements in the middle of the vector:\n", testGrp, testID);
	fflush(stdout);

		int x=555555;
		vect_add_at(v, &x, MAX_ITEMS / 2);

		assert(*((int *)vect_get_at(v, MAX_ITEMS / 2)) == x);

		assert(*((int *)vect_get_at(v, (MAX_ITEMS / 2)+1)) == (MAX_ITEMS / 2));

	printf("done.\n");
	testID++;

	fflush(stdout);

	printf("Test %s_%d: Remove an element from the middle of the vector:\n", testGrp, testID);
	fflush(stdout);

		printf("At index 5001 now we have: %d\n", *((int *)vect_get_at(v, 5001)));

		vect_delete_at(v, 5001);

		int value = *((int *)vect_get_at(v, 5001));
		printf("At index 5001 now we have: %d\n", value);
		fflush(stdout);
		assert( value == 5001);

	printf("done.\n");
	testID++;

	fflush(stdout);

#ifdef ZVECT_SFMD_EXTENSIONS
	printf("Test %s_%d: Sort the vector:\n", testGrp, testID);
	fflush(stdout);

		vect_qsort(v, compare_func);
		printf("Last element in the vector should now be %d: %d\n", x, *((int *)vect_get(v)));
		assert( *((int *)vect_get(v)) == x);

		printf("Checking all elements in the vector now to ensure they are in the correct order:\n");
		zvect_index item_id = 0;
		for (item_id = 1; item_id < vect_size(v); item_id++) {
			if ( compare_func( vect_get_at(v, item_id), vect_get_at(v, item_id - 1)) <= 0 )
			{
				printf(
					"Comparing element %d which is %d with element %d which is %d\n",
					item_id, *((int *)vect_get_at(v, item_id)), item_id - 1,
					*((int *)vect_get_at(v, item_id - 1)) );
				assert( compare_func( vect_get_at(v, item_id), vect_get_at(v, item_id - 1)) > 0);
			}
		}

	printf("done.\n");
	testID++;

	fflush(stdout);
#endif

#ifdef ZVECT_SFMD_EXTENSIONS
	int key = 5001;
	printf("Test %s_%d: Search for the element %d in the vector using Adaptive Binary Search:\n", testGrp, testID, key);
	fflush(stdout);

		zvect_index item_index = 0; // Let's define an item index as a pointer to a zvect_index
					// so we can pass it as parameter and the search function will
					// return the item index in there.
		if ( vect_bsearch(v, &key, compare_func, &item_index) ) {
			printf("Item %d found at location %d\n", key, item_index);
		} else {
			printf("Item %d not found!\n", key);
		}

	printf("done.\n");
	testID++;

	fflush(stdout);
#endif

	printf("Test %s_%d: Remove vector elements one by one:\n", testGrp, testID);
	fflush(stdout);

		while (!vect_is_empty(v))
			vect_delete(v);

	printf("done.\n");
	testID++;

	fflush(stdout);

	printf("Test %s_%d: check if vector is empty:\n", testGrp, testID);
	fflush(stdout);

		assert(vect_is_empty(v));

	printf("done.\n");
	testID++;

	fflush(stdout);

	printf("Test %s_%d: Check if vector size is now 0 (zero):\n", testGrp, testID);
	fflush(stdout);

		assert(vect_size(v) == 0);

	printf("done.\n");
	testID++;

	fflush(stdout);

	printf("Test %s_%d: destroy the vector:\n", testGrp, testID);
	fflush(stdout);

		vect_destroy(v);

	printf("done.\n");
	testID++;

	fflush(stdout);

	printf("================\n\n");

    return 0;
}
