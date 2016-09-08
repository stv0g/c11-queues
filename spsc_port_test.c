#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>

#include "unbounded_spsc.h"
#include "memory.h"

// usage example
int main()
{
	int test[5] = {1,2,3,4,5};
	struct spsc_ub_queue q;
	spsc_ub_queue_init(&q, 1, &memtype_hugepage);	/** @todo change size from 1 in case of bounded queue impl */
	spsc_ub_enqueue(&q, &test[0]);
	spsc_ub_enqueue(&q, &test[1]);
	int* v;
	int b = spsc_ub_dequeue(&q, (void**) &v);
	printf("dequeue 1 %d, return %d\n", *v, b);
	b = spsc_ub_dequeue(&q, (void**) &v);
	printf("dequeue 2 %d, return %d\n", *v, b);
	spsc_ub_enqueue(&q, &test[2]);
	spsc_ub_enqueue(&q, &test[3]);
	b = spsc_ub_dequeue(&q, (void**) &v);
	printf("dequeue 3 %d, return %d\n", *v, b);
	b = spsc_ub_dequeue(&q, (void**) &v);
	printf("dequeue 4 %d, return %d\n", *v, b);
	b = spsc_ub_dequeue(&q, (void**) &v);
	printf("dequeue 5 %d, return %d\n", *v, b);
	
	return 0;
}
