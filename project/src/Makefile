CC=gcc
all: guard guard-client intercept.so con-manager

con-manager:con-manager.c common.o 
	gcc -c con-manager.c -I.
	gcc -o con-manager con-manager.o common.o tpl.o -lpthread

intercept.so: intercept.c common.c
	gcc -fPIC -c common.c -o common_pic.o -I.
	gcc -fPIC -c intercept.c -o intercept.o	-I.
	gcc -fPIC -c tpl.c -o tpl_pic.o -I.
	gcc -fPIC -shared -o intercept.so common_pic.o intercept.o tpl_pic.o -ldl

guard: guard.c guard.o tpl.o common.o
	gcc -c guard.c -I.
	gcc -o guard guard.o tpl.o common.o
	
guard-client: guard-client.c common.o tpl.o 
	gcc -c guard-client.c -I.
	gcc -o guard-client guard-client.o common.o tpl.o 

common.o: common.c
	gcc -c common.c -I.

tpl.o: tpl.c
	gcc -c tpl.c -I.

con-hashmap.o: con-hashmap.c
	gcc -c con-hashmap.c -I.

con-manager-guest: con-manager-guest.c con-hashmap.o common.o tpl.o
	gcc -c con-manager-guest.c -I.
	gcc -o con-manager-guest con-manager-guest.o con-hashmap.o common.o tpl.o -lpthread
