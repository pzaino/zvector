/*
 *    Name: UTest003
 * Purpose: Unit Testing ZVector Library
 *  Author: Paolo Fabio Zaino
 *  Domain: General
 * License: Copyright by Paolo Fabio Zaino, all rights reserved
 *          Distributed under MIT license
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include <string.h>

#if (  defined(_MSC_VER) )
 // Silly stuff that needs to be added for Microsoft compilers
 // which are still at the MS-DOS age apparently...
#define ZVECTORH "../src/zvector.h"
#else
#define ZVECTORH "zvector.h"
#endif
#include ZVECTORH

#define MAX_ITEMS 20
#define MAX_MSG_SIZE 72
typedef struct QueueItem {
	uint32_t eventID;
	char msg[MAX_MSG_SIZE];
	uint32_t priority;
} QueueItem;

// Generates random strings of chars:
void mk_rndstr(char *rndStr, size_t len) {
    static char charset[] =
    	"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789,.-#'?!";

	if (len) {
		if (rndStr) {
			int l = (int)(sizeof(charset) -1);
			for (size_t n = 0; n < len; n++) {
				int key = rand() % l;
				rndStr[n] = charset[key];
			}
			rndStr[len] = '\0';
		}
	}
}

void clear_str(char *str, size_t len)
{
	memset(str, 0, len);
}

int main()
{
	// Setup tests:
	char *testGrp = "004";
	uint8_t testID = 1;

	// Initialise Random Gnerator
	static int mySeed = 25011984;
	int max_strLen = 64;

	srand((time(NULL) * max_strLen) + (++mySeed));

	printf("=== UTest%s ===\n", testGrp);
	printf("Testing Dynamic QUEUES (single thread)\n");

	fflush(stdout);

#if ( ZVECT_THREAD_SAFE == 1 )
	vect_lock_disable();
#endif

	printf("Test %s_%d: Create a vector of 2 elements and using int for the vector data:\n", testGrp, testID);

		vector v;
		v = vect_create(2, sizeof(struct QueueItem), ZV_SEC_WIPE);

	printf("done.\n");
	testID++;

	fflush(stdout);

	// Simulating Producer:
	printf("Test %s_%d: Produce %d events, store them in the queue and check if they are stored correctly:\n", testGrp, testID, MAX_ITEMS);
	fflush(stdout);

		uint32_t i;
		for (i = 0; i < MAX_ITEMS; i++) {
			QueueItem qi;
			qi.eventID = i;
			qi.msg[0] = '\0';
			clear_str(qi.msg, MAX_MSG_SIZE);
			mk_rndstr(qi.msg, max_strLen - 1);
			qi.priority = 0;

			//printf("produced event message: %s\n", qi.msg);

			// Let's add a new item in the queue:
			vect_add(v, &qi);

			// Let's check if the vector size has grown correctly:
			assert(vect_size(v) == (zvect_index)i + 1);

			// Let's retrieve the value from the vector correctly:
			// For beginners: this is how in C we convert back a void * into the original dtata_type
			uint32_t value = (uint32_t)((*((QueueItem *)vect_get_at(v, i))).eventID);

			// Let's test if the value we have retrieved is correct:
			assert(value == i);

			QueueItem item = *((QueueItem *)vect_get_at(v, i));

			// Let's test if the value we have retrieved is correct:
			printf("Event %*d: ID (%*d) - Message: %s\n", 2, i, 2, item.eventID, item.msg);
		}

	printf("done.\n");
	testID++;

	fflush(stdout);

	// Simulating Consumer:
	printf("Test %s_%d: Consume %d events from the queue:\n", testGrp, testID, MAX_ITEMS);
	fflush(stdout);

	for (i = 0; i < MAX_ITEMS; i++) {
		// Let's retrieve the value from the vector correctly:
		// For beginners: this is how in C we convert back a void * into the original dtata_type
		QueueItem *item = (QueueItem *)vect_remove_front(v);

		// Let's test if the value we have retrieved is correct:
		printf("Event %*d: ID (%*d) - Message: %s\n", 2, i, 2, item->eventID, item->msg);
		fflush(stdout);

		free(item);
	}

	printf("done.\n");
	testID++;

	fflush(stdout);

	printf("Test %s_%d: Delete all left over events (if any):\n", testGrp, testID);
	fflush(stdout);

		while (!vect_is_empty(v))
			vect_delete(v);

	printf("done.\n");
	testID++;

	fflush(stdout);

	printf("Test %s_%d: Clear vector:\n", testGrp, testID);
	fflush(stdout);

		vect_clear(v);

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
