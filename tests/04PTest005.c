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

//# define DEBUG

// Setup tests:
char *testGrp = "005";
uint8_t testID = 4;

#if ( ZVECT_THREAD_SAFE == 1 ) && ( OS_TYPE == 1 )

// Initialise Random Generator
static int mySeed = 25011984;
size_t max_strLen = 32;

#include <pthread.h>

// Please note: Increase the number of threads here below
//              to measure scalability of ZVector on your
//              system when using multi-threaded queues.
#define MAX_THREADS 16

#define TOTAL_ITEMS 100000000
#define MAX_ITEMS (TOTAL_ITEMS / ( MAX_THREADS / 2))
#define MAX_MSG_SIZE 72
pthread_t tid[MAX_THREADS]; // threads IDs

struct thread_args {
    int id;
    vector v;
};

typedef struct QueueItem {
	uint32_t eventID;
	uint32_t priority;
	char msg[MAX_MSG_SIZE];
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

		CCPAL_START_MEASURING;

		uint32_t i;
		QueueItem qi;
		qi.priority = 0;

		// Create a local vector (we'll use it as a partition)
		vector v2 = vect_create(MAX_ITEMS+(MAX_ITEMS/2), sizeof(struct QueueItem), ZV_NONE | ZV_NOLOCKING );

		printf("Producer Thread %*i, address of v2 is: %p\n", 3, id, v2);
		fflush(stdout);

		// Populate the local vector with the messages:
		for (i = 0; i < MAX_ITEMS; i++)
		{
			// Generate message:
			qi.eventID = ((id*MAX_ITEMS)+1)+i;
			memset(qi.msg, 65+id, max_strLen - 1);
			qi.msg[max_strLen]='\0';

			// Enqueue message on the local queue:
			// (this will copy qi into v2, so no need
			//  to free qi after, we can simply reuse it)
			vect_add(v2, &qi);
		}

		// Now move the local partition to the shared vector:
		vect_move(v, v2, 0, MAX_ITEMS);

		// We're done, display some stats and terminate the thread:
		printf("Producer thread %i done. Produced %d events.\n", id, i);
#ifdef DEBUG
		printf("--- Events in the queue right now: %*i ---\n", 4, vect_size(v));
#endif
		// No need to destroy v2, it was already destroyed automatically after the merge!

		CCPAL_STOP_MEASURING;

		// Returns perf analysis results:
		printf("Time spent to complete the thread vector operations:\n");
		CCPAL_REPORT_ANALYSIS;

#ifdef DEBUG
		printf("Signal status %*i\n\n", 8, !vect_send_signal(v));
		printf("\n\n");
#endif
		fflush(stdout);

		// Given that we have moved all the references to our message
		// set from v2 to v, we can safely destroy v2 and recover its
		// memory:
		vect_destroy(v2);

	pthread_exit(NULL);
	return NULL;
}

zvect_retval check_if_correct_size(void *v1, void *v2) {
	return ( vect_size((vector)v2) >= MAX_ITEMS );
	(void)v1;
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

		CCPAL_START_MEASURING;

		uint32_t i;
		vector v2 = vect_create(MAX_ITEMS+(MAX_ITEMS/2), sizeof(struct QueueItem), ZV_NONE | ZV_NOLOCKING);

#ifdef DEBUG
		printf("Consumer Thread %*i, address of v2 is: %p\n", 3, id, v2);
		fflush(stdout);
#endif

		// Wait for a chunk of messages to be available:
		//vect_wait_for_signal(v);
		while (true) {
			//if (vect_trylock(v))
			//if (vect_wait_for_signal(v))
			//{
				//if ( vect_size(v) >= MAX_ITEMS )
				if (!vect_move_if(v2, v, 0, MAX_ITEMS, check_if_correct_size))
				{
					//vect_move(v2, v, 0, MAX_ITEMS);
#ifdef DEBUG
					printf("Moved data from global vector to local, global vector size: %*i, local vector size: %*i\n", 8, vect_size(v), 8, vect_size(v2));
					fflush(stdout);
#endif
					//vect_unlock(v);
					// Ok, all good, we have our chunk of messages, let's
					// start processing them:
					goto START_JOB;
				}
				//printf("Main Vector size: %*i\n", 8, vect_size(v));
				//fflush(stdout);
				// No luck, let's remove the lock and repeat the process:
				//vect_unlock(v);
			//}
		}

START_JOB:
		printf("--- Consumer Thread %*i received a chunk of %*i messages ---\n\n", 3, id, 4, vect_size(v2));

		QueueItem *item; // We do not need to allocate item,
		                 // ZVector vect_remove_front will do it for us :)
		evt_counter = 0;

		for (i = 0; i < MAX_ITEMS; i++)
		{
			item  = (QueueItem *)vect_remove_front(v2);
			if ( item->msg != NULL )
			{
				evt_counter++;
				if (i < MAX_ITEMS - 1)
				{
					free(item);
					item = NULL;
				}
			}
#ifdef DEBUG
			// printf("thread %*i, item %*u\n", 10, id, 8, evt_counter);
#endif
		}


#ifdef DEBUG
		printf("Last element in the queue for Consumer Thread %*i: ID (%*d) - Message: %s\n",
			8, id, 8, item->eventID, item->msg);
#endif

	free(item);
	item = NULL;

	printf("Consumer thread %i done. Consumed %d events.\n", id, evt_counter);

	CCPAL_STOP_MEASURING;

	// Returns perf analysis results:
	printf("Time spent to complete the thread work:\n");
	CCPAL_REPORT_ANALYSIS;

	printf("\n\n");
	fflush(stdout);

	vect_destroy(v2);

	pthread_exit(NULL);
	return NULL;
}

int main() {
	// Setup
	srand((time(NULL) * max_strLen) + (++mySeed));

	CCPAL_INIT_LIB;

	printf("=== PTest%s ===\n", testGrp);
	printf("Testing Dynamic QUEUES (MULTI thread, with many threads and passing complex data structures)\n");

	fflush(stdout);

	printf("Test %s_%d: Create a Queue of %*i initial capacity and use it with %*i messages:\n", testGrp, testID, 8, TOTAL_ITEMS+(TOTAL_ITEMS/2), 8, TOTAL_ITEMS);
	fflush(stdout);

		vector v;
		v = vect_create(TOTAL_ITEMS+(TOTAL_ITEMS/2), sizeof(struct QueueItem), ZV_NONE);

	printf("done.\n");
	testID++;

	fflush(stdout);

	printf("Test %s_%d: Spin %i threads (%i producers and %i consumers) and use them to manipulate the Queue above.\n", testGrp, testID, MAX_THREADS, MAX_THREADS / 2, MAX_THREADS / 2);
	fflush(stdout);

		int err = 0;
		int i = 0;
		struct thread_args *targs[MAX_THREADS+1];

		CCPAL_START_MEASURING;

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

		CCPAL_STOP_MEASURING;

		for(i=0; i < MAX_THREADS; i++) {
			free(targs[i]);
		}

	printf("Done.\n\nAll threads have completed their job.\n");
	testID++;

	fflush(stdout);

	// Returns perf analysis results:
	CCPAL_REPORT_ANALYSIS;
	fflush(stdout);

	printf("--- Events missed in the queue: %*i ---\n\n", 4, vect_size(v));
	fflush(stdout);

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