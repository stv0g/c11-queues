/** Lock-free Single-Producer Single-consumer (SPSC) queue.
 *
 * @author Steffen Vogel <post@steffenvogel.de>
 * @copyright 2016 Steffen Vogel
 * @license BSD 2-Clause License
 * 
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
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

#include "queue.h"

int queue_init(struct queue *q, size_t len)
{
	if(len < sizeof(struct queue) + 2*sizeof(q->pointers[0]))
		return -1;
	q->capacity = (len - sizeof(struct queue)) / sizeof(q->pointers[0]) - 1;
	
	atomic_init(&q->tail, 0);
	atomic_init(&q->head, 0);

	return 0;
}

void queue_destroy(struct queue *q)
{
	/* Nothing to do here */
	return;
}

int queue_get_many(struct queue *q, void *ptrs[], size_t cnt)
{
	int filled_slots = q->capacity - queue_free_slots(q);
	
	if(cnt > filled_slots)
		cnt = filled_slots;
	
	for(int i = 0; i < cnt; i++)
		ptrs[i] = q->pointers[q->head % (q->capacity + 1)];
	
	return cnt;
}

int queue_push_many(struct queue *q, void **ptrs, size_t cnt)
{
	//int free_slots = q->tail < q->head ? q->head - q->tail - 1 : q->head + (q->capacity - q->tail);
	int free_slots = queue_free_slots(q);
	
	if(cnt > free_slots)
		cnt = free_slots;
	
	for(int i = 0; i < cnt; i++) {
		q->pointers[q->tail] = ptrs[i]; 			//--? alternate use (q->tail + i)%(q->capacity + 1) as index and update q->tail at the end of loop
		q->tail = (q->tail + 1)%(q->capacity + 1);
	}
	
	return cnt;
}

int queue_pull_many(struct queue *q, void **ptrs, size_t cnt)
{
	int filled_slots = q->capacity - queue_free_slots(q);
	
	if(cnt > filled_slots)
		cnt = filled_slots;
	
	for(int i = 0; i < cnt; i++) {
		ptrs[i] = q->pointers[q->head];
		q->head = (q->head + 1)%(q->capacity + 1);
	}
	
	return cnt;
}

int queue_free_slots(struct queue *q)	//--? make this func inline
{
	if(q->tail < q->head)
		return q->head - q->tail - 1;
	else
		return q->head + (q->capacity - q->tail);
}