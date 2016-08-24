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
	//q->capacity = floor((len - sizeof(queue)) / sizeof(struct queue_element));
	q->capacity = div(len - sizeof(q), sizeof(struct queue_element)).quot;
	atomic_init(&q->free_ptrs, q->capacity - 1); /** One slot wasted to distinguish empty queue from full queue*/
	
	atomic_init(&q->tail, 0);
	atomic_init(&q->head, 0);

	return 0;
}

void queue_destroy(struct queue *q)
{
	/* Nothing to do here */
	return;
}

int queue_get(struct queue *q, void **ptr)
{
	*ptr = q->pointers[q->head % q->capacity];		//--? verify
	return 0;
}

int queue_push(struct queue *q, void *ptr)
{
	if(atomic_load(&q->free_ptrs) == 0)			//--? is atomic_load enough...other thread can make the value 0 after this check before pushing elem
		return ENOMEM;							//--? should atomic_compare_exchange_weak be used
	
	struct queue_element * elem = (struct queue_element *) *(q->pointers);		//--? verify
	//elem[atomic_load(&q->tail)].value = atomic_load(&ptr);
	
	if(atomic_load(&q->tail) == (q->capacity - 1) && atomic_load(&q->head) != 0) {	/** check if queue is full */
		atomic_store(&elem[atomic_load(&q->tail)].value, ptr);
		atomic_store(&q->tail, 0);
	}
	else if((atomic_load(&q->tail) + 1) != atomic_load(&q->head)) {	//--? is this check needed, if no free_ptrs then condition already fulfulled
		atomic_store(&elem[atomic_load(&q->tail)].value, ptr);
		atomic_fetch_add(&q->tail, 1);
	}
	else
		return ENOMEM;
	
	atomic_fetch_sub(&q->free_ptrs, 1);
	
	return 0;
}

int queue_pull(struct queue *q, void **ptr)
{
	if(atomic_load(&q->free_ptrs) == (q->capacity - 1))		/** Queue empty*/
		return -1;
	
	struct queue_element * elem = (struct queue_element *) *(q->pointers);		//--? verify
	
	if(atomic_load(&q->head) == (q->capacity - 1) && atomic_load(&q->tail) != (q->capacity - 1)) {	/** check if queue is empty */
		//atomic_store(elem[atomic_load(q->tail)].value, ptr);
		ptr = (void ** ) &elem[atomic_load(&q->head)].value;	//--? verify
		atomic_store(&q->head, 0);
	}
	else if(atomic_load(&q->head) != atomic_load(&q->tail)) {
		ptr = (void ** ) &elem[atomic_load(&q->head)].value;
		atomic_fetch_add(&q->head, 1);
	}
	else
		return ENOMEM;
	
	atomic_fetch_add(&q->free_ptrs, 1);
	
	return 0;
}

int queue_get_many(struct queue *q, void *ptrs[], size_t cnt)
{
	return 0;
}

int queue_push_many(struct queue *q, void **ptrs, size_t cnt)
{
	return 0;
}

int queue_pull_many(struct queue *q, void **ptrs, size_t cnt)
{
	return 0;
}