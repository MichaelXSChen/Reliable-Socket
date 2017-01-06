CC=gcc
all: guard guard-client intercept.so con-manager

con-manager:con-manager.c common.o util.o
	gcc -c con-manager.c -I.
	gcc -o con-manager util.o con-manager.o common.o tpl.o -lpthread

intercept.so: intercept.c util.c common.c
	gcc -fPIC -c util.c -o util_pic.o -I.
	gcc -fPIC -c common.c -o common_pic.o -I.
	gcc -fPIC -c intercept.c -o intercept.o	-I.
	gcc -shared -o intercept.so util_pic.o common_pic.o intercept.o

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
