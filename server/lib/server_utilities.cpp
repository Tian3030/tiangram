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
#include <sys/stat.h>
#include <semaphore.h>

#include <openssl/sha.h>
#include <openssl/rand.h>
#include "server_utilities.h"
#include <regex.h>
#include <sys/sendfile.h>
#include <fcntl.h>

#include <sys/types.h>
#ifndef _WIN32
#include <sys/select.h>
#include <sys/socket.h>
#else
#include <winsock2.h>
#endif
#include <string.h>
#include <microhttpd.h>
#include <stdio.h>

#define BUFFER_MAX 256

const char POST_SUCCESS[] = "Post Success";
const char SHATS_PATH[] = "shat/";
const char *chatTemplate = "<!DOCTYPE html>\n"
                           "<html lang=\"es\">\n"
                           "<head>\n"
                           "    <meta charset=\"UTF-8\">\n"
                           "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
                           "    <title>Tiangram</title>\n"
                           "    <link rel=\"stylesheet\" href=\"/web/chat.css\" type=\"text/css\"> \n"
                           "</head>\n"
                           "<body>\n"
                           "    <div class=\"chat-container\">\n"
                           "        <div id=\"messages\" class=\"messages\"> <!-- Here we add the fetch function-->\n"
                           "            <script>\n"
                           "                    function obtenerTextoChat() {\n"
                           "                        fetch('%s')\n"
                           "                            .then(response => response.text())\n"
                           "                            .then(texto => {\n"
                           "                                document.getElementById('messages').innerText = texto;\n" // If it was innerHTML instead Text we could face some XSS
                           "                            })\n"
                           "                            .catch(error => {\n"
                           "                                console.error('Error al obtener el texto:', error);\n"
                           "                                document.getElementById('messages').innerText = 'Error al cargar el texto';\n"
                           "                            });\n"
                           "                    }\n"
                           "                    setInterval(obtenerTextoChat, 5000);\n"
                           "                    obtenerTextoChat();\n"
                           "            </script>\n"
                           "        </div>\n"
                           "<div class=\"input-container\">\n"
                           "    <input type=\"text\" id=\"textInput\" placeholder=\"Type your message here\" />\n"
                           "    <button type=\"button\" id=\"sendButton\">Send</button>\n"
                           "    <script>\n"
                           "        document.getElementById(\'sendButton\').addEventListener(\'click\', function() {\n"
                           "            const message = document.getElementById(\'textInput\').value;\n"
                           "             const url = \'%s\';\n"
                           "             const data = new URLSearchParams();\n"
                           "              data.append(\'message\', message);\n"
                           "            fetch(url, {\n"
                           "                method: \'POST\',\n"
                           "                headers: {\n"
                           "                    \'Content-Type\': \'application/x-www-form-urlencoded\'\n"
                           "                },\n"
                           "                body: data.toString()\n"
                           "            })\n"
                           "            .then(response => {\n"
                           "                if (response.ok) {\n"
                           "                    console.log(\'Message sent successfully\');\n"
                           "                } else {\n"
                           "                    console.error(\'Failed to send message\');\n"
                           "                }\n"
                           "            })\n"
                           "            .catch(error => console.error(\'Error:\', error));\n"
                           "            document.getElementById(\'textInput\').value = '';\n"
                           "        });\n"
                           "    </script>\n"
                           "</div>\n"
                           "    </div>\n"
                           "</body>\n"
                           "</html>";

const char *redirectTemplate = "<!DOCTYPE html>\n"
                               "<html lang=\"es\">\n"
                               "<head>\n"
                               "    <meta charset=\"UTF-8\">\n"
                               "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
                               "    <title>Redirecting......</title>\n"
                               "    <meta http-equiv=\"refresh\" content=\"5; url=http://localhost:8888/chat/%s\">\n"
                               "</head>\n"
                               "<body>\n"
                               "    <p> Redirecting to a private chat</p>\n"
                               "</body>\n"
                               "</html>";
const char *errorpage =
  "<html><body>This doesn't seem to be right.</body></html>";


/*
// Registers the user and sets the conn->fd used to the connection response
struct MHD_Response *user_register(struct conn_info *user)
{

    int size[3]; // Los sizes tienen +1 por el \0
    for (int i = 0; i < 3; i++)
    {
        // TODO regex Size and Format sanitization
        //...
        size[i] = strlen(user->str[i]); // Size IS without \0
    }

    // We add a '0' to the end of the name buffer and a '1' to the ond of the pwd

    int sizeOftoHashBuffer = size[1] + 1 + size[2] + 1 + 1; // Size to store both strings: pwd +'0' and name + '1' +\0
    char *roomCreds;
    if ((roomCreds = malloc(sizeOftoHashBuffer)) == NULL)
    {
        return NULL;
    }
    roomCreds[sizeOftoHashBuffer - 1] = '\0';
    roomCreds[size[1]] = '0';
    roomCreds[sizeOftoHashBuffer - 2] = '1';

    // Hash calculation
    strncpy(roomCreds, user->str[1], size[1]);               // Adds the name of the room
    strncpy(roomCreds + size[1] + 1, user->str[2], size[2]); // Adds the pwd of the room
    printf("room Credentials: %s\n :", roomCreds);

    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA1((unsigned char *)roomCreds, sizeOftoHashBuffer, hash);

    char *chatTemp = getHexHash(hash);
    if (chatTemp == NULL)
        return NULL;

    memset(hash, 0, 20);
    memset(roomCreds, 0, sizeOftoHashBuffer);
    free(roomCreds);

    // Creates the file that will have plain text format IF IT DOESNT EXIST
    char *path;
    path = malloc(46);
    if (path == NULL)
        return NULL;
    path[45] = '\0';

    strcpy(path, SHATS_PATH);
    strncpy(path + 5, chatTemp, 40);
    struct stat *statbuf = malloc(sizeof(struct stat));
    if (statbuf == NULL)
        return NULL;
    int fd = -1;
    printf("--------------------------------------------------Accessing %s\n", path);

    if (access(path, F_OK) != 0)
    { // File DOES NOT exist
        // TODO give less permissions
        if ((fd = creat(path, 0770)) == -1)
        {
            return NULL;
        }
    }

    int templateSize = strlen(redirectTemplate) + 41; // Template + hash+\0
    char *chatRedirect = malloc(templateSize);
    // TODO When fails we should always free dynamic mem
    if (chatRedirect == NULL)
        return NULL;
    sprintf(chatRedirect, redirectTemplate, chatTemp);
    chatRedirect[templateSize - 1] = '\0';
    struct MHD_Response *response = MHD_create_response_from_buffer(strlen(chatRedirect), chatRedirect, MHD_RESPMEM_PERSISTENT);
    if (response == NULL)
        return NULL;

    memset(path, 0, 45);
    free(path);
    memset(statbuf, 0, sizeof(struct stat));
    free(statbuf);
    return response;
}
*/
char *getHexHash(unsigned char *hash)
{
    char *hexHash = (char*) malloc(41);
    if (hexHash == NULL)
        return NULL;
    hexHash[40] = '\0';

    for (int i = 0; i < 20; i++)
    {
        if ((sprintf(hexHash + 2 * i, "%02X", hash[i])) < 0)
        {
            return NULL;
        }
    }

    return hexHash;
}
/*
void closeConn(struct conn_info *user, int code)
{
    printf("ExitCode: %d\n", code);
    exit(-1);
}
*/
// If regex is correct, returns 1, if it is not, then returns 0
short regexValid(const char *url, const char *regexStr)
{

    regex_t regex;
    // Initialize regexes
    if (regcomp(&regex, regexStr, REG_EXTENDED) != 0)
    {
        printf("%s NO pasa el regex %s\n", url, regexStr);
        return 0;
    }

    // Si no cumple el regex de shats abandona
    if (regexec(&regex, url, 0, NULL, 0) != 0)
    {
        regfree(&regex);
        printf("%s NO pasa el regex %s\n", url, regexStr);
        return 0;
    }
    regfree(&regex);
    printf("%s pasa el regex %s\n", url, regexStr);
    return 1;
}
/*
int readChat(struct conn_info *client) // uri: /shat/AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
{
    int fd = -1;
    if ((fd = open(client->url + 1, O_RDONLY)) == -1)
    {
        printf("Could not open %s\n", client->url + 1);
        return -1;
    }
    return fd;
}

int writeChat(struct conn_info *client, void *semaphore)
{
    int length=strlen(client->message);
    client->message=realloc(client->message,length+2);
    if(client->message==NULL) return -1;
    client->message[length]='\n';
    client->message[length+1]='\0';

    int fd = -1;
    if ((fd = open(client->url + 1, O_WRONLY | O_APPEND)) == -1)
    {
        printf("Could not open %s\n", client->url + 1);
        return -1;
    }
    sem_t *semaphoreTemp = (sem_t *)semaphore;
    sem_wait(semaphoreTemp); // So we don't have trouble writing eventhough we lose that multithreading advantage
    int size = strlen(client->message);
    if (write(fd, client->message, size) == -1)
    {
        close(fd);
        sem_post(semaphoreTemp);
        return -1;
    }
    sem_post(semaphoreTemp);
    return fd;
}
*/
//token must be not initialized
unsigned char * generateToken(int len){
    unsigned char * token = (unsigned char*) malloc(len);
    if(token==NULL)return NULL;

    if(RAND_bytes(token,len) != 1) {
        free(token);
        return NULL;
    }

    unsigned char * hexRand = (unsigned char*) malloc(2*len+1);
    if (hexRand == NULL)return NULL;
    
    hexRand[2*len] = '\0';

    for (int i = 0; i < len; i++)
    {
        if ((sprintf((char *)hexRand + 2 * i, "%02X", token[i])) < 0)
        {
            return NULL;
        }
    }
    memset(token,0,len);
    free(token);
    return hexRand;
}

