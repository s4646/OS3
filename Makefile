.PHONY = all run clean
CC = gcc
FLAGS = -Wall -Werror -g

all: benchmark

run:
	./benchmark
	rm socket clisocket servsocket

benchmark: benchmark.o
	$(CC) benchmark.o $(FLAGS) -lpthread -o benchmark

benchmark.o: benchmark.c
	$(CC) $(FLAGS) -c benchmark.c

clean:
	rm -f *.a *.o *.so *.gch socket clisocket servsocket data.txt benchmark