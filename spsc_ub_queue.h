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

#ifndef _SPSC_UB_QUEUE_H_
#define _SPSC_UB_QUEUE_H_

#include <stdatomic.h>

#include "memory.h"

// cache line size on modern x86 processors (in bytes)
#define CACHE_LINE_SIZE 64

struct node
{
	struct node* _Atomic next_;
	void *value_;
};

struct spsc_ub_queue 
{
	struct memtype const * mem;
	char cache_line_pad_0_ [CACHE_LINE_SIZE];
	
	// consumer part 
	// accessed mainly by consumer, infrequently be producer 
	struct node* _Atomic tail_; // tail of the queue 

	// delimiter between consumer part and producer part, 
	// so that they situated on different cache lines 
	char cache_line_pad_ [CACHE_LINE_SIZE]; 

	// producer part 
	// accessed only by producer 
	struct node* _Atomic head_; // head of the queue 
	struct node* _Atomic first_; // last unused node (tail of node cache) 
	struct node* _Atomic tail_copy_; // helper (points somewhere between first_ and tail_)
};

void spsc_ub_queue_init(struct spsc_ub_queue* q, size_t size, const struct memtype *mem);

void spsc_ub_queue_destroy(struct spsc_ub_queue* q);

struct node* spsc_ub_alloc_node(struct spsc_ub_queue* q);

void spsc_ub_enqueue(struct spsc_ub_queue* q, void * v);

int spsc_ub_dequeue(struct spsc_ub_queue* q, void** v);

#endif /* _SPSC_UB_QUEUE_H_ */