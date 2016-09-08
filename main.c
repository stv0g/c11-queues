#include "bounded_spsc_wip.h"

// usage example 
int main() 
{ 
	int test = 1;
	struct spsc_queue q;
	spsc_queue_init(&q);
	enqueue(&q, &test);
	test++;
	enqueue(&q, &test); 
	int* v; 
	int b = dequeue(&q, (void**) &v); 
	b = dequeue(&q, (void**) &v);
	test++;
	enqueue(&q, &test);
	test++;
	enqueue(&q, &test); 
	b = dequeue(&q, (void**) &v); 
	b = dequeue(&q, (void**) &v); 
	b = dequeue(&q, (void**) &v); 
	
	if(b)
		return -1;
}