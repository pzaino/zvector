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

#define MAX_ITEMS 4

// Setup tests:
char *testGrp = "001";
uint8_t testID = 1;

// Create the type handler which is a pointer to handler function
typedef void (*Handler)(void);

// The functions to call via jump table:
void func3 (void) { printf( "func 3\n" ); }
void func2 (void) { printf( "func 2\n" ); }
void func1 (void) { printf( "func 1\n" ); }
void func0 (void) { printf( "func 0\n" ); }

// Let's also create an enum to have a ref to each functions:
enum{
    FUNC0,
    FUNC1,
    FUNC2,
    FUNC3
};

int compare_func(const void* a, const void* b) {
	return ( *(int*)a - *(int*)b );
}

int main() {

	printf("=== UTest%s ===\n", testGrp);
	printf("Creating Jump Tables using ZVector\n");

	fflush(stdout);

#if ( ZVECT_THREAD_SAFE == 1 )
	vect_lock_disable();
#endif

	printf("Test %s_%d: Create a vector of 10 elements and using int for the vector data:\n",
		testGrp, testID);
	fflush(stdout);

        	vector v = vect_create(MAX_ITEMS, sizeof(Handler), ZV_BYREF);

	printf("done.\n");
	testID++;

	fflush(stdout);

	printf("Test %s_%d: Configure our jump table using %d functions and check if they are stored correctly:\n",
		testGrp, testID, MAX_ITEMS);
	fflush(stdout);

	vect_add(v, func0);
	vect_add(v, func1);
	vect_add(v, func2);
	vect_add(v, func3);

	printf("done.\n");
	testID++;

	fflush(stdout);

	printf("Test %s_%d: check if the size of the vector is now %i:\n", testGrp, testID, MAX_ITEMS);
	fflush(stdout);

        	assert(vect_size(v) == MAX_ITEMS);

	printf("done.\n");
	testID++;

	fflush(stdout);

	printf("Test %s_%d: Call each function in order:\n", testGrp, testID);
	fflush(stdout);

		Handler object;
		object = (Handler)vect_get_at(v, FUNC0);
		object();
		object = (Handler)vect_get_at(v, FUNC1);
		object();
		object = (Handler)vect_get_at(v, FUNC2);
		object();
		object = (Handler)vect_get_at(v, FUNC3);
		object();

	printf("done.\n");
	testID++;

	fflush(stdout);

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
