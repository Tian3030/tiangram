CC1= -g -Wall 
CC1Libs= -lssl -lcrypto -lpthread 


all: hello

hello:  
	g++ $(CC1) src/tiangram.cpp lib/server_utilities.cpp -o bin/tiangram `pkg-config --cflags --libs libmicrohttpd` $(CC1Libs)

clear:
	rm -rf bin/*
	rm -rf shats/*