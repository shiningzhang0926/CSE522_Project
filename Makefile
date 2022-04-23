
CC=gcc
CFLAGS=-g -lpthread

TARGET = resmanager

DEPS = mid_wrapper.h
OBJ = resmamager.o mid_wrapper.o 

default: $(TARGET) tests
all: default tests

%.o: %.c $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $< 

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ 

tests: test_normal.c test_fork_bomb.c test_increase.c parallel_dense_mm.c
	$(CC) $(CFLAGS) -o test_normal test_normal.c
	$(CC) $(CFLAGS) -o test_fork_bomb test_fork_bomb.c
	$(CC) $(CFLAGS) -o test_increase test_increase.c
	$(CC) $(CFLAGS) -o parallel_dense_mm parallel_dense_mm.c -fopenmp

clean:
	rm -f *.o
	rm -f $(TARGET) test_increase test_fork_bomb test_normal parallel_dense_mm