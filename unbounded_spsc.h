/** Lock-free Unbounded Single-Producer Single-consumer (SPSC) queue.
 *
 * Based on Dmitry Vyukov's Unbounded SPSC queue:
 *   http://www.1024cores.net/home/lock-free-algorithms/queues/unbounded-spsc-queue
 *
 * @author Umar Farooq <umar1.farooq1@gmail.com>
 * @copyright 2016 Steffen Vogel
 * @license BSD 2-Clause License
 * 
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modiffication, are permitted provided that the following conditions are met:
 * 
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 * 
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _SPSC_UNBOUNDED_H_
#define _SPSC_UNBOUNDED_H_

#include <stdlib.h>
#include <sys/mman.h>
#include <stdatomic.h>

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

void spsc_queue_init(struct spsc_queue* q)
{
	struct node* n = (struct node *) mmap(NULL, sizeof(struct node), PROT_READ|PROT_WRITE, MAP_ANONYMOUS|MAP_PRIVATE, -1, 0);
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
	// if attempt fails, allocates node via mmap()

	if (q->first_ != q->tail_copy_)
	{
		struct node* n = q->first_;
		q->first_ = q->first_->next_;
		return n;
	}
	//q->tail_copy_ = load_consume(q->tail_);
	q->tail_copy_ = atomic_load_explicit(&q->tail_, memory_order_consume);
	
	if (q->first_ != q->tail_copy_)
	{
		struct node* n = q->first_;
		q->first_ = q->first_->next_;
		return n;
	} 
	struct node* n = mmap(NULL, sizeof(struct node), PROT_READ|PROT_WRITE, MAP_ANONYMOUS|MAP_PRIVATE, -1, 0);
	return n;
}

void enqueue(struct spsc_queue* q, void * v)
{
	struct node* n = alloc_node(q);
	n->next_ = NULL;
	n->value_ = v;
	//store_release(&(q->head_->next_), n);
	atomic_store_explicit(&(q->head_->next_), n, memory_order_release);
	q->head_ = n;
	
	return;
}

int dequeue(struct spsc_queue* q, void** v)
{
	if (atomic_load_explicit(&(q->tail_->next_), memory_order_consume))
	{
		*v = q->tail_->next_->value_;
		//store_release(&q->tail_, q->tail_->next_);
		atomic_store_explicit(&q->tail_, q->tail_->next_, memory_order_release);
		return 0;
	}
	
	return -1;
}

#endif //_SPSC_UNBOUNDED_H_