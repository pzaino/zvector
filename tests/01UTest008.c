// Write an integration test for the function "vect_rotate_right" in "vector.c"
// The function rotates a vector by a given number of positions.
// The function returns the rotated vector.
// The function takes a vector and a number of positions as arguments.
// The function returns the input vector modified.
// The function does modify the original vector.
// The function does not modify the number of positions.
// The function does not modify the original vector if the number of positions is zero.
// The function does not modify the original vector if the number of positions is equal to the vector size.
// The function does not modify the original vector if the number of positions is greater than the vector size.
// The function does not modify the original vector if the number of positions is negative.
// The function does not modify the original vector if the number of positions is zero.
// The function does not modify the original vector if the number of positions is negative.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#if ( defined(_MSC_VER) )
 // Silly stuff that needs to be added for Microsoft compilers
 // which are still at the MS-DOS age apparently...
#define ZVECTORH "../src/zvector.h"
#else
#define ZVECTORH "zvector.h"
#endif
#include ZVECTORH

// Setup tests:
char *testGrp = "008";
uint8_t testID = 1;

int main(void)
{
	printf("=== UTest%s ===\n", testGrp);
	printf("Testing basic vector functionalities\n");

	fflush(stdout);

	printf("Test %s_%d: Create a vector of 10 elements and using int for the vector data:\n",
		testGrp, testID);
	fflush(stdout);

	// Create a vector of integers.
	vector v = vect_create(10, sizeof(int), 0);

	assert(v != NULL);
	// Add some elements to the vector.
	for (int i=1; i<=10; i++) {
		vect_add(v, &i);
	}

	printf("done.\n");
	testID++;

	fflush(stdout);

	printf("Test %s_%d: Rotate right the vector of %d items and check if it's not modified:\n",
		testGrp, testID, 0);
	fflush(stdout);

	// Rotate the vector by 0 positions.
	vect_rotate_right(v, 0);

	printf("first element: %d\n", *((int *)vect_get_at(v, 0)));

	// Check that the vector is not modified.
	assert(vect_size(v) == 10);
	assert(*((int *)vect_get_at(v, 0)) == 1);
	assert(*((int *)vect_get_at(v, 1)) == 2);
	assert(*((int *)vect_get_at(v, 2)) == 3);
	assert(*((int *)vect_get_at(v, 3)) == 4);
	assert(*((int *)vect_get_at(v, 4)) == 5);
	assert(*((int *)vect_get_at(v, 5)) == 6);
	assert(*((int *)vect_get_at(v, 6)) == 7);
	assert(*((int *)vect_get_at(v, 7)) == 8);
	assert(*((int *)vect_get_at(v, 8)) == 9);
	assert(*((int *)vect_get_at(v, 9)) == 10);

	printf("done.\n");
	testID++;

	fflush(stdout);

	printf("Test %s_%d: Rotate right the vector of %d items and check if it's not modified:\n",
		testGrp, testID, 10);
	fflush(stdout);

	// Rotate the vector by 10 positions.
	vect_rotate_right(v, 10);

	// Check that the vector is not modified.
	assert(vect_size(v) == 10);

	printf("first element: %d\n", *((int *)vect_get_at(v, 0)));

	assert(*((int *)vect_get_at(v, 0)) == 1);
	assert(*((int *)vect_get_at(v, 1)) == 2);
	assert(*((int *)vect_get_at(v, 2)) == 3);
	assert(*((int *)vect_get_at(v, 3)) == 4);
	assert(*((int *)vect_get_at(v, 4)) == 5);
	assert(*((int *)vect_get_at(v, 5)) == 6);
	assert(*((int *)vect_get_at(v, 6)) == 7);
	assert(*((int *)vect_get_at(v, 7)) == 8);
	assert(*((int *)vect_get_at(v, 8)) == 9);
	assert(*((int *)vect_get_at(v, 9)) == 10);

	printf("done.\n");
	testID++;

	fflush(stdout);

	printf("Test %s_%d: Rotate right the vector of %d items and check if it's not modified:\n",
		testGrp, testID, 11);
	fflush(stdout);

	// Rotate the vector by 11 positions.
	vect_rotate_right(v, 11);

	// Check that the vector is not modified.
	assert(vect_size(v) == 10);

	printf("first element: %d\n", *((int *)vect_get_at(v, 0)));

	assert(*((int *)vect_get_at(v, 0)) == 10);
	assert(*((int *)vect_get_at(v, 1)) == 1);
	assert(*((int *)vect_get_at(v, 2)) == 2);
	assert(*((int *)vect_get_at(v, 3)) == 3);
	assert(*((int *)vect_get_at(v, 4)) == 4);
	assert(*((int *)vect_get_at(v, 5)) == 5);
	assert(*((int *)vect_get_at(v, 6)) == 6);
	assert(*((int *)vect_get_at(v, 7)) == 7);
	assert(*((int *)vect_get_at(v, 8)) == 8);
	assert(*((int *)vect_get_at(v, 9)) == 9);

	printf("done.\n");
	testID++;

	fflush(stdout);

	printf("Test %s_%d: Rotate right the vector of %d items and check if it's not modified:\n",
		testGrp, testID, -1);
	printf("         (This should be the same as rotating right by 5)\n");
	printf("Explanation: -1 is converted to 4,294,967,295 (2^32 - 1) and then the number of rotations is equal to 4,294,967,295 modulo 10 which is 5.\n");
	fflush(stdout);

	// Rotate the vector by -1 positions.
	vect_rotate_right(v, -1);

	// Check that the vector is not modified.
	assert(vect_size(v) == 10);

	printf("first element: %d\n", *((int *)vect_get_at(v, 0)));

	assert(*((int *)vect_get_at(v, 0)) == 5);
	assert(*((int *)vect_get_at(v, 1)) == 6);
	assert(*((int *)vect_get_at(v, 2)) == 7);
	assert(*((int *)vect_get_at(v, 3)) == 8);
	assert(*((int *)vect_get_at(v, 4)) == 9);
	assert(*((int *)vect_get_at(v, 5)) == 10);
	assert(*((int *)vect_get_at(v, 6)) == 1);
	assert(*((int *)vect_get_at(v, 7)) == 2);
	assert(*((int *)vect_get_at(v, 8)) == 3);
	assert(*((int *)vect_get_at(v, 9)) == 4);

	printf("done.\n");
	testID++;

	fflush(stdout);

	printf("Test %s_%d: Rotate right the vector of %d items and check if it's not modified:\n",
		testGrp, testID, 1);
	fflush(stdout);

	// Rotate the vector by 1 position.
	vect_rotate_right(v, 1);

	// Check that the vector is modified.
	assert(vect_size(v) == 10);

	printf("first element: %d\n", *((int *)vect_get_at(v, 0)));

	assert(*((int *)vect_get_at(v, 0)) == 4);
	assert(*((int *)vect_get_at(v, 1)) == 5);
	assert(*((int *)vect_get_at(v, 2)) == 6);
	assert(*((int *)vect_get_at(v, 3)) == 7);
	assert(*((int *)vect_get_at(v, 4)) == 8);
	assert(*((int *)vect_get_at(v, 5)) == 9);
	assert(*((int *)vect_get_at(v, 6)) == 10);
	assert(*((int *)vect_get_at(v, 7)) == 1);
	assert(*((int *)vect_get_at(v, 8)) == 2);
	assert(*((int *)vect_get_at(v, 9)) == 3);

	printf("done.\n");
	testID++;

	fflush(stdout);

	printf("Test %s_%d: Rotate right the vector of %d items and check if it's not modified:\n",
		testGrp, testID, 2);
	fflush(stdout);

	// Rotate the vector by 2 positions.
	vect_rotate_right(v, 2);

	// Check that the vector is modified.
	assert(vect_size(v) == 10);

	printf("first element: %d\n", *((int *)vect_get_at(v, 0)));

	assert(*((int *)vect_get_at(v, 0)) == 2);
	assert(*((int *)vect_get_at(v, 1)) == 3);
	assert(*((int *)vect_get_at(v, 2)) == 4);
	assert(*((int *)vect_get_at(v, 3)) == 5);
	assert(*((int *)vect_get_at(v, 4)) == 6);
	assert(*((int *)vect_get_at(v, 5)) == 7);
	assert(*((int *)vect_get_at(v, 6)) == 8);
	assert(*((int *)vect_get_at(v, 7)) == 9);
	assert(*((int *)vect_get_at(v, 8)) == 10);
	assert(*((int *)vect_get_at(v, 9)) == 1);

	printf("done.\n");
	testID++;

	fflush(stdout);

	printf("Test %s_%d: Rotate right the vector of %d items and check if it's not modified:\n",
		testGrp, testID, 3);
	fflush(stdout);

	// Rotate the vector by 3 positions.
	vect_rotate_right(v, 3);

	// Check that the vector is modified.
	assert(vect_size(v) == 10);

	printf("first element: %d\n", *((int *)vect_get_at(v, 0)));

	assert(*((int *)vect_get_at(v, 0)) == 9);
	assert(*((int *)vect_get_at(v, 1)) == 10);
	assert(*((int *)vect_get_at(v, 2)) == 1);
	assert(*((int *)vect_get_at(v, 3)) == 2);
	assert(*((int *)vect_get_at(v, 4)) == 3);
	assert(*((int *)vect_get_at(v, 5)) == 4);
	assert(*((int *)vect_get_at(v, 6)) == 5);
	assert(*((int *)vect_get_at(v, 7)) == 6);
	assert(*((int *)vect_get_at(v, 8)) == 7);
	assert(*((int *)vect_get_at(v, 9)) == 8);

	printf("done.\n");
	testID++;

	fflush(stdout);

	printf("Test %s_%d: Rotate right the vector of %d items and check if it's not modified:\n",
		testGrp, testID, 4);
	fflush(stdout);

	// Rotate the vector by 4 positions.
	vect_rotate_right(v, 4);

	// Check that the vector is modified.
	assert(vect_size(v) == 10);

	printf("first element: %d\n", *((int *)vect_get_at(v, 0)));

	assert(*((int *)vect_get_at(v, 0)) == 5);
	assert(*((int *)vect_get_at(v, 1)) == 6);
	assert(*((int *)vect_get_at(v, 2)) == 7);
	assert(*((int *)vect_get_at(v, 3)) == 8);
	assert(*((int *)vect_get_at(v, 4)) == 9);
	assert(*((int *)vect_get_at(v, 5)) == 10);
	assert(*((int *)vect_get_at(v, 6)) == 1);
	assert(*((int *)vect_get_at(v, 7)) == 2);
	assert(*((int *)vect_get_at(v, 8)) == 3);
	assert(*((int *)vect_get_at(v, 9)) == 4);

	printf("done.\n");
	testID++;

	fflush(stdout);

	printf("Test %s_%d: Rotate right the vector of %d items and check if it's not modified:\n",
		testGrp, testID, 5);
	fflush(stdout);

	// Rotate the vector by 5 positions.
	vect_rotate_right(v, 5);

	// Check that the vector is modified.
	assert(vect_size(v) == 10);

	printf("first element: %d\n", *((int *)vect_get_at(v, 0)));

	assert(*((int *)vect_get_at(v, 0)) == 10);
	assert(*((int *)vect_get_at(v, 1)) == 1);
	assert(*((int *)vect_get_at(v, 2)) == 2);
	assert(*((int *)vect_get_at(v, 3)) == 3);
	assert(*((int *)vect_get_at(v, 4)) == 4);
	assert(*((int *)vect_get_at(v, 5)) == 5);
	assert(*((int *)vect_get_at(v, 6)) == 6);
	assert(*((int *)vect_get_at(v, 7)) == 7);
	assert(*((int *)vect_get_at(v, 8)) == 8);
	assert(*((int *)vect_get_at(v, 9)) == 9);

	printf("done.\n");
	testID++;

	fflush(stdout);

	printf("Test %s_%d: Rotate right the vector of %d items and check if it's not modified:\n",
		testGrp, testID, 6);
	fflush(stdout);

	// Rotate the vector by 6 positions.
	vect_rotate_right(v, 6);

	// Check that the vector is modified.
	assert(vect_size(v) == 10);

	printf("first element: %d\n", *((int *)vect_get_at(v, 0)));

	assert(*((int *)vect_get_at(v, 0)) == 4);
	assert(*((int *)vect_get_at(v, 1)) == 5);
	assert(*((int *)vect_get_at(v, 2)) == 6);
	assert(*((int *)vect_get_at(v, 3)) == 7);
	assert(*((int *)vect_get_at(v, 4)) == 8);
	assert(*((int *)vect_get_at(v, 5)) == 9);
	assert(*((int *)vect_get_at(v, 6)) == 10);
	assert(*((int *)vect_get_at(v, 7)) == 1);
	assert(*((int *)vect_get_at(v, 8)) == 2);
	assert(*((int *)vect_get_at(v, 9)) == 3);

	printf("done.\n");
	testID++;

	fflush(stdout);

	printf("Test %s_%d: Rotate right the vector of %d items and check if it's not modified:\n",
		testGrp, testID, 7);
	fflush(stdout);

	// Rotate the vector by 7 positions.
	vect_rotate_right(v, 7);

	// Check that the vector is modified.
	assert(vect_size(v) == 10);

	printf("first element: %d\n", *((int *)vect_get_at(v, 0)));

	assert(*((int *)vect_get_at(v, 0)) == 7);
	assert(*((int *)vect_get_at(v, 1)) == 8);
	assert(*((int *)vect_get_at(v, 2)) == 9);
	assert(*((int *)vect_get_at(v, 3)) == 10);
	assert(*((int *)vect_get_at(v, 4)) == 1);
	assert(*((int *)vect_get_at(v, 5)) == 2);
	assert(*((int *)vect_get_at(v, 6)) == 3);
	assert(*((int *)vect_get_at(v, 7)) == 4);
	assert(*((int *)vect_get_at(v, 8)) == 5);
	assert(*((int *)vect_get_at(v, 9)) == 6);

	printf("done.\n");
	testID++;

	fflush(stdout);

	printf("Test %s_%d: Rotate right the vector of %d items and check if it's not modified:\n",
		testGrp, testID, 8);
	fflush(stdout);

	// Rotate the vector by 8 positions.
	vect_rotate_right(v, 8);

	// Check that the vector is modified.
	assert(vect_size(v) == 10);

	printf("first element: %d\n", *((int *)vect_get_at(v, 0)));

	assert(*((int *)vect_get_at(v, 0)) == 9);
	assert(*((int *)vect_get_at(v, 1)) == 10);
	assert(*((int *)vect_get_at(v, 2)) == 1);
	assert(*((int *)vect_get_at(v, 3)) == 2);
	assert(*((int *)vect_get_at(v, 4)) == 3);
	assert(*((int *)vect_get_at(v, 5)) == 4);
	assert(*((int *)vect_get_at(v, 6)) == 5);
	assert(*((int *)vect_get_at(v, 7)) == 6);
	assert(*((int *)vect_get_at(v, 8)) == 7);
	assert(*((int *)vect_get_at(v, 9)) == 8);

	printf("done.\n");
	testID++;

	fflush(stdout);

	printf("Test %s_%d: Rotate right the vector of %d items and check if it's not modified:\n",
		testGrp, testID, 9);
	fflush(stdout);

	// Rotate the vector by 9 positions.
	vect_rotate_right(v, 9);

	// Check that the vector is modified.
	assert(vect_size(v) == 10);

	printf("first element: %d\n", *((int *)vect_get_at(v, 0)));

	assert(*((int *)vect_get_at(v, 0)) == 10);
	assert(*((int *)vect_get_at(v, 1)) == 1);
	assert(*((int *)vect_get_at(v, 2)) == 2);
	assert(*((int *)vect_get_at(v, 3)) == 3);
	assert(*((int *)vect_get_at(v, 4)) == 4);
	assert(*((int *)vect_get_at(v, 5)) == 5);
	assert(*((int *)vect_get_at(v, 6)) == 6);
	assert(*((int *)vect_get_at(v, 7)) == 7);
	assert(*((int *)vect_get_at(v, 8)) == 8);
	assert(*((int *)vect_get_at(v, 9)) == 9);

	printf("done.\n");
	testID++;

	fflush(stdout);

	printf("Test %s_%d: Rotate right the vector of %d items and check if it's not modified:\n",
		testGrp, testID, 10);
	fflush(stdout);

	// Rotate the vector by 10 positions.
	vect_rotate_right(v, 10);

	// Check that the vector is modified.
	assert(vect_size(v) == 10);

	printf("first element: %d\n", *((int *)vect_get_at(v, 0)));

	assert(*((int *)vect_get_at(v, 0)) == 10);
	assert(*((int *)vect_get_at(v, 1)) == 1);
	assert(*((int *)vect_get_at(v, 2)) == 2);
	assert(*((int *)vect_get_at(v, 3)) == 3);
	assert(*((int *)vect_get_at(v, 4)) == 4);
	assert(*((int *)vect_get_at(v, 5)) == 5);
	assert(*((int *)vect_get_at(v, 6)) == 6);
	assert(*((int *)vect_get_at(v, 7)) == 7);
	assert(*((int *)vect_get_at(v, 8)) == 8);
	assert(*((int *)vect_get_at(v, 9)) == 9);

	printf("done.\n");
	testID++;

	fflush(stdout);

	printf("Test %s_%d: Rotate right the vector of %d items and check if it's not modified:\n",
		testGrp, testID, 11);
	fflush(stdout);

	// Rotate the vector by 11 positions.
	vect_rotate_right(v, 11);

	// Check that the vector is modified.
	assert(vect_size(v) == 10);

	printf("first element: %d\n", *((int *)vect_get_at(v, 0)));

	assert(*((int *)vect_get_at(v, 0)) == 9);
	assert(*((int *)vect_get_at(v, 1)) == 10);
	assert(*((int *)vect_get_at(v, 2)) == 1);
	assert(*((int *)vect_get_at(v, 3)) == 2);
	assert(*((int *)vect_get_at(v, 4)) == 3);
	assert(*((int *)vect_get_at(v, 5)) == 4);
	assert(*((int *)vect_get_at(v, 6)) == 5);
	assert(*((int *)vect_get_at(v, 7)) == 6);
	assert(*((int *)vect_get_at(v, 8)) == 7);
	assert(*((int *)vect_get_at(v, 9)) == 8);

	printf("done.\n");
	testID++;

	fflush(stdout);

	printf("Test %s_%d: Rotate right the vector of %d items and check if it's not modified:\n",
		testGrp, testID, 12);
	fflush(stdout);

	// Rotate the vector by 12 positions.
	vect_rotate_right(v, 12);

	// Check that the vector is modified.
	assert(vect_size(v) == 10);

	printf("first element: %d\n", *((int *)vect_get_at(v, 0)));

	assert(*((int *)vect_get_at(v, 0)) == 7);
	assert(*((int *)vect_get_at(v, 1)) == 8);
	assert(*((int *)vect_get_at(v, 2)) == 9);
	assert(*((int *)vect_get_at(v, 3)) == 10);
	assert(*((int *)vect_get_at(v, 4)) == 1);
	assert(*((int *)vect_get_at(v, 5)) == 2);
	assert(*((int *)vect_get_at(v, 6)) == 3);
	assert(*((int *)vect_get_at(v, 7)) == 4);
	assert(*((int *)vect_get_at(v, 8)) == 5);
	assert(*((int *)vect_get_at(v, 9)) == 6);

	printf("done.\n");
	testID++;

	fflush(stdout);

	printf("Test %s_%d: Rotate right the vector of %d items and check if it's not modified:\n",
		testGrp, testID, 13);
	fflush(stdout);

	// Rotate the vector by 13 positions.
	vect_rotate_right(v, 13);

	// Check that the vector is modified.
	assert(vect_size(v) == 10);

	printf("first element: %d\n", *((int *)vect_get_at(v, 0)));

	assert(*((int *)vect_get_at(v, 0)) == 4);
	assert(*((int *)vect_get_at(v, 1)) == 5);
	assert(*((int *)vect_get_at(v, 2)) == 6);
	assert(*((int *)vect_get_at(v, 3)) == 7);
	assert(*((int *)vect_get_at(v, 4)) == 8);
	assert(*((int *)vect_get_at(v, 5)) == 9);
	assert(*((int *)vect_get_at(v, 6)) == 10);
	assert(*((int *)vect_get_at(v, 7)) == 1);
	assert(*((int *)vect_get_at(v, 8)) == 2);
	assert(*((int *)vect_get_at(v, 9)) == 3);

	printf("done.\n");
	testID++;

	fflush(stdout);

	printf("Destroy the vector\n");
	v = vect_destroy(v);
	printf("done.\n");

}

// The function "vect_rotate_right" is tested.
