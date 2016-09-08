#define _GNU_SOURCE

#include <stdio.h>

#include "unbounded_spsc.h"

// usage example
int main()
{
	int test[5] = {1,2,3,4,5};
	struct spsc_queue q;
	spsc_queue_init(&q);
	enqueue(&q, &test[0]);
	enqueue(&q, &test[1]);
	int* v;
	int b = dequeue(&q, (void**) &v);
	printf("dequeue 1 %d, return %d\n", *v, b);
	b = dequeue(&q, (void**) &v);
	printf("dequeue 2 %d, return %d\n", *v, b);
	enqueue(&q, &test[2]);
	enqueue(&q, &test[3]);
	b = dequeue(&q, (void**) &v);
	printf("dequeue 3 %d, return %d\n", *v, b);
	b = dequeue(&q, (void**) &v);
	printf("dequeue 4 %d, return %d\n", *v, b);
	b = dequeue(&q, (void**) &v);
	printf("dequeue 5 %d, return %d\n", *v, b);
	
	return 0;
}
