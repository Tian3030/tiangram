
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
#define MAX_BUFFER 255

void sendInfo();
int clnSock;
char buffer[256];
int integer;

//El server siempre usara el puerto 80(x ahora)
const char * MSG[]={"INTRODUCE ALIAS","INTRODUCE SESSION NAME","INTRODUCE SESSION PASSWORD"};
char byte;

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

int number1,number2,number3;
int iovecs=-1;
struct iovec * iov;

while(1) {
    scanf("%10d",&integer); //Trunca para que no puedan overflowear el scan
    while ((byte = getchar()) != '\n'); //Retires the rest of buffer chars

    switch(integer){
        case 0:
            break;
        case 1: //Write to a shat
            if((iov=malloc(sizeof(struct iovec)*3))==NULL){exit(-1);}
            iovecs=3;
            fgets(buffer,255,stdin);
            number1=strlen(buffer);
            buffer[number1]='\0';
            iov[2].iov_len=number1;
            iov[2].iov_base=buffer;

            number1=htonl(number1);
            iov[1].iov_len=sizeof(int);
            iov[1].iov_base=&number1;

            number2=htonl(1);
            iov[0].iov_len=sizeof(int);
            iov[0].iov_base=&number2;
            break;
        case 2: //Read to a shat
            break;
        
    }

    int bytes;
    if ((bytes=writev(clnSock, iov, iovecs))<0) {
        perror("Error sending the information to the server"); 
        exit(-1);
    }

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

    while(1){
        for(int i=0;i<3;i++){
            printf("%s\n",MSG[i]);
            fgets(buffer[i],MAX_BUFFER,stdin);    //Fgets deja un \n al final 

            size[i]=strlen(buffer[i])-1;   //Se le quita el \n por \0 pero no influye en los datos que se envian
            buffer[i][size[i]]='\0';    //Lo ponemos para el print
            
            //printf("El char es %s y el size es: %d\n",buffer[i],size[i]);
            iov[i+3].iov_len=size[i];
            iov[i+3].iov_base=buffer[i];

            size[i]=htonl(size[i]);
            iov[i].iov_len=4;
            iov[i].iov_base=&size[i];   //Se enva el strlen, es decir el tamaño sin el \0
        }
        printf("ALIAS:\t%s\nSESSION_NAME:\t%s\nSESSION_PASSWORD:\t%s\nIs this information correct[y/other]?\n",buffer[0],buffer[1],buffer[2]);
        char ch;
        scanf("%1c",&ch);
        while (getchar() != '\n');
        if(ch=='y' || ch=='Y') break; 
    }

    int bytes;
    if ((bytes=writev(clnSock, iov, 6))<0) {
        perror("Error sending the information to the server"); 
        exit(-1);
    }
    printf("%s,%s,%s has been sent\n",buffer[0],buffer[1],buffer[2]);
    
}