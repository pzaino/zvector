/*
 *    Name: PTest005
 * Purpose: Performance Testing ZVector Library for high volume threads
 *  Author: Paolo Fabio Zaino
 *  Domain: General
 * License: Copyright by Paolo Fabio Zaino, all rights reserved
 *          Distributed under MIT license
 */

#if (__GNUC__ < 6)
#define _BSD_SOURCE
#endif
#if (__GNUC__ > 5)
#define _DEFAULT_SOURCE
#endif

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

// Setup tests:
char *testGrp = "005";
uint8_t testID = 4;

#if ( ZVECT_THREAD_SAFE == 1 ) && ( OS_TYPE == 1 )

// Initialise Random Generator
static int mySeed = 25011984;
int max_strLen = 16;

#include <pthread.h>

#define TOTAL_ITEMS 10000000
#define MAX_THREADS 8
#define MAX_ITEMS TOTAL_ITEMS / ( MAX_THREADS / 2)
#define MAX_MSG_SIZE 72
pthread_t tid[MAX_THREADS]; // threads IDs

struct thread_args {
    int id;
    vector v;
};

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
			int l = (int)(sizeof(charset) - 1);
			for (size_t n = 0; n < len; n++) {
				int key = rand() % l;
				rndStr[n] = charset[key];
			}
			rndStr[len] = '\0';
		}
	}
}

void clear_str(char *str, size_t len) {
	memset(str, 0, len);
}

// Threads
void *producer(void *arg) {
	struct thread_args *targs = (struct thread_args *)arg;
	vector v = (vector)targs->v;
	int id = (int)targs->id;

	CCPAL_INIT_LIB;

	// Simulating Producer:
	printf("Test %s_%d: Thread %*i, produce %*d events, store them in the queue and check if they are stored correctly:\n", testGrp, testID, 3, id, 4, MAX_ITEMS);
	fflush(stdout);

		uint32_t i;
		QueueItem qi;
		memset(qi.msg, 65, MAX_MSG_SIZE);
		qi.priority = 0;
		vect_lock(v);
		CCPAL_START_MEASURING;
		for (i = 0; i < MAX_ITEMS; i++)
		{
			qi.eventID = ((id*MAX_ITEMS)+1)+i;

			// Let's add a new item in the queue:
			vect_add(v, &qi);
		}
		CCPAL_STOP_MEASURING;
		vect_unlock(v);

	printf("Producer thread %i done. Produced %d events.\n", id, i);
	fflush(stdout);

	// Returns perf analysis results:
	CCPAL_REPORT_ANALYSIS;
	fflush(stdout);

	pthread_exit(NULL);
	return NULL;
}

void *consumer(void *arg) {
	struct thread_args *targs = (struct thread_args *)arg;
	vector v = (vector)targs->v;
	int id = (int)targs->id;
	int evt_counter = 0;

	CCPAL_INIT_LIB;

	// Simulating Consumer:
	printf("Test %s_%d: Thread %*i, consume %*d events from the queue in FIFO order:\n", testGrp, testID, 3, id, 4, MAX_ITEMS);
	fflush(stdout);

		uint32_t i;
		QueueItem *item = (QueueItem *)malloc(sizeof(QueueItem *));
		int fetched_item = 0;
		int missed = 0;
		vect_lock(v);
		CCPAL_START_MEASURING;
		for (i = 0; i < MAX_ITEMS;) {

			if (!vect_is_empty(v))
			{
				item = (QueueItem *)vect_remove_front(v);
				fetched_item=1;
			} else {
				fetched_item= 0;
				missed++;
			}

			if ( fetched_item == 1 )
			{
				evt_counter++;
				i++;
			} else if ( missed > 1000000 )
				break;

		}
		CCPAL_STOP_MEASURING;
		vect_unlock(v);
		free(item);

	printf("Consumer thread %i done. Consumed %d events.\n", id, evt_counter);
	fflush(stdout);

	// Returns perf analysis results:
	CCPAL_REPORT_ANALYSIS;
	fflush(stdout);

	pthread_exit(NULL);
	return NULL;
}

int main() {
	// Setup
	srand((time(NULL) * max_strLen) + (++mySeed));

	printf("=== PTest%s ===\n", testGrp);
	printf("Testing Dynamic QUEUES (MULTI thread, with many threads and passing complex data structures)\n");

	fflush(stdout);

	printf("Test %s_%d: Create a Queue of %*i initial elements capacity:\n", testGrp, testID, 8, TOTAL_ITEMS*2);
	fflush(stdout);

		vector v;
		v = vect_create(TOTAL_ITEMS*2, sizeof(struct QueueItem), ZV_NONE);

	printf("done.\n");
	testID++;

	fflush(stdout);

	printf("Test %s_%d: Spin %i threads (%i producers and %i consumers) and use them to manipulate the Queue above.\n", testGrp, testID, MAX_THREADS, MAX_THREADS / 2, MAX_THREADS / 2);
	fflush(stdout);

		int err = 0;
		int i = 0;
		struct thread_args *targs[MAX_THREADS+1];
		for (i=0; i < MAX_THREADS / 2; i++) {
			targs[i]=(struct thread_args *)malloc(sizeof(struct thread_args));
			targs[i]->id=i;
			targs[i]->v=v;
			err = pthread_create(&(tid[i]), NULL, &producer, targs[i]);
			if (err != 0)
				printf("Can't create producer thread :[%s]\n", strerror(err));

			targs[i+(MAX_THREADS / 2)]=(struct thread_args *)malloc(sizeof(struct thread_args));
			targs[i+(MAX_THREADS / 2)]->id=i+(MAX_THREADS / 2);
			targs[i+(MAX_THREADS / 2)]->v=v;
			err = pthread_create(&(tid[i+(MAX_THREADS / 2)]), NULL, &consumer, targs[i+(MAX_THREADS / 2)]);
			if (err != 0)
				printf("Can't create consumer thread :[%s]\n", strerror(err));
		}

		// Let's start the threads:
		for (i=0; i < MAX_THREADS; i++) {
			pthread_join(tid[i], NULL);
		}

		for(i=0; i < MAX_THREADS; i++) {
			free(targs[i]);
		}

	printf("done.\n");
	testID++;

	fflush(stdout);

	// Allow the Producer and Consumer process
	// To start properly:
	//usleep(100);

	printf("--- Events missed in the queue: %*i ---\n", 4, vect_size(v));
	fflush(stdout);

	// Now wait until the Queue is empty:
	//while(!vect_is_empty(v))
	//	;

	//printf("--- Events missed in the queue: %*i ---\n", 4, vect_size(v));
	//fflush(stdout);

	printf("Test %s_%d: Delete all left over events (if any):\n", testGrp, testID);
	fflush(stdout);

		while (!vect_is_empty(v))
			vect_delete(v);

	printf("done.\n");
	testID++;

	fflush(stdout);

	printf("Test %s_%d: Clear Queue:\n", testGrp, testID);
	fflush(stdout);

		vect_clear(v);

	printf("done.\n");
	testID++;

	fflush(stdout);

	printf("Test %s_%d: Check if Queue size is now 0 (zero):\n", testGrp, testID);
	fflush(stdout);

		assert(vect_size(v) == 0);

	printf("done.\n");
	testID++;

	fflush(stdout);

	printf("Test %s_%d: destroy the Queue:\n", testGrp, testID);
	fflush(stdout);

		vect_destroy(v);

	printf("done.\n");
	testID++;

	fflush(stdout);

	printf("================\n\n");

	return 0;
}

#else

int main() {
	printf("=== ITest%s ===\n", testGrp);
	printf("Testing Thread_safe features:\n");

	printf("Skipping test because library has been built without THREAD_SAFE enabled or on a platform that does not supports pthread.\n");

	printf("================\n\n");

	return 0;
}

#endif // THREAD_SAFE