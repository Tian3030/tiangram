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

char * getHexHash(struct client_conn * user,unsigned char * hash);
void * serverService(void * client);
void closeConn(struct client_conn * client);
void user_register(struct client_conn * user);
void createChatIfNotExists(struct client_conn * client);

const char shatsPath[]="shats/";


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
        closeConn(user);
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
        closeConn(user);
    }

    char * inputs[3];
    for(int i=0;i<3;i++){
        size[i]=ntohl(size[i]);
        if(size[i]>BUFFER_MAX || size[i]<0){
            closeConn(user);
        }

        inputs[i]=malloc(size[i]+1);    //Ponemos el \0 para el print
        inputs[i][size[i]]='\0'; //Forzamos que tenga el tam especificado
        if(inputs[i]==NULL){
            closeConn(user);
        }
        iov[i].iov_len=size[i];
        iov[i].iov_base=inputs[i];
    }
    if(readv(user->socket, iov, 3)<0){
        closeConn(user);
    }
    //Alias set
    user->alias=inputs[0];      //BORRAR INPUTS 1 Y 2 NO EL 0
    //Hash calculation
    unsigned char hash[SHA_DIGEST_LENGTH];
    if((inputs[1]=realloc(inputs[1],size[1]+size[2]+1))==NULL){
        closeConn(user);
    }
    inputs[1][size[1]+size[2]]='\0';
    strcpy((inputs[1]+size[1]),inputs[2]);
    SHA1((unsigned char *)inputs[1],size[1]+size[2], hash);

    char * chatTemp=getHexHash(user,hash);
    strcpy(user->shat,chatTemp);
    fprintf(stderr,"CHATCODE: %s\n",user->shat);
    createChatIfNotExists(user);

    memset(chatTemp,0,41);
    memset(inputs[1],0,size[1]);
    memset(inputs[2],0,size[2]);
    free(chatTemp);
}

char * getHexHash(struct client_conn * user,unsigned char * hash){
    char * hexHash = malloc(41);
    if(hexHash==NULL) closeConn(user);
    hexHash[40]='\0';

    
    for(int i=0;i<20;i++){
        if((sprintf(hexHash+2*i,"%02X",hash[i]))<0){
        closeConn(user);
        }
    }

return hexHash;
}

void closeConn(struct client_conn * client){
    close(client->socket);
    perror("Closing connection");
    exit(-1);
}

void createChatIfNotExists(struct client_conn * client){
    char * path;
    if((path = malloc(47))==NULL){
        closeConn(client);
    }
    
    strcpy(path,shatsPath);
    strcpy(path+6,client->shat);
    fprintf(stderr,"Trying to create %s\n",path);
    if (access(path, F_OK) != 0) {  //File exists
        //TODO give more permissions
        if(creat(path,0777)==-1){
            closeConn(client);
        }
    } else {
        fprintf(stderr,"File already exists\n");
    }

}

