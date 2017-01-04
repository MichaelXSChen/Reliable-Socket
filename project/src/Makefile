CC=gcc
all: guard guard-client

guard: guard.c guard.o tpl.o common.o util.o
	gcc -c guard.c -I.
	gcc -o guard guard.o tpl.o common.o util.o
	
guard-client: guard-client.c common.o tpl.o util.o
	gcc -c guard-client.c -I.
	gcc -o guard-client guard-client.o common.o tpl.o util.o

common.o: common.c
	gcc -c common.c -I.

tpl.o: tpl.c
	gcc -c tpl.c -I.

util.o: util.c
	gcc -c util.c -I.
