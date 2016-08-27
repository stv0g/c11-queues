#include <sys/mman.h>
#include <pthread.h>
#include <stdio.h>

#include "queue.h"

#define N 5

int test_single_threaded(struct queue *q)
{	
	int count, r, n1, n2;
	int * fib_ptr[N];
	int fib1[N];
	
	/* Enqueue */
	for (count = 0, n1 = 0, n2 = 1; count < N; count++) {
		fib1[count] = n1 + n2;
		
		fib_ptr[count] = &fib1[count];
		
		r = queue_push(q, (void *) &fib_ptr[count]);
		if (r != 1) {
			printf("Queue push failed\n");
			return -1;
		}
			
		n1 = n2; n2 = fib1[count];
	}

	/* Dequeue */
	for (count = 0, n1 = 0, n2 = 1; count < N; count++) {
		int fib2 = n1 + n2;
		int *pulled;

		r = queue_pull(q, (void **) &pulled);
		if (r != 1) {
			printf("Queue pull failed\n");
			return -1;
		}
		
		if (*pulled != fib2) {
			printf("Pulled != fib\n");
			return -1;
		}		
		
		n1 = n2; n2 = fib2;
	}
	
	printf("Test Complete\n");
	
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
	
	//len = 16 << 20; // 16 MiB
	len = 140;
	
	hp = mmap(NULL, len, PROT_READ | PROT_WRITE, MAP_LOCKED | MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, -1, 0); //--? sudo previleges needed to mount hugepages and run the executable....automate in the code
	if (hp == MAP_FAILED) {
		printf("Memory assignment error\n");
		return -1;
	}
	
	q = hp;

	if(queue_init(q, len)) {
		printf("Error in initialization\n");
		return -1;
	}
	
	queue_info(q);
	test_single_threaded(q);
	
	return 0;
}