CC1= -g -Wall -o bin/tiangram 
CC1Libs= -lssl -lcrypto -lpthread 
CC2= -g -Wall -o bin/client 


all: hello

hello:  
	gcc $(CC1) src/tiangram.c lib/srv_util.c $(CC1Libs)
	gcc $(CC2) src/client.c lib/cln_util.c

clear:
	rm -rf bin/*
	rm -rf /shats/*