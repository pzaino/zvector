/*
 *    Name: PTest001
 * Purpose: Performance Testing for ZVector Library
 *  Author: Paolo Fabio Zaino
 *  Domain: General
 * License: Copyright by Paolo Fabio Zaino, all rights reserved
 *          Distributed under MIT license
 */

#if __STDC_VERSION__ >= 199901L
#define _XOPEN_SOURCE 600
#else
#define _XOPEN_SOURCE 500
#endif /* __STDC_VERSION__ */

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "ccpal.h"

#if (  defined(_MSC_VER) )
 // Silly stuff that needs to be added for Microsoft compilers
 // which are still at the MS-DOS age apparently...
#define ZVECTORH "../src/zvector.h"
#else
#define ZVECTORH "zvector.h"
#endif
#include ZVECTORH

#define MAX_ITEMS 10000000

// Setup tests:
char *testGrp = "003";
uint8_t testID = 1;

#if ( OS_TYPE == 1 )

void populate_vector(const vector v, int max_elements, int place) {
	/*
	 * This macro initialise the perf mesurement library.
	 * have a look at my CCPal project on github for more details.
	 */
	CCPAL_INIT_LIB;

	switch (place) {
		case 0: printf("Test %s_%d: Insert %d elements (one by one) at the BEGINING of the vector and check how long this takes:\n", testGrp, testID, max_elements);
			break;
		case 1: printf("Test %s_%d: Insert %d elements (one by one) at the END of the vector and check how long this takes:\n", testGrp, testID, max_elements);
			break;
		case 2: printf("Test %s_%d: Insert %d elements (one by one) in the MIDDLE of the vector and check how long this takes:\n", testGrp, testID, max_elements);
			break;
	}

	fflush(stdout);

		int i = 0;
		CCPAL_START_MEASURING;

		for ( i = 0; i < max_elements; i++ )
			switch (place) {
				case 0: vect_add_front(v, &i);
					break;
				case 1: vect_add(v, &i);
					break;
				case 2: vect_add_at(v, &i, vect_size(v)/2);
			}

		CCPAL_STOP_MEASURING;

		// Returns perf analysis results:
		CCPAL_REPORT_ANALYSIS

	printf("done.\n");
	testID++;

	fflush(stdout);
}

int main() {
	/*
	 * This macro initialise the perf mesurement library.
	 * have a look at my CCPal project on github for more details.
	 */
	CCPAL_INIT_LIB;

	printf("=== PTest%s ===\n", testGrp);
	printf("Testing basic vector PERFORMANCE (Typical Dynamic Arrays adding elements from the middle of the Array)\n");

	fflush(stdout);

	printf("Test %s_%d: Create a vector of 10 elements and using int for the vector data:\n", testGrp, testID);
	fflush(stdout);

		vector v, tmpv1;
		    v = vect_create(10, sizeof(int), ZV_NOLOCKING);
		tmpv1 = vect_create(10, sizeof(int), ZV_NOLOCKING);

	printf("done.\n");
	testID++;

	fflush(stdout);

	// Populate the vector and measure how long it takes:
	printf("Test %s_%d: Add 10 elements to the main vector to generate an initial size of 10:\n", testGrp, testID);
	fflush(stdout);

		populate_vector(v, 10, 1);

	printf("Test %s_%d: Add now %*u elements to the subvector tmpv1 to create a partition of items:\n", testGrp, testID, 10, (MAX_ITEMS-10));
	fflush(stdout);

		populate_vector(tmpv1, (MAX_ITEMS-10), 1);

	printf("Test %s_%d: insert now tmpv1 inside the general vector in the middle:\n", testGrp, testID);
	fflush(stdout);

		vect_insert(v, tmpv1, 0, vect_size(tmpv1), vect_size(v)/2);

	printf("done.\n");
	testID++;

	fflush(stdout);

	printf("Test %s_%d: check if the size of the vector is now %d:\n", testGrp, testID, MAX_ITEMS);
	fflush(stdout);

		assert(vect_size(v) == MAX_ITEMS);

	printf("done.\n");
	testID++;

	fflush(stdout);

	printf("Test %s_%d: Remove vector elements one by one (from the END of the GENERAL vector) and test how long it takes:\n", testGrp, testID);
	fflush(stdout);


		CCPAL_START_MEASURING;

		while ( !vect_is_empty(v) )
			vect_delete(v);

		CCPAL_STOP_MEASURING;

		// Returns perf analysis results:
		CCPAL_REPORT_ANALYSIS;

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

	// Re-populate the vector again and measure how long it takes:
	printf("Test %s_%d: Add 10 elements to the main vector to generate an initial size of 10:\n", testGrp, testID);
	fflush(stdout);

		populate_vector(v, 10, 1);

	// We need to re-populate tmpv1, because insert moves elements, so now tmpv1 is empty:
	printf("Test %s_%d: Add now %*u elements to the subvector tmpv1 to create a partition of items:\n", testGrp, testID, 10, (MAX_ITEMS-10));
	fflush(stdout);

		populate_vector(tmpv1, (MAX_ITEMS-10), 1);

	printf("Test %s_%d: insert now (again) tmpv1 inside the general vector in the middle:\n", testGrp, testID);
	fflush(stdout);

		vect_insert(v, tmpv1, 0, vect_size(tmpv1), vect_size(v)/2);

	printf("done.\n");
	testID++;

	fflush(stdout);

	printf("Test %s_%d: destroy the vector:\n", testGrp, testID);
	fflush(stdout);

		// We cannot use vect_lock when destroying (vect_destroy will
		// destroy the lock mutex too!!!

		CCPAL_START_MEASURING;

		vect_destroy(v);
		vect_destroy(tmpv1);

		CCPAL_STOP_MEASURING;

		// Returns perf analysis results:
		CCPAL_REPORT_ANALYSIS;

	printf("done.\n");
	testID++;

	fflush(stdout);

	printf("================\n\n");

	return 0;
}

#else
int main()
{
	printf("=== PTest%s ===\n", testGrp);
	printf("Testing ZVector Library PERFORMANCE:\n");

	printf("Skipping test because this OS is not yet supported for perf tests, sorry!\n");

	printf("================\n\n");

	return 0;
}
#endif
