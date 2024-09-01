#include <sys/socket.h>
#include <netinet/in.h>
//#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>

int create_server_socket(unsigned short ideal_port){
    int servSock;
    if ((servSock=socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        perror("[-] Error creating the socket"); 
        return -1;
    }
    int options=1;
    if (setsockopt(servSock, SOL_SOCKET, SO_REUSEADDR, &options, sizeof(options))<0){
        perror("[-] Error setting the socket's options"); 
        close(servSock); 
        return -1;
    }
    struct sockaddr_in server_addr;

    server_addr.sin_family= PF_INET;
    server_addr.sin_addr.s_addr= INADDR_ANY;
    server_addr.sin_port=htons(ideal_port);

    if(bind(servSock,(struct sockaddr *)&server_addr,sizeof(server_addr))==-1){
        perror("[-] Error binding the port"); close(servSock); return -1;
    }

    if(ntohs(server_addr.sin_port)!=ideal_port){
        fprintf(stderr,"[-] Couldn't bind in the specified port");
        close(servSock); 
        return -1;

    }
    //8 people to queue max
    if(listen(servSock,8)==-1){
        perror("[-] Failed listening"); 
        close(servSock); 
        return -1;
    }
    return servSock;
}
