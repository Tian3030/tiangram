CC1= -g -o bin/Tiangram  -Wall


all: hello

hello:  
	gcc $(CC1) src/tiangram.c

clear:
	rm -rf bin/*
	rm -rf /shats/*