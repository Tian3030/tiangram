#include <sys/socket.h>

typedef struct message{
    char message[255];
    short size;
    char * alias;
};


void main(void){


}

/*  TODO
    Codes:
        0)  Log in to a chat with name & password of a session
        1)  Send message into a chat (with alias as nickname)
        2)  Read messages from a chat

*/
void serverPetition(int code,struct message * mensaje){


}
/* TODO
        Create the chat if not exists. This will be done hashing the 
        concat result of mixing the name & password, so it will create the 
        respective file of the chat. 

*/
void createChatIfNotExists(){

}