TARGETS = spsc_port_test
CFLAGS = -Wall -std=c11

DEBUG ?= 1

ifdef DEBUG
	CFLAGS += -g -O0
else
	CFLAGS += -O3
endif

.PHONY: all clean

all: $(TARGETS)

spsc_port_test: spsc_port_test.o
	$(CC) $^ -Wall $(LIBS) -o $@

#mpmc_test: mpmc_test.o mpmc_queue.o memory.o
#	$(CC) $^ -Wall $(LIBS) -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o
	rm -f $(TARGETS)
