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
#include <openssl/sha.h>
#include <sys/sendfile.h>

#include "../lib/srv_util.h" 
#define BUFFER_MAX 255


typedef struct client_conn {
    char * message; 
    short size; //Tam del mensaje
    short alias_size;
    char shat[41];
    off_t shatOffset;
    int shat_Socket;
    int socket;
} client_conn;

char * getHexHash(struct client_conn * user,unsigned char * hash);
void * serverService(void * client);
void closeConn(struct client_conn * client,int code);
void user_register(struct client_conn * user);
void readWriteOpps(struct client_conn * client,short read_write);
void readChat(struct client_conn * client);
void writeChat(struct client_conn * client);
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
        closeConn(user,82);
    }
    user_register(user);
    while(1){
            if (recv(user->socket, &integer, sizeof(int), MSG_WAITALL)!=sizeof(int))
            break;

            integer=ntohl(integer);
            printf("Instruction %d received\n",integer);

            switch(integer){
                case 0:     //Connect to a shat
                    fprintf(stderr,"Entered %d\n",integer);
                    break;
                case 1:     //Write to a shat

                fprintf(stderr,"Entered %d\n",integer);
                if (recv(user->socket, &integer, sizeof(int), MSG_WAITALL)!=sizeof(int)){   //Gets the size of 
                finish=1;    
                }
                integer=ntohl(integer);

                if(integer>0 && integer < BUFFER_MAX){  //Prohibiremos el length 0 al enviar un archivo
                    user->size=integer;
                    if (recv(user->socket, user->message+user->alias_size, user->size, MSG_WAITALL)!=user->size){  //Se le pasara al servidor el strlen
                        finish=1;    
                    }
                } else finish=1;
                
                readWriteOpps(user,1);
                break;

                case 2:     //Read from a shat
                    fprintf(stderr,"Entered %d\n",integer);
                    readWriteOpps(user,0);
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
        closeConn(user,146);
    }

    char * inputs[3];
    for(int i=0;i<3;i++){
        size[i]=ntohl(size[i]);
        if(size[i]>BUFFER_MAX || size[i]<0){
            closeConn(user,153);
        }

        inputs[i]=malloc(size[i]+1);    //Ponemos el \0 para el print
        if(inputs[i]==NULL){
            closeConn(user,159);
        }
        inputs[i][size[i]]='\0'; //Forzamos que tenga el tam especificado

        iov[i].iov_len=size[i];
        iov[i].iov_base=inputs[i];
    }
    if(readv(user->socket, iov, 3)<0){
        closeConn(user,165);
    }
    //True size comprobation 
    for(int i=0;i<3;i++){
        if(size[i]!=strlen(inputs[i])) closeConn(user,173);
    }

    //Alias set
    user->alias_size=size[0]+1;//+:(1) Starting from int[user->alias] point we can strcpy the actual message (Max 255 bytes)
    
    //Shat offset set
    user->shatOffset=0;

    //BORRAR INPUTS 1 Y 2 NO EL 0
    if((user->message=calloc(1,strlen(inputs[0])+1+BUFFER_MAX*sizeof(char) +1))==NULL){ //Max message length (alias_strlen + : + BUFFER_MAX_LENGTH + 1)
        closeConn(user,172);
    }   

    //Pegamos el alias en la primera parte del buffer.
    strcpy(user->message,inputs[0]);
    user->message[size[0]]=':';
    user->message[size[0]+1]='\0';
    printf("Message: %s\n",user->message);

    //Hash calculation
    unsigned char hash[SHA_DIGEST_LENGTH];
    if((inputs[1]=realloc(inputs[1],size[1]+size[2]+1))==NULL){
        closeConn(user,172);
    }
    inputs[1][size[1]+size[2]]='\0';
    strcpy((inputs[1]+size[1]),inputs[2]);
    SHA1((unsigned char *)inputs[1],size[1]+size[2], hash);

    char * chatTemp=getHexHash(user,hash);
    strcpy(user->shat,chatTemp);
    fprintf(stderr,"CHATCODE: %s\n",user->shat);

    memset(chatTemp,0,41);
    memset(inputs[1],0,size[1]);
    memset(inputs[2],0,size[2]);
    free(chatTemp);
}

char * getHexHash(struct client_conn * user,unsigned char * hash){
    char * hexHash = malloc(41);
    if(hexHash==NULL) closeConn(user,190);
    hexHash[40]='\0';

    
    for(int i=0;i<20;i++){
        if((sprintf(hexHash+2*i,"%02X",hash[i]))<0){
        closeConn(user,196);
        }
    }

return hexHash;
}

void closeConn(struct client_conn * client,int line){
    close(client->socket);
    close(client->shat_Socket);
    memset(client,0,sizeof(struct client_conn));
    free(client);
    fprintf(stderr,"Closing connection\tError code #%d\n",line);
    exit(-1);
}

void readWriteOpps(struct client_conn * client,short read_write){
    char * path;
    if((path = malloc(47))==NULL){
        closeConn(client,215);
    }
    
    strcpy(path,shatsPath);
    strcpy(path+6,client->shat);
    if (access(path, F_OK) != 0) {  //File exists
        //TODO give more permissions
        if((client->shat_Socket=creat(path,0777))==-1){
            closeConn(client,224);
        }
    } else {
        if((client->shat_Socket=open(path,O_RDWR))==-1){
            closeConn(client,228);
        }
    }

    if(read_write==0){
        readChat(client);
    } else if(read_write==1) {
        writeChat(client);
    }

    memset(path,0,47);
    free(path);
    close(client->shat_Socket);
}

void readChat(struct client_conn * client){
    int sentBytes;
    if((sentBytes=lseek(client->shat_Socket,0,SEEK_END))==-1){    //Calculate the file size
        closeConn(client,247);
    }
    if(lseek(client->shat_Socket,client->shatOffset,SEEK_SET)==-1){
        closeConn(client,250);
    }

    printf("File size:%d\n",sentBytes);


        sentBytes=sentBytes - client->shatOffset;
        printf("%d bytes are about to be sent\n",sentBytes);

        sentBytes=htonl(sentBytes);
        send(client->socket,&sentBytes,sizeof(int),0);
        sentBytes=ntohl(sentBytes);

    if((sentBytes!=0)){
        int bytes_read=-1;
        if((bytes_read=sendfile(client->socket,client->shat_Socket,NULL,sentBytes))==-1){
            closeConn(client,262);
        }

        client->shatOffset+=bytes_read; //Update read pointer
        printf("New offset: %li\n",client->shatOffset);
    }

}

void writeChat(struct client_conn * client){
    if(lseek(client->shat_Socket,0,SEEK_END)==-1){
        closeConn(client,271);
    }

    
    if(write(client->shat_Socket,client->message,strlen(client->message))==-1){
        closeConn(client,275);
    }
    memset(client->message+client->alias_size,0,BUFFER_MAX);
}
