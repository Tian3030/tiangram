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
#include "../lib/srv_util.h"

void * serverService(void * client);

typedef struct message{
    char message[256];
    short size;
    char * alias;
    char shat[41];
}message;

typedef struct client_conn {
    int socket; // añadir los campos necesarios
}client_conn;


unsigned short * port;
int servSock;

int main(int argc,char * argv[]){

    if (argc!=2) {
        fprintf(stderr, "Usage: %s <port>\n",argv[0]);
        return -1;
    }

    port = malloc(sizeof(short));
    //Intentar usar el pto 80
    servSock=create_server_socket(atoi(argv[1]),port);

    if (servSock<0){
        return -1;
    }    
    pthread_t thid;
    pthread_attr_t atrib_th;
    pthread_attr_init(&atrib_th); 
    pthread_attr_setdetachstate(&atrib_th, PTHREAD_CREATE_DETACHED);

    while(1){

        fprintf(stderr,"Waiting for connections\n");
        int cliSock;
        if((cliSock=accept(servSock,NULL,NULL))<0){    //Conexiones mas anonimas 
            fprintf(stderr, "Error accepting the connection\n");
            return -1;
        }

    struct client_conn * client_info = malloc(sizeof(struct client_conn));
    client_info->socket=cliSock;
    
    pthread_create(&thid,&atrib_th,serverService,client_info);   
    }    

    close(servSock);

    return 0;
}

/*  TODO
    Codes:
        0)  Log in to a chat with name & password of a session
        1)  Send message into a chat (with alias as nickname)
        2)  Read messages from a chat

*/
void * serverService(void * client){
    struct client_conn * cliSock = (struct client_conn *) client;
    while(1){
        






    }

    close(cliSock->socket);
    exit(0);
}

/* TODO
        Create the chat if not exists. This will be done hashing the 
        concat result of mixing the name & password, so it will create the 
        respective file of the chat. 

*/
void createChatIfNotExists(){

}