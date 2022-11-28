.PHONY: 1 2

all: 1 2

1:
	gcc -c ./primes/primes.c
	gcc -c ./area/area.c
	gcc -c ./1.c
	gcc -g -o ./1 ./1.o ./primes.o ./area.o

2: 
	gcc -shared -fPIC -o ./primes.so ./primes/primes.c
	gcc -shared -fPIC -o ./area.so ./area/area.c
	gcc -o ./2 ./2.c
