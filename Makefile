.PHONY = all clean
CC = gcc
FLAGS = -Wall -Werror -g

all: benchmark

benchmark: benchmark.o
	$(CC) benchmark.o $(FLAGS) -o benchmark

benchmark.o: benchmark.c
	$(CC) $(FLAGS) -c benchmark.c

clean:
	rm -f *.a *.o *.so *.gch benchmark