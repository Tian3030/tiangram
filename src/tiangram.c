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
int user_register();

typedef struct client_conn{
    char message[256];
    short size;
    char * alias;
    char shat[41];
    int socket;
}client_conn;



int cliSock;

int main(int argc,char * argv[]){
    if (argc!=2) {
        fprintf(stderr, "Usage: %s <port>\n",argv[0]);
        return -1;
    }
    int servSock;
    //Usar el pto 80
    if ((servSock=create_server_socket(atoi(argv[1])))<0){
        return -1;
    }    
    pthread_t thid;
    pthread_attr_t atrib_th;
    pthread_attr_init(&atrib_th); 
    pthread_attr_setdetachstate(&atrib_th, PTHREAD_CREATE_DETACHED);

    while(1){

        fprintf(stderr,"Waiting for connections\n");
        if((cliSock=accept(servSock,NULL,NULL))<0){    //Conexiones mas anonimas 
            fprintf(stderr, "Error accepting the connection\n");
            return -1;
        }

    struct client_conn * client_info = calloc(1,sizeof(struct client_conn));
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
    int integer=-1;
    short finish=0;

    //Antes de enviar alias, nombre sesion y tamaños enviará las longitudes.

    

    user_register();

    while(1){
            if (recv(cliSock->socket, &integer, sizeof(int), MSG_WAITALL)!=sizeof(int))
            break;
            integer=ntohl(integer);

 
            switch(integer){
                case 0: 
                    printf("Entered %d\n",integer);
                    break;
                case 1:
                    printf("Entered %d\n",integer);
                    break;
                case 2:
                    printf("Entered %d\n",integer);
                    break;
                case 80:
                    finish=1;
                    break;
            }

            if(finish) break;
    }

    close(cliSock->socket);
    memset(client,0,sizeof(struct client_conn));
    free(client);
    exit(0);
}

/* TODO
        Create the chat if not exists. This will be done hashing the 
        concat result of mixing the name & password, so it will create the 
        respective file of the chat. 

*/

int user_register(){
    int size1,size2,size3;
    struct iovec iov[3];
    iov[0].iov_base = &size1;
    iov[0].iov_len = sizeof(int);
    iov[1].iov_base = &size2;
    iov[1].iov_len = sizeof(int);
    iov[2].iov_base = &size3;
    iov[2].iov_len = sizeof(int);

    if(readv(cliSock, iov, 3)<12){
        perror("Error reading sizes");
        return -1;
    }
    size1=ntohl(size1);
    size2=ntohl(size2);
    size3=ntohl(size3);
    struct iovec iov2[3];
    char buffer1[size1];
    char buffer2[size1];
    char buffer3[size1];

    iov2[0].iov_base = buffer1;
    iov2[0].iov_len = size1;
    iov2[1].iov_base = buffer2;
    iov2[1].iov_len = size2;
    iov2[2].iov_base = buffer3;
    iov2[2].iov_len = size3;

    if(readv(cliSock, iov2, 3)<0){
        perror("Error reading sizes");
        return -1;
    }

    printf("Los datos recibidos son: %s\n%s\n%s\n",buffer1,buffer2,buffer3);

    return 0;
}

void createChatIfNotExists(){

}

