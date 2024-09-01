
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

void sendInfo();
int clnSock;
char buffer[256];
int integer;
//El server siempre usara el puerto 80(x ahora)
const char * MSG[3]={"INTRODUCE ALIAS\n","INTRODUCE SESSION NAME\n","INTRODUCE SESSION PASSWORD\n"};

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

sendInfo();

while(1) {
    //TODO input sanitizatiton
    //fgets(buffer,11,stdin);
    //integer=atoi(buffer);
    scanf("%10d",&integer); //Trunca para que no puedan overflowear el scan
    printf("Enviando %d\n",integer);
    integer=htonl(integer);
    send(clnSock,&integer,sizeof(int),0);

    //write(1,buffer,sizeof(buffer));
}

memset(buffer,0,256);
close(clnSock);
return 0;
}

//Primero se envian las longitudes
void sendInfo(){
    char buffer [3][256];
    int size[3];
    struct iovec iov[6];
    memset(iov,0,6*sizeof(struct iovec));

    for(int i=0;i<3;i++){
        buffer[i][255]='\0';
        printf("%s",MSG[i]);
        fgets(buffer[i],255,stdin);    //Fgets deja un \n al final 
        size[i]=strlen(buffer[i]);
        buffer[i][size[i]-1]='\0';
        
        printf("El char es %s y el size es: %d\n",buffer[i],size[i]);
        iov[i+3].iov_len=size[i];
        iov[i+3].iov_base=buffer[i];

        size[i]=htonl(size[i]);
        iov[i].iov_len=4;
        iov[i].iov_base=&size[i];
    }

    int bytes;
    if ((bytes=writev(clnSock, iov, 6))<0) {
        perror("Error sending the information to the server"); 
        exit(-1);
    }

    /*char buffer1[256];
    char buffer2[256];
    char buffer3[256];
    buffer1[255]='\0';
    buffer2[255]='\0';
    buffer3[255]='\0';
    int size1,size2,size3;
    struct iovec iov[6];

    printf("%s\n",MSG_INTRODUCE_ALIAS);
    fgets(buffer1,255,stdin);    
    size1=strlen(buffer1);
    buffer1[size1-1]='\0';
    size1=htonl(size1);

    iov[0].iov_len=4;
    iov[0].iov_base=&size1;

    iov[3].iov_len=strlen(buffer1);
    iov[3].iov_base=strdup(buffer1);
    //printf("%s\n",buffer);

    printf("%s\n",MSG_INTRODUCE_SESSION_NAME);
    fgets(buffer2,255,stdin);    
    size2=strlen(buffer2);
    buffer2[size2-1]='\0';
    size2=htonl(size2);

    iov[1].iov_len=4;
    iov[1].iov_base=&size2;

    iov[4].iov_len=strlen(buffer2)+1;
    iov[4].iov_base=strdup(buffer2);
    //printf("%s\n",buffer2);

    printf("%s\n",MSG_INTRODUCE_SESSION_NAME);
    fgets(buffer3,255,stdin);    
    size3=strlen(buffer3);
    buffer3[size3-1]='\0';
    size3=htonl(size3);

    iov[2].iov_len=4;
    iov[2].iov_base=&size3;

    iov[5].iov_len=strlen(buffer3)+1;
    iov[5].iov_base=strdup(buffer3);
    //printf("%s\n",buffer3);

    if (writev(clnSock, iov, 6)<0) {
        perror("Error al enviar el size al cliente"); 
    }
    printf("Enviado todo tams: %d, %d, %d\n",ntohl(size1),ntohl(size2),ntohl(size3));
*/

}