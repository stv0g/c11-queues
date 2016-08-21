#include <sys/mman.h>
#include <pthreads.h>

#include "queue.h"

#define N 1000

int test_single_threaded(struct queue *q)
{
	int i, r, n1, n2;
	
	/* Enqueue */
	for (i = 0, n1 = 0, n2 = 1; i < N; i++) {
		int fib = n1 + n2;
		
		r = queue_push(q, (void *) fib);
		if (r != 1)
			return -1;
		
		n1 = n2; n2 = fib;
	}

	/* Dequeue */
	for (i = 0, n1 = 0, n2 = 1; i < N; i++) {
		int fib = n1 + n2;
		int pulled;

		r = queue_pull(q, (void **) &pulled);
		if (r != 1)
			return -1;
		
		if (pulled != fib)
			return -1;
			
			
		n1 = n2; n2 = fib;
	}
	
	return 0;
}

int main(int argc, char *argv[])
{
	void *hp;
	struct queue *q;
	size_t len;
	
	len = 16 << 20; // 16 MiB
	
	hp = mmap(NULL, len, PROT_READ | PROT_WRITE, MAP_LOCKED | MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, -1, 0);
	if (hp == MAP_FAILED)
		return -1;
	
	q = hp;

	queue_init(q, len);

	return 0;
}