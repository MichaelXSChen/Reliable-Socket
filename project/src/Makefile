CC=gcc
all: guard-client intercept.so guest-daemon tcp cli-test


intercept.so: intercept.c common.c
	gcc -fPIC -c common.c -o common_pic.o -I.
	gcc -fPIC -c intercept.c -o intercept.o	-I.
	gcc -fPIC -c tpl.c -o tpl_pic.o -I.
	gcc -fPIC -shared -o intercept.so common_pic.o intercept.o tpl_pic.o -ldl

# guard: guard.c guard.o tpl.o common.o
# 	gcc -c guard.c -I.
# 	gcc -o guard guard.o tpl.o common.o
	
guard-client: guard-client.c common.o tpl.o 
	gcc -c guard-client.c -I.
	gcc -o guard-client guard-client.o common.o tpl.o 

common.o: common.c
	gcc -c common.c -I.

tpl.o: tpl.c
	gcc -c tpl.c -I.


con-manager-guest.o: con-manager-guest.c
	gcc -c con-manager-guest.c -I.

guest-daemon: guest-daemon.c con-manager-guest.o tpl.o common.o
	gcc -c guest-daemon.c -I.
	gcc -o guest-daemon common.o guest-daemon.o tpl.o con-manager-guest.o -lpthread

tcp: tcp.c tpl.o common.o
	gcc -c tcp.c
	gcc -o tcp tcp.o tpl.o common.o

cli-test: test.c common.o tpl.o 
	gcc -c test.c 
	gcc -o cli-test test.o common.o tpl.o 

.PHONY: clean

clean:
	rm -f *.o *.so
