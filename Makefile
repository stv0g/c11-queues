TARGETS = spsc_ub_test mpmc_test spsc_test
CFLAGS = -Wall -std=c11

DEBUG ?= 1

ifdef DEBUG
	CFLAGS += -g -O0
else
	CFLAGS += -O3
endif

.PHONY: all clean

all: $(TARGETS)

spsc_ub_test: spsc_ub_test.o spsc_ub_queue.o memory.o
	$(CC) $^ -Wall $(LIBS) -o $@

mpmc_test: mpmc_test.o mpmc_queue.o memory.o
	$(CC) $^ -Wall $(LIBS) -o $@

spsc_test: spsc_test.o spsc_queue.o memory.o
	$(CC) $^ -Wall $(LIBS) -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o
	rm -f $(TARGETS)
