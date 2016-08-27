TARGETS = test mpmc_test
CFLAGS = -Wall -std=c11

ifdef DEBUG
	CFLAGS += -g -O0
else
	CFLAGS += -O3
endif

.PHONY: all clean

all: $(TARGETS)

test: test.o queue.o
	$(CC) $^ -Wall $(LIBS) -o $@

mpmpc_test: mpmc_test.o
	$(CC) $^ -Wall $(LIBS) -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o
	rm -f $(TARGET)
