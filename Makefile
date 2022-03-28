
CC=gcc
CFLAGS=-g

TARGET = resmanager

DEPS = mid_wrapper.h
OBJ = cgroup_monitor.o mid_wrapper.o 

default: $(TARGET)
all: default tests

%.o: %.c $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $< 

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ 

tests: test_normal.c
	$(CC) $(CFLAGS) -o test_normal test_normal.c
	$(CC) $(CFLAGS) -o test_fork_bomb test_fork_bomb.c

clean:
	rm -f *.o
	rm -f $(TARGET)