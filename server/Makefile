CC1= -g -Wall 
CC1Libs= -lssl -lcrypto -lpthread 


all: hello

hello:  
	gcc $(CC1) src/tiangram.c lib/server_utilities.c -o bin/tiangram `pkg-config --cflags --libs libmicrohttpd` $(CC1Libs)

clear:
	rm -rf bin/*
	rm -rf shats/*