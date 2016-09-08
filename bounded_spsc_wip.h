//http://www.1024cores.net/home/lock-free-algorithms/queues/unbounded-spsc-queue 

#ifndef _SPSC_UNBOUNDED_H_
#define _SPSC_UNBOUNDED_H_

#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/mman.h>

// cache line size on modern x86 processors (in bytes)
#define CACHE_LINE_SIZE 64

struct node 
{ 
	struct node* next_; 
	void* value_; 
}; 

struct spsc_queue 
{
	// consumer part 
	// accessed mainly by consumer, infrequently be producer 
	struct node* tail_; // tail of the queue 

	// delimiter between consumer part and producer part, 
	// so that they situated on different cache lines 
	char cache_line_pad_ [CACHE_LINE_SIZE]; 

	// producer part 
	// accessed only by producer 
	struct node* head_; // head of the queue 
	struct node* first_; // last unused node (tail of node cache) 
	struct node* tail_copy_; // helper (points somewhere between first_ and tail_)
};

// load with 'consume' (data-dependent) memory ordering
void * load_consume(struct node const* addr) 
{ 
	return NULL;
	/*
	// hardware fence is implicit on x86 
	struct spsc_queue v = (struct spsc_queue const volatile*)(addr); 
	__memory_barrier(); // compiler fence 
	return v;
		*/
} 

// store with 'release' memory ordering 
void store_release(struct spsc_queue* addr, struct spsc_queue v) 
{ 
	return;
	/*
	// hardware fence is implicit on x86 
	__memory_barrier(); // compiler fence 
	*const_cast<struct spsc_queue volatile*>(addr) = v;
	*/
} 

void spsc_queue_init(struct spsc_queue* q) 
{
	struct node* n = mmap(NULL, sizeof(struct node), PROT_READ | PROT_WRITE, 0, -1, 0);
	n->next_ = NULL; 
	q->tail_ = q->head_ = q->first_= q->tail_copy_ = n;	
	
	return;
}

void spsc_queue_destroy(struct spsc_queue* q)
{
	struct node* n = q->first_; 
	do 
	{ 
		struct node* next = n->next_; 
		munmap((void *) n, sizeof(struct node));
		n = next;
	} 
	while (n);
}

struct node* alloc_node(struct spsc_queue* q)
{
	// first tries to allocate node from internal node cache,
	// if attempt fails, allocates node via ::operator new()

	if (q->first_ != q->tail_copy_)
	{
		struct node* n = q->first_;
		q->first_ = q->first_->next_;
		return n;
	}
	q->tail_copy_ = load_consume(q->tail_);
	
	if (q->first_ != q->tail_copy_)
	{
		struct node* n = q->first_;
		q->first_ = q->first_->next_;
		return n;
	} 
	struct node* n = mmap(NULL, sizeof(struct node), PROT_READ | PROT_WRITE, 0, -1, 0);
	return n;
}

void enqueue(struct spsc_queue* q, void * v) 
{ 
	struct node* n = alloc_node(q); 
	n->next_ = NULL; 
	n->value_ = v; 
	store_release(&(q->head_->next_), n); 
	q->head_ = n; 
	
	return;
}

int dequeue(struct spsc_queue* q, void** v)
{
	if (load_consume(&(q->tail_->next_))) 
	{ 
		*v = q->tail_->next_->value_; 
		store_release(&q->tail_, q->tail_->next_); 
		return 0; 
	} 
	
	return -1;
}

#endif //_SPSC_UNBOUNDED_H_