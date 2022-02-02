/*
 *    Name: UTest003
 * Purpose: Unit Testing ZVector Library
 *  Author: Paolo Fabio Zaino
 *  Domain: General
 * License: Copyright by Paolo Fabio Zaino, all rights reserved
 *          Distributed under MIT license
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if (defined(_MSC_VER))
// Silly stuff that needs to be added for Microsoft compilers
// which are still at the MS-DOS age apparently...
#define ZVECTORH "../src/zvector.h"
#else
#define ZVECTORH "zvector.h"
#endif
#include ZVECTORH

#define MAX_ITEMS 100
#define MAX_MSG_SIZE 72

// Initialise Random Gnerator
static uint32_t mySeed = 25011984;

typedef struct QueueItem {
	int32_t eventID;
	char msg[MAX_MSG_SIZE];
	uint32_t priority;
} QueueItem;

int compare_func(const void *a, const void *b) {
	return (((QueueItem *)a)->eventID - ((QueueItem *)b)->eventID);
}

// Generates random strings of chars:
void mk_rndstr(char *rndStr, size_t len) {
	static char charset[] =
		"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789,.-#'?!";

	if (len) {
		if (rndStr) {
			int l = (int)(sizeof(charset) - 1);
			for (size_t n = 0; n < len; n++) {
				int key = rand() % l;
				rndStr[n] = charset[key];
			}
			rndStr[len] = '\0';
		}
	}
}

void clear_str(const char *str, const size_t len) {
	memset((void *)str, 0, len);
}

int main() {
	// Setup tests:
	char *testGrp = "006";
	uint8_t testID = 1;

	int max_strLen = MAX_MSG_SIZE - 8;

	srand((time(NULL) * max_strLen) + (++mySeed));

	printf("=== UTest%s ===\n", testGrp);
	printf("Testing Dynamic QUEUES (single thread) with vect_merge\n");

	fflush(stdout);

#if (ZVECT_THREAD_SAFE == 1)
	vect_lock_disable();
#endif

	printf("Test %s_%d: Create a Queue of 2 elements and using QueueItem for the "
		"vector data:\n",
		testGrp, testID);

	vector v = vect_create(2, sizeof(struct QueueItem), ZV_SEC_WIPE);

	printf("done.\n");
	testID++;

	fflush(stdout);

	printf("Test %s_%d: Create 2 ordered Queues of 2 elements each and using "
		"QueueItem for the vector data:\n",
		testGrp, testID);

		vector v1 = vect_create(2, sizeof(struct QueueItem), ZV_NONE | ZV_SEC_WIPE);
		vector v2 = vect_create(2, sizeof(struct QueueItem), ZV_NONE | ZV_SEC_WIPE);

	printf("done.\n");
	testID++;

	fflush(stdout);

	// Simulating Producer:
	uint32_t i;
	printf(
		"Test %s_%d: Produce %d events with event id randomly generated, store "
		"them ordered in the queue v1 and check if they are stored correctly:\n",
		testGrp, testID, MAX_ITEMS / 2);
	fflush(stdout);

		for (i = 0; i < MAX_ITEMS / 2; i++) {
			QueueItem qi;
			qi.eventID = rand();
			qi.msg[0] = '\0';
			clear_str(qi.msg, MAX_MSG_SIZE);
			mk_rndstr(qi.msg, max_strLen - 1);
			qi.priority = 0;

			// printf("produced event message: %s\n", qi.msg);

			// Let's add a new item in the queue:
			vect_add_ordered(v1, &qi, compare_func);

			// Let's check if the vector size has grown correctly:
			assert(vect_size(v1) == (zvect_index)i + 1);
		}

	printf(" - Test %s_%d: Ordered Vector generated, now proceeding with "
		"checking if it's correctly ordered:\n",
		testGrp, testID);
	fflush(stdout);

	for (i = 1; i < MAX_ITEMS / 2; i++) {
		QueueItem item1 = *((QueueItem *)vect_get_at(v1, i - 1));
		QueueItem item2 = *((QueueItem *)vect_get_at(v1, i));

		assert(item1.eventID <= item2.eventID);
	}

	printf("done.\n");
	testID++;

	fflush(stdout);

	// Simulating Producer:
	printf(
		"Test %s_%d: Produce %d events with event id randomly generated, store "
		"them ordered in v2 queue and check if they are stored correctly:\n",
		testGrp, testID, MAX_ITEMS / 2);
	fflush(stdout);

		for (i = 0; i < MAX_ITEMS / 2; i++) {
			QueueItem qi;
			qi.eventID = rand();
			qi.msg[0] = '\0';
			clear_str(qi.msg, MAX_MSG_SIZE);
			mk_rndstr(qi.msg, max_strLen - 1);
			qi.priority = 0;

			// printf("produced event message: %s\n", qi.msg);

			// Let's add a new item in the queue:
			vect_add_ordered(v2, &qi, compare_func);

			// Let's check if the vector size has grown correctly:
			assert(vect_size(v2) == (zvect_index)i + 1);
		}

	printf(" - Test %s_%d: Ordered Vector generated, now proceeding with "
		"checking if it's correctly ordered:\n",
		testGrp, testID);
	fflush(stdout);

	for (i = 1; i < MAX_ITEMS / 2; i++) {
		QueueItem item1 = *((QueueItem *)vect_get_at(v2, i - 1));
		QueueItem item2 = *((QueueItem *)vect_get_at(v2, i));

		assert(item1.eventID <= item2.eventID);
	}

	printf("done.\n");
	testID++;

	fflush(stdout);

	printf(" - Test %s_%d: merge v1 into v:\n", testGrp, testID);
	fflush(stdout);

		vect_merge(v, v1);

	printf("done.\n");
	testID++;

	printf(" - Test %s_%d: Rotate right v of %d:\n", testGrp, testID,
		MAX_ITEMS / 4);
	fflush(stdout);

		vect_rotate_right(v, MAX_ITEMS / 4);

	printf("done.\n");
	testID++;

	printf(" - Test %s_%d: merge v2 into v:\n", testGrp, testID);
	fflush(stdout);

		vect_merge(v, v2);

	printf("done.\n");
	testID++;

	// Simulating Consumer:
	printf("Test %s_%d: Consume %d events from the queue:\n", testGrp, testID,
		MAX_ITEMS);
	fflush(stdout);

	for (i = 0; i < MAX_ITEMS; i++) {
		// Let's retrieve the value from the vector correctly:
		// For beginners: this is how in C we convert back a void * into the
		// original data_type
		QueueItem *item = (QueueItem *)vect_remove_front(v);

		// Let's test if the value we have retrieved is correct:
		printf("Event %*d: ID (%*d) - Message: %s\n", 4, i, 12, item->eventID,
			item->msg);
		fflush(stdout);

		free(item);
	}

	printf("done.\n");
	testID++;

	fflush(stdout);

	printf("Test %s_%d: Delete all left over events (if any):\n", testGrp,
		testID);
	fflush(stdout);

		while (!vect_is_empty(v)) {
			vect_delete(v);
		}

	printf("done.\n");
	testID++;

	fflush(stdout);

	printf("Test %s_%d: Clear vector:\n", testGrp, testID);
	fflush(stdout);

		vect_clear(v);

	printf("done.\n");
	testID++;

	fflush(stdout);

	printf("Test %s_%d: Check if vector size is now 0 (zero):\n", testGrp,
		testID);
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
