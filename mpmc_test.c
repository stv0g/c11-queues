#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>

/* There are not many compilers yet which support threads.h natively.
 * We will fallback to a drop-in replacement which is based on pthreads.h */
#include "c11threads.h"

/* Required to allocate hugepages on Apple OS X */
#ifdef __MACH__
  #include <mach/vm_statistics.h>
#endif

/* For thread_get_id() */
#ifdef __MACH__
  #include <pthread.h>
#elif defined(__linux__)
  #include <sys/types.h>
#endif

#include "mpmc_queue.h"

size_t const thread_count = 4;
size_t const batch_size = 1;
size_t const iter_count = 2000000;

int volatile g_start = 0;

void * malloc_hp(size_t len)
{
	int prot = PROT_READ | PROT_WRITE;
	int flags = MAP_PRIVATE | MAP_ANONYMOUS;
	
#ifdef __MACH__
	flags |= VM_FLAGS_SUPERPAGE_SIZE_2MB;
#elif defined(__linux__)
	flags |= MAP_HUGETLB | MAP_LOCKED;
#endif
	
	return mmap(NULL, len, prot, flags, -1, 0);
}

/** Get CPU timestep counter */
__attribute__((always_inline)) static inline uint64_t rdtscp()
{
	uint64_t tsc;

	__asm__ ("rdtscp;"
		 "shl $32, %%rdx;"
		 "or %%rdx,%%rax"
		: "=a" (tsc)
		:
		: "%rcx", "%rdx", "memory");

	return tsc;
}

__attribute__((always_inline)) static inline void nop()
{
	__asm__("rep nop;");
}

uint64_t thread_get_id()
{
#ifdef __MACH__
	uint64_t id;
	pthread_threadid_np(pthread_self(), &id);
	return id;
#elif defined(__linux__)
	return (int) gettid();
#endif
}

int thread_func(void *ctx)
{
	struct mpmc_queue *q = (struct mpmc_queue *) ctx;

	srand((unsigned) time(0) + thread_get_id());
	size_t pause = rand() % 1000;

	/* Wait for global start signal */
	while (g_start == 0)
		thrd_yield();

	/* Wait for a random time */
	for (size_t i = 0; i != pause; i += 1)
		nop();

	for (int iter = 0; iter != iter_count; ++iter) {
		for (size_t i = 0; i != batch_size; i += 1) {
			void *ptr = (void *) i;
			while (!mpmc_queue_push(q, ptr))
				thrd_yield(); // queue full, let other threads proceed
		}

		for (size_t i = 0; i != batch_size; i += 1) {
			void *ptr;
			while (!mpmc_queue_pull(q, &ptr))
				thrd_yield(); // queue empty, let other threads proceed
		}
	}

	return 0;
}

int main()
{
	struct mpmc_queue queue;
	thrd_t threads[thread_count];
	
	mpmc_queue_init(&queue, 1024, malloc_hp);

	for (int i = 0; i != thread_count; ++i)
		thrd_create(&threads[i], thread_func, &queue);

	sleep(1);

	uint64_t start = rdtscp();
	g_start = 1;

	for (int i = 0; i != thread_count; ++i)
		thrd_join(threads[i], NULL);

	uint64_t end = rdtscp();
	
	printf("cycles/op = %llu\n", (end - start) / (batch_size * iter_count * 2 * thread_count));
}