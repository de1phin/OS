.PHONY: all mq main

all: mq main

mq:
	gcc -o mq ./broker.c ./broker/broker.c -lzmq
main:
	gcc -o main ./main.c ./src/common.c ./src/node.c ./src/master.c -lzmq
