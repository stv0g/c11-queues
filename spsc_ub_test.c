#ifdef __linux__
	#define _GNU_SOURCE
#endif
	
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>

#include "spsc_ub_queue.h"
#include "memory.h"
#include "c11threads.h"

/* Usage example */

#define N 20000000

/* Static global storage */
int fibs[N];

int producer(void *ctx)
{
	printf("producer\n"); //DELETEME
	struct spsc_ub_queue *q = (struct spsc_ub_queue *) ctx;
	
	/* Enqueue */
	for (unsigned long count = 0, n1 = 0, n2 = 1; count < N; count++) {
		fibs[count] = n1 + n2;
		
		void *fibptr = (void *) &fibs[count];
		
		if (spsc_ub_push(q, fibptr)) {
			printf("Queue push failed\n");
			return -1;
		}
			
		n1 = n2; n2 = fibs[count];
	}
	
	return 0;
}

int consumer(void *ctx)
{
	printf("consumer\n"); 	//DELETEME
	struct spsc_ub_queue *q = (struct spsc_ub_queue *) ctx;
	
	/* Dequeue */
	for (unsigned long count = 0, n1 = 0, n2 = 1; count < N; count++) {
		int fib = n1 + n2;
		int *pulled;
		
		while (spsc_ub_pull(q, (void **) &pulled)) {
			//printf("Queue empty: %d\n", temp);
			//return -1;
		}
		
		if (*pulled != fib) {
			printf("Pulled != fib\n");
			return -1;
		}		
		
		n1 = n2; n2 = fib;
	}
	
	return 0;
}

int test_single_threaded(struct spsc_ub_queue *q)
{
	int resp, resc;
	
	resp = producer(q);
	if (resp)
		printf("Enqueuing failed");
	
	resc = consumer(q);
	if (resc)
		printf("Consumer failed");
	
	if (resc || resp)
		printf("Single Thread Test Failed");
	else
		printf("Single Thread Test Complete\n");
	
	return 0;
}

int test_multi_threaded(struct spsc_ub_queue *q)
{
	thrd_t thrp, thrc;
	int resp, resc;
	
	thrd_create(&thrp, producer, q);	/** @todo Why producer thread runs earlier? */
	thrd_create(&thrc, consumer, q);
	
	thrd_join(thrp, &resp);
	thrd_join(thrc, &resc);
	
	if (resc || resp)
		printf("Queue Test failed");
	else
		printf("Two-thread Test Complete\n");
	
	return 0;
}

int main()
{
	struct spsc_ub_queue q;
	spsc_ub_queue_init(&q, 1, &memtype_heap);	/** @todo change size>1 in case of bounded queue impl. memtype_hugepage impl for un_spsc */
	
	test_single_threaded(&q);
	test_multi_threaded(&q);
	
	return 0;
}
