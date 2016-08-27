#include <sys/mman.h>
#include <stdio.h>

/* Required to allocate hugepages on Apple OS X */
#ifdef __MACH__
  #include <mach/vm_statistics.h>
#endif

/* There are not many compilers yet which support threads.h natively.
 * We will fallback to a drop-in replacement which is based on pthreads.h */
#include "c11threads.h"

#include "queue.h"

#define N 2000000

/* Static global storage */
int fibs[N];

void * malloc_hp(size_t len)
{
	int prot = PROT_READ | PROT_WRITE;
	int flags = MAP_PRIVATE | MAP_ANONYMOUS;
	
#ifdef __MACH__
	flags |= VM_FLAGS_SUPERPAGE_SIZE_2MB;
#elif defined(__linux__)
	flags |= MAP_HUGETLB | MAP_LOCKED;
#endif
	
	return mmap(NULL, len, prot, flags, -1, 0);
}

int producer(void *ctx)
{
	struct queue *q = (struct queue *) ctx;
	
	/* Enqueue */
	for (int count = 0, n1 = 0, n2 = 1, r; count < N; count++) {
		fibs[count] = n1 + n2;
		
		void *fibptr = (void *) &fibs[count];
		
		r = queue_push(q, &fibptr);
		if (r != 1) {
			printf("Queue push failed\n");
			return -1;
		}
			
		n1 = n2; n2 = fibs[count];
	}
	
	return 0;
}

int consumer(void *ctx)
{
	struct queue *q = (struct queue *) ctx;
	
	/* Dequeue */
	for (int count = 0, n1 = 0, n2 = 1, r; count < N; count++) {
		int fib = n1 + n2;
		int *pulled;

		r = queue_pull(q, (void **) &pulled);
		if (r != 1) {
			printf("Queue pull failed: %d\n", r);
			return -1;
		}
		
		if (*pulled != fib) {
			printf("Pulled != fib\n");
			return -1;
		}		
		
		n1 = n2; n2 = fib;
	}
	
	
	return 0;
}

int test_single_threaded(struct queue *q)
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

int test_multi_threaded(struct queue *q)
{
	thrd_t thrp, thrc;
	int resp, resc;
	
	thrd_create(&thrp, producer, q);
	thrd_create(&thrc, consumer, q);
	
	thrd_join(thrp, &resp);
	thrd_join(thrc, &resc);
	
	if (resc || resp)
		printf("Queue Test failed");
	else
		printf("Two-thread Test Complete\n");
	
	return 0;
}

void queue_info(struct queue *q) 
{
	printf("Queue tail %d, queue head %d, queue capacity %lu, Queue free slots %d\n", q->tail, q->head, q->capacity, queue_free_slots(q));
	return;
}

int main(int argc, char *argv[])
{
	void * hp;
	struct queue *q;
	size_t len;
	
	len = 16 << 20; // 16 MiB
	
	hp = malloc_hp(len);
	if (hp == MAP_FAILED) {
		printf("Memory assignment error\n");
		return -1;
	}
	
	q = hp;

	if (queue_init(q, len)) {
		printf("Error in initialization\n");
		return -1;
	}
	
	queue_info(q);
	
	test_single_threaded(q);
	test_multi_threaded(q);
	
	return 0;
}