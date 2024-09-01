
#include <stdio.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/uio.h>
#include <string.h>
#include <pthread.h>
#include <sys/socket.h>
#include <errno.h>
#include "../lib/cln_util.h"

int clnSock;
char buffer[256];

//El server siempre usara el puerto 7777
int main(void){
buffer[255]='\0';

clnSock=socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

struct sockaddr_in server_address;
server_address.sin_family = PF_INET;
server_address.sin_port = htons(80);
server_address.sin_addr.s_addr = INADDR_ANY; // Se conecta a 0.0.0.0 (Cualquier ip usada en la maquina local)

int connection;
connection=connect(clnSock,(struct sockaddr *) &server_address,sizeof(server_address));
if(connection==-1){
    perror("Error with the connection");
    return -1;
}

while(1) {
    recv(clnSock,buffer,255,0);
    write(1,buffer,sizeof(buffer));
}

memset(buffer,0,256);
close(clnSock);
return 0;
}