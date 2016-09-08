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

#include "spsc_ub_queue.h"

void spsc_ub_queue_init(struct spsc_ub_queue* q, size_t size, const struct memtype *mem)
{
	q->mem = mem;
	struct node* n = memory_alloc(q->mem, sizeof(struct node) * size);
	n->next_ = NULL;
	q->tail_ = q->head_ = q->first_= q->tail_copy_ = n;
	
	return;
}

void spsc_ub_queue_destroy(struct spsc_ub_queue* q)
{
	struct node* n = q->first_;
	do
	{
		struct node* next = n->next_;
		memory_free(q->mem, (void *) n, sizeof(struct node));
		n = next;
	}
	while (n);
}

struct node* spsc_ub_alloc_node(struct spsc_ub_queue* q)
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
	struct node* n = memory_alloc(q->mem, sizeof(struct node));
	return n;
}

void spsc_ub_enqueue(struct spsc_ub_queue* q, void * v)
{
	struct node* n = spsc_ub_alloc_node(q);
	n->next_ = NULL;
	n->value_ = v;
	//store_release(&(q->head_->next_), n);
	atomic_store_explicit(&(q->head_->next_), n, memory_order_release);
	q->head_ = n;
	
	return;
}

int spsc_ub_dequeue(struct spsc_ub_queue* q, void** v)
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
