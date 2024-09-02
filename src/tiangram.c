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
 #include <openssl/sha.h>
#define BUFFER_MAX 255

typedef struct client_conn {
    char message[BUFFER_MAX+1];
    short size;
    char * alias; 
    char shat[41];
    int socket;
} client_conn;

void * serverService(void * client);
void closeConn(int clientSocket);
void user_register(struct client_conn * user);

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
    int cliSock;
    while(1){

        fprintf(stderr,"Waiting for connections\n");
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
    struct client_conn * user = (struct client_conn *) client;
    int integer=-1;
    short finish=0;

    if(user==NULL){
        closeConn(user->socket);
    }
    user_register(user);

    while(1){
            if (recv(user->socket, &integer, sizeof(int), MSG_WAITALL)!=sizeof(int))
            break;
            integer=ntohl(integer);

 
            switch(integer){
                case 0: 
                    fprintf(stderr,"Entered %d\n",integer);
                    break;
                case 1:
                    fprintf(stderr,"Entered %d\n",integer);
                    break;
                case 2:
                    fprintf(stderr,"Entered %d\n",integer);
                    break;
                case 80:
                    finish=1;
                    break;
            }

            if(finish) break;
    }

    close(user->socket);
    memset(client,0,sizeof(struct client_conn));
    free(client);
    return 0;
}

/* TODO
        Create the chat if not exists. This will be done hashing the 
        concat result of mixing the name & password, so it will create the 
        respective file of the chat. 

*/

void user_register(struct client_conn * user){
    
    int size[3]; //Los sizes tienen +1 por el \0
    struct iovec iov[3];
    for(int i=0;i<3;i++){
        iov[i].iov_len=sizeof(int);
        iov[i].iov_base=&size[i];
    }
    if(readv(user->socket, iov, 3)<12){
        closeConn(user->socket);
    }

    char * inputs[3];
    for(int i=0;i<3;i++){
        size[i]=ntohl(size[i]);
        if(size[i]>BUFFER_MAX || size[i]<0){
            closeConn(user->socket);
        }

        inputs[i]=malloc(size[i]+1);    //Ponemos el \0 para el print
        inputs[i][size[i]]='\0'; //Forzamos que tenga el tam especificado
        if(inputs[i]==NULL){
            closeConn(user->socket);
        }
        iov[i].iov_len=size[i];
        iov[i].iov_base=inputs[i];
    }
    if(readv(user->socket, iov, 3)<0){
        closeConn(user->socket);
    }
    unsigned char hash[SHA_DIGEST_LENGTH];
    if((inputs[1]=realloc(inputs[1],size[1]+size[2]+1))==NULL){
        closeConn(user->socket);
    }
    inputs[1][size[1]+size[2]]='\0';
    strcpy(inputs[1]+size[1],inputs[2]);
    printf("ROOM+PWD CONCAT: %s\n",inputs[1]);
    //SHA1((unsigned char *)inputs[1],size[0], hash);

    for (int i = 0; i < 20; i++)
    {
        printf("%02X\n", hash[i]);
    }
}

void closeConn(int clientSocket){
    close(clientSocket);
    perror("Closing connection");
    exit(-1);
}