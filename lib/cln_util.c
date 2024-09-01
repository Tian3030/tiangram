#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <netdb.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>

int cln_createSocket(unsigned short ideal_port,unsigned short * assigned_port){
    int s;
    if ((s=socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        perror("[-] Error creating the socket"); 
        return -1;
    }
    return s;
}
int cln_createSocketbyName(char * hostname,unsigned short ideal_port,unsigned short * assigned_port){
    return cln_createSocket(ideal_port,assigned_port);
}
