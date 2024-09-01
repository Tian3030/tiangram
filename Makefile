CC1= -g -Wall -lpthread -o bin/tiangram 
CC2= -g -Wall -o bin/client 


all: hello

hello:  
	gcc $(CC1) src/tiangram.c lib/srv_util.c
	gcc $(CC2) src/client.c lib/cln_util.c

clear:
	rm -rf bin/*
	rm -rf /shats/*