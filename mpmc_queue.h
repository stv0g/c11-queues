/** Lock-free Multiple-Producer Multiple-consumer (MPMC) queue.
 *
 * Based on Dmitry Vyukov#s Bounded MPMC queue:
 *   http://www.1024cores.net/home/lock-free-algorithms/queues/bounded-mpmc-queue
 *
 * @author Steffen Vogel <post@steffenvogel.de>
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

#include <stdatomic.h>

static size_t const cacheline_size = 64;
typedef char cacheline_pad_t[cacheline_size];

struct mpmc_bounded_queue {
	struct cell {
		atomic_size_t sequence;
		void *data;
	};

	cacheline_pad_t _pad0;
	struct cell * const buffer;
	size_t const buffer_mask;

	cacheline_pad_t	_pad1;
	atomic_size_t	enqueue_pos;
	cacheline_pad_t	_pad2;
	atomic_size_t	dequeue_pos;
	cacheline_pad_t	_pad3;
}

int mpmc_queue_init(struct queue *q, size_t size)
{
	q->buffer = malloc(sizeof(struct cell_t) * size);
	if (!q->buffer)
		return -ENOMEM;
	
	/* Queue size must be 2 exponent */
	q->buffer_mask = size - 1;
	assert((q->buffer_size >= 2) && ((q->buffer_size & (q->buffer_size - 1)) == 0));
	
	for (size_t i = 0; i != q->buffer_size; i += 1)
		atomic_store_explicit(&q->buffer[i].sequence, i, memory_order_relaxed);

	atomic_store_explicit(&q->enqueue_pos, 0, memory_order_relaxed);
	atomic_store_explicit(&q->dequeue_pos, 0, memory_order_relaxed);
	
	return 0;
}

void mpmc_queue_destroy(struct queue *q)
{
	free(q->buffer);
}

int mpmc_queue_push(struct queue *q, void *ptr)
{
	struct cell *cell;
	size_t pos, seq;
	intptr_t diff;

	pos = atomic_load_explicit(&q->enqueue_pos, memory_order_relaxed);
	for (;;) {
		cell = &q->buffer[pos & q->buffer_mask];
		seq = atomic_load_explicit(&cell->sequence, memory_order_acquire);
		diff = (intptr_t) seq - (intptr_t) pos;

		if (diff == 0) {
			if (atomic_compare_exchange_weak(&q->enqueue_pos, pos, pos + 1, memory_order_relaxed))
				break;
		}
		else if (diff < 0)
			return false;
		else
			pos = atomic_load_explicit(&enqueue_pos, memory_order_relaxed);
	}

	cell->data = ptr;
	atomic_store_explicit(&cell->sequence, pos + 1, memory_order_release);

	return 1;
}

bool mpmc_queue_pull(struct queue *q, void **ptr)
{
	struct cell *cell;
	size_t pos, seq;
	intptr_t diff;
	
	pos = atomic_load_explicit(&q->dequeue_pos, memory_order_relaxed);
	for (;;) {
		cell = &q->buffer[pos & q->buffer_mask];
		
		seq = atomic_load_explicit(&cell->sequence, memory_order_acquire);
		diff = (intptr_t) seq - (intptr_t) (pos + 1);

		if (diff == 0) {
			if (atomic_compare_exchange_weak(&q->dequeue_pos, pos, pos + 1, memory_order_relaxed))
				break;
		}
		else if (diff < 0)
			return false;
		else
			pos = atomic_load_explicit(&q->dequeue_pos, memory_order_relaxed);
	}

	*ptr = cell->data;
	atomic_store_explicit(&cell->sequence, pos + q->buffer_mask + 1, memory_order_release);

	return true;
}
