/*
 *    Name: ITest004
 * Purpose: Integration Testing ZVector Library
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

// Setup tests:
char *testGrp = "003";
uint8_t testID = 4;

#if ( ZVECT_THREAD_SAFE == 1 ) && ( OS_TYPE == 1 )

// Initialise Random Generator
static int mySeed = 25011984;
int max_strLen = 64;

#include <pthread.h>

#define MAX_THREADS 10
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
	int evt_counter = 0;

	// Simulating Producer:
	printf("Test %s_%d: Thread %i, produce %d events, store them in the queue and check if they are stored correctly:\n", testGrp, testID, id, MAX_ITEMS);
	fflush(stdout);

		uint32_t i;
		for (i = 0; i < MAX_ITEMS; i++)
		{
			QueueItem qi;
			qi.eventID = ((id*MAX_ITEMS)+1)+i;
			// qi.msg = malloc(sizeof(char) * max_strLen);
			clear_str(qi.msg, MAX_MSG_SIZE);
			mk_rndstr(qi.msg, max_strLen - 1);
			qi.priority = 0;

			//printf("produced event message: %s\n", qi.msg);
			//vect_lock(v);

			// Let's add a new item in the queue:
			vect_add(v, &qi);

			//QueueItem item = *((QueueItem *)vect_get(v));

			//vect_unlock(v);

			// Let's test if the value we have retrieved is correct:
			printf("T %*i produced Event %*d: ID (%*d) - Message: %s\n", 2, id, 2, i, 3,
				qi.eventID, qi.msg);
			fflush(stdout);
			evt_counter++;
		}

	printf("Producer thread %i done. Produced %d events.\n", id, evt_counter);
	fflush(stdout);

	// pthread_exit(NULL);
	return NULL;
}

void *consumer(void *arg) {
	struct thread_args *targs = (struct thread_args *)arg;
	vector v = (vector)targs->v;
	int id = (int)targs->id;
	int evt_counter = 0;

	// Simulating Consumer:
	printf("Test %s_%d: Thread %i, consume %d events from the queue in FIFO order:\n", testGrp, testID, id, MAX_ITEMS);
	fflush(stdout);

		uint32_t i;
		for (i = 0; i < MAX_ITEMS;) {
			// For beginners: this is how in C we convert back a void * into the original dtata_type
			QueueItem *item = (QueueItem *)malloc(sizeof(QueueItem *));
			int fetched_item= 0;

			// Let's retrieve the value from the vector correctly:
			//vect_lock(v);

			if (!vect_is_empty(v))
			{
				item = (QueueItem *)vect_remove_front(v);
				fetched_item=1;
			}

			//vect_unlock(v);

			if ( fetched_item == 1 && item != NULL )
			{
				// Let's test if the value we have retrieved is correct:
				printf("T %*i consumed Event %*d: ID (%*d) - Message: %s\n", 2, id, 2, i, 3, item->eventID, item->msg);
				fflush(stdout);
				evt_counter++;
				i++;
			}

			free(item);
			// item = NULL;
		}

	printf("Consumer thread %i done. Consumed %d events.\n", id, evt_counter);
	fflush(stdout);

	//pthread_exit(NULL);
	return NULL;
}

int main() {
	// Setup
	srand((time(NULL) * max_strLen) + (++mySeed));

	printf("=== ITest%s ===\n", testGrp);
	printf("Testing Dynamic QUEUES (MULTI thread, with many threads)\n");

	fflush(stdout);

	printf("Test %s_%d: Create a Queue of 10 initial elements capacity:\n", testGrp, testID);
	fflush(stdout);

		vector v;
		v = vect_create(10, sizeof(struct QueueItem), ZV_SEC_WIPE);

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
				printf("Can't create thread :[%s]\n", strerror(err));
		}

		for(i=MAX_THREADS/2; i < MAX_THREADS; i++) {
			targs[i]=(struct thread_args *)malloc(sizeof(struct thread_args));
			targs[i]->id=i;
			targs[i]->v=v;
			err = pthread_create(&(tid[i]), NULL, &consumer, targs[i]);
			if (err != 0)
				printf("Can't create thread :[%s]\n", strerror(err));
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

	// Now wait until the Queue is empty:
	while(!vect_is_empty(v))
		;

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