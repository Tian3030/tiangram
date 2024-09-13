
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
#include <regex.h>
#include "../lib/server_utilities.h"
#include <semaphore.h>
#include <sys/mman.h>
#include <openssl/sha.h>
#include <openssl/rand.h>
/* Feel free to use this exFample code in any way
   you see fit (Public Domain) */
#include <sys/types.h>
#ifndef _WIN32
#include <sys/select.h>
#include <sys/socket.h>
#else
#include <winsock2.h>
#endif
#include <string.h>
#include <string>
#include <microhttpd.h>
#include <stdio.h>
#include <iostream>

#define PORT 8888
#define GET 0
#define POST 1
#define BUFFER_MAX 256
const int COOKIESIZE = 15;

const char *regexCSS = "^/web/[[:lower:]]+\\.css$";
const char *regexChat = "^/chat/[A-F0-9]{40}$";
const char *regexShats = "^/shat/[A-F0-9]{40}$";

const char *regexAlias = "^[_a-zA-ZáéíóúüñÑ0-9]{1,40}$";        
const char *regexSessionName = "^[_a-zA-ZáéíóúüñÑ0-9]{1,256}$"; 
const char *regexSessionPwd = "^[_a-zA-ZáéíóúüñÑ0-9]{1,256}$";  // ayé falla si hay \\[\\]

const char *regexMessage = "^[a-zA-ZáéíóúüñÑ0-9[:blank:]\\.,!\\?;:\\(\\){}@#\\$\%&\\*\\+\\-\\=~\\|\\^<>]{1,255}$"; // ayé falla si hay \\[\\]
const char *httpOnly = "; HttpOnly";
sem_t *semaphore; // Makes impossible to write-conflict (Not read-write)
void *ptr;

class UserConnection
{
private:
    short connectionType;
    int fd; // File descriptor to close as soon as the request is solved
    int chatOffset;
    struct MHD_PostProcessor *pp;
    std::string alias;
    std::string userPwd;
    std::string user_cookie;
    std::string message; // with a regex we'll regulate a max of 256 chars

    char *sessionName;
    char *sessionPwd;
    char *url;

public:
    UserConnection(short connectionType)
    {
        this->connectionType = connectionType;
        fd = -1;
        chatOffset = -1;
        pp = NULL;
        alias = "";
        userPwd="";
        user_cookie = "";
        message = "";
        sessionName = NULL;
        sessionPwd = NULL;
        url = NULL;
    }
    struct MHD_Response *user_register()
    {

        int size[3]; // Los sizes tienen +1 por el \0
        size[0] = this->getAlias().length();
        size[1] = strlen(this->getSessionName());
        size[2] = strlen(this->getSessionPwd());

        // We add a '0' to the end of the name buffer and a '1' to the ond of the pwd

        int sizeOftoHashBuffer = size[1] + 1 + size[2] + 1 + 1; // Size to store both strings: pwd +'0' and name + '1' +\0
        char *roomCreds;
        if ((roomCreds = (char *)malloc(sizeOftoHashBuffer)) == NULL)
        {
            return NULL;
        }
        roomCreds[sizeOftoHashBuffer - 1] = '\0';
        roomCreds[size[1]] = '0';
        roomCreds[sizeOftoHashBuffer - 2] = '1';

        // Hash calculation
        strncpy(roomCreds, this->getSessionName(), size[1]);              // Adds the name of the room
        strncpy(roomCreds + size[1] + 1, this->getSessionPwd(), size[2]); // Adds the pwd of the room
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
        path = (char *)malloc(46);
        if (path == NULL)
            return NULL;
        path[45] = '\0';

        strcpy(path, SHATS_PATH);
        strncpy(path + 5, chatTemp, 40);
        struct stat *statbuf = (struct stat *)malloc(sizeof(struct stat));
        if (statbuf == NULL)
            return NULL;
        int fd = -1;

        if (access(path, F_OK) != 0)
        { // File DOES NOT exist
            // TODO give less permissions
            if ((fd = creat(path, 0770)) == -1)
            {
                return NULL;
            }
        }

        int templateSize = strlen(redirectTemplate) + 41; // Template + hash+\0
        char *chatRedirect = (char *)malloc(templateSize);
        // TODO When fails we should always free dynamic mem
        if (chatRedirect == NULL)
            return NULL;
        sprintf(chatRedirect, redirectTemplate, chatTemp);
        chatRedirect[templateSize - 1] = '\0';
        struct MHD_Response *response = MHD_create_response_from_buffer(strlen(chatRedirect), chatRedirect, MHD_RESPMEM_PERSISTENT);
        if (response == NULL)return NULL;
            

        memset(chatTemp, 0, strlen(chatTemp));
        free(chatTemp);
        memset(path, 0, 45);
        free(path);
        memset(statbuf, 0, sizeof(struct stat));
        free(statbuf);
        return response;
    }
    // Frees all the object
    void destroy()
    {
        connectionType = -1;
        if (fd != -1)
            close(fd);
        fd = -1;
        chatOffset = -1;
        if (pp != NULL)
            MHD_destroy_post_processor(pp);

        alias.clear();
        userPwd.clear();
        user_cookie.clear();
        message.clear();

        if (sessionName != NULL)
        {
            memset(sessionName, 0, strlen(sessionName));
            free(sessionName);
        }
        if (sessionPwd != NULL)
        {
            memset(sessionPwd, 0, strlen(sessionPwd));
            free(sessionPwd);
        }
        if (url != NULL)
        {
            memset(url, 0, strlen(url));
            free(url);
        }
    }
    enum MHD_Result readChat(struct MHD_Response *&response)
    {
        int fd = -1;
        if ((fd = open(this->url + 1, O_RDONLY)) == -1)
        {
            printf("Could not open %s\n", this->url + 1);
            return MHD_NO;
        }
        this->fd = fd;
        struct stat *statbuf = (struct stat *)malloc(sizeof(struct stat));
        fstat(fd, statbuf);
        response = MHD_create_response_from_fd(statbuf->st_size, fd);
        memset(statbuf, 0, sizeof(struct stat));
        free(statbuf);
        return MHD_YES;
    }
    enum MHD_Result writeChat(struct MHD_Response *&response, sem_t *&semaphore)
    {
        message += '\n';

        if (message == "")
            return MHD_NO;

        int fd = -1;
        if ((fd = open(url + 1, O_WRONLY | O_APPEND)) == -1)
        {
            printf("Could not open %s\n", url + 1);
            return MHD_NO;
        }
        sem_t *semaphoreTemp = (sem_t *)semaphore;
        sem_wait(semaphoreTemp); // So we don't have trouble writing eventhough we lose that multithreading advantage
        int size = message.length();
        if (write(fd, message.c_str(), size) == -1)
        {
            close(fd);
            sem_post(semaphoreTemp);
            return MHD_NO;
        }
        sem_post(semaphoreTemp);
        if (fd == -1)
            return MHD_NO;
        this->fd = fd;
        response = MHD_create_response_from_buffer(strlen(POST_SUCCESS), (char *)POST_SUCCESS, MHD_RESPMEM_PERSISTENT);
        return MHD_YES;
    }


    // GETTERS
    short getConnType() { return connectionType; }
    int getFd() { return fd; }
    int getChatOffset() { return chatOffset; }
    struct MHD_PostProcessor *getPP() { return pp; }
    char *getSessionName() { return sessionName; }
    char *getSessionPwd() { return sessionPwd; }
    char *getUrl() { return url; }

    std::string getAlias() { return alias; }
    std::string getUserPwd() { return userPwd; }
    std::string getCookie() { return user_cookie; }
    std::string getMessage() { return message; }

    // SETTERS
    void getConnType(short connType) { this->connectionType = connType; }
    void setFd(int fd) { this->fd = fd; }
    void setChatOffset(int chatOffset) { this->chatOffset = chatOffset; }
    void setPP(struct MHD_PostProcessor *pp) { this->pp = pp; }
    void setSessionNameCh(const char *sessionName) { this->sessionName = strdup(sessionName); }
    void setSessionPwdCh(const char *sessionPwd) { this->sessionPwd = strdup(sessionPwd); }
    void setAliasCh(const char *alias) { this->alias = alias; }
    void setUserPwdCh(const char *userPwd) { this->userPwd = userPwd; }
    void setUrlCh(const char *url) { this->url = strdup(url); }

    // Doesn't duplicate the char string
    void setCookieCh(char *user_cookie) { this->user_cookie = user_cookie; }
    void setMessageCh(const char *message) { this->message = message; }
};

static enum MHD_Result
iterate_post(void *coninfo_cls, enum MHD_ValueKind kind, const char *key,
             const char *filename, const char *content_type,
             const char *transfer_encoding, const char *data, uint64_t off,
             size_t size)
{
    (void)kind;              /* Unused. Silent compiler warning. */
    (void)filename;          /* Unused. Silent compiler warning. */
    (void)content_type;      /* Unused. Silent compiler warning. */
    (void)transfer_encoding; /* Unused. Silent compiler warning. */
    (void)off;               /* Unused. Silent compiler warning. */

    std::cout<<"Enters iterate post\n";
    UserConnection *con_info = (UserConnection *)coninfo_cls;

    std::cout << con_info->getUrl();

    if (strcmp(con_info->getUrl(), "/login") == 0)
    {
        if (0 == strcmp(key, "alias"))
        {
            // Regex sanitization
            //...
            if (regexValid(data, regexAlias))
            {
                con_info->setAliasCh(data);
            }
        }
        else if (0 == strcmp(key, "pwd1"))
        {
            // Regex sanitization
            //...
            if (regexValid(data, regexSessionPwd))
            {
                con_info->setUserPwdCh(data);
            }
        }
        else if (0 == strcmp(key, "name"))
        {
            // Regex sanitization
            //...
            if (regexValid(data, regexSessionName))
            {
                con_info->setSessionNameCh(data);
            }
        }
        else if (0 == strcmp(key, "pwd2"))
        {
            // Regex sanitization
            //...
            if (regexValid(data, regexSessionPwd))
            {
                con_info->setSessionPwdCh(data);
            }
        }
    }
    else if (regexValid(con_info->getUrl(), regexShats))
    {
        if (0 == strcmp(key, "message"))
        {
            if (!regexValid(data, regexMessage))
                return MHD_NO;
            con_info->setMessageCh(data);
        }
        else
        {
            printf("Message not proccessed\n");
        }
    }
    return MHD_YES;
}

enum MHD_Result sendPage(struct MHD_Connection *connection, struct MHD_Response *response, int code)
{
    if (!response)
        return MHD_NO;
    enum MHD_Result ret;
    ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
    MHD_destroy_response(response);
    return ret;
}

static enum MHD_Result
server_response(void *cls, struct MHD_Connection *connection,
                const char *url, const char *method,
                const char *version, const char *upload_data,
                size_t *upload_data_size, void **con_cls)
{

    struct MHD_Response *response;

    (void)cls;              /* Unused. Silent compiler warning. */
    (void)url;              /* Unused. Silent compiler warning. */
    (void)method;           /* Unused. Silent compiler warning. */
    (void)version;          /* Unused. Silent compiler warning. */
    (void)upload_data;      /* Unused. Silent compiler warning. */
    (void)upload_data_size; /* Unused. Silent compiler warning. */
    (void)con_cls;          /* Unused. Silent compiler warning. */
    printf("Request content: url: %s, method: %s version: %s, upload_data_size:%li\n", url, method, version, *upload_data_size);

    UserConnection *con_info;

    if (*con_cls == NULL)
    {
        if (strcmp(method, MHD_HTTP_METHOD_POST) == 0)
        {
            con_info = new UserConnection(POST);
            con_info->setUrlCh(url);
            ptr = MHD_create_post_processor(connection, 640, &iterate_post, (void *)con_info);
            if (ptr == NULL)
                return MHD_NO;
            con_info->setPP((MHD_PostProcessor *)ptr);
        }
        else
        {
            con_info = new UserConnection(GET);
            con_info->setUrlCh(url);
        }
        *con_cls = (void *)con_info;
        return MHD_YES;
    }

    con_info = (UserConnection *)*con_cls;
    int fd = -1;
    if (strcmp(url, "/login") == 0)
    {
        if (strcmp(method, MHD_HTTP_METHOD_GET) == 0)
        { // LOGIN GET
            fd = open("web/index.html", O_RDONLY);
            if (fd == -1)
                return MHD_NO;

            struct stat *statbuf = (struct stat *)malloc(sizeof(struct stat));
            if (statbuf == NULL)
                return MHD_NO;
            fstat(fd, statbuf);
            response = MHD_create_response_from_fd(statbuf->st_size, fd);
            memset(statbuf, 0, sizeof(struct stat));
            free(statbuf);
        }
        else if (strcmp(method, MHD_HTTP_METHOD_POST) == 0)
        { // LOGIN REGISTRER   (Sesion que se borra)

            if (*upload_data_size != 0)
            {
                if (MHD_post_process(con_info->getPP(), upload_data, *upload_data_size) == MHD_NO)
                    return MHD_NO;
                *upload_data_size = 0;
                return MHD_YES;
            }
            else if (con_info->getAlias().compare("") != 0 && con_info->getUserPwd().compare("") != 0 && con_info->getSessionName() != NULL && con_info->getSessionPwd() != NULL)
            {
                response = con_info->user_register();
                unsigned char *token = generateToken(COOKIESIZE);
                token = (unsigned char *)realloc(token, COOKIESIZE * 2 + strlen(httpOnly) + 1);
                if (token == NULL)
                    return MHD_NO;
                strcpy((char *)token + COOKIESIZE * 2, httpOnly);
                printf("The token is %s\n", token);

                
                // Sets the cookie !MUST BE INSIDE USER_REGISTER!
                /*if (MHD_add_response_header(response, "Set-Cookie", (char *)token) == MHD_NO)
                {
                    printf("Could not load the token %s\n", token);
                    memset(token, 0, strlen((const char *)token));
                    free(token);
                    return MHD_NO;
                }*/
                memset(token, 0, strlen((const char *)token));
                free(token);
            }
            else
            { // Unvalid params
                response = MHD_create_response_from_buffer(strlen(errorpage), (char *)errorpage, MHD_RESPMEM_PERSISTENT);
                return sendPage(connection, response, MHD_HTTP_BAD_REQUEST);
            }
        }
        else
        {
            response = MHD_create_response_from_buffer(strlen(errorpage), (char *)errorpage, MHD_RESPMEM_PERSISTENT);
            return sendPage(connection, response, MHD_HTTP_BAD_REQUEST);
        }
    }
    else if (strncmp(url, "/shat/", 6) == 0) // Its a shat
    {                                        // URI sanitization
        // Depurado de uri correcta y permisos correctos
        //   ...
        //
        if (!regexValid(url, regexShats))
        {
            response = MHD_create_response_from_buffer(strlen(errorpage), (char *)errorpage, MHD_RESPMEM_PERSISTENT);
            return sendPage(connection, response, MHD_HTTP_BAD_REQUEST);
        }
        // We have to make sure in the POST they will send: the hash, the session_name and the password
        if (strcmp(method, MHD_HTTP_METHOD_GET) == 0)
        { // Leer chat /shat/HASHCODE
            if (con_info->readChat(response) == MHD_NO)
            {
                response = MHD_create_response_from_buffer(strlen(errorpage), (char *)errorpage, MHD_RESPMEM_PERSISTENT);
                return sendPage(connection, response, MHD_HTTP_BAD_REQUEST);
            }
        }
        else if (strcmp(method, MHD_HTTP_METHOD_POST) == 0)
        { 
            if (*upload_data_size != 0)
            {
                MHD_post_process(con_info->getPP(), upload_data, *upload_data_size);
                *upload_data_size = 0;
                return MHD_YES;
            }
            else if (con_info->getMessage().compare("") != 0 && con_info->getMessage().length() < BUFFER_MAX)   //Message correct format
            {
                if (con_info->writeChat(response, semaphore) == MHD_NO)
                {
                    response = MHD_create_response_from_buffer(strlen(errorpage), (char *)errorpage, MHD_RESPMEM_PERSISTENT);
                    return sendPage(connection, response, MHD_HTTP_BAD_REQUEST);
                }
            }
            else
            {
                response = MHD_create_response_from_buffer(strlen(errorpage), (char *)errorpage, MHD_RESPMEM_PERSISTENT);
                return sendPage(connection, response, MHD_HTTP_BAD_REQUEST);
            }
        }
        else
        {
            response = MHD_create_response_from_buffer(strlen(errorpage), (char *)errorpage, MHD_RESPMEM_PERSISTENT);
            return sendPage(connection, response, MHD_HTTP_BAD_REQUEST);
        }
    }
    else if (strncmp(url, "/chat/", 6) == 0)
    {
        if (!regexValid(url, regexChat))
        {
            response = MHD_create_response_from_buffer(strlen(errorpage), (char *)errorpage, MHD_RESPMEM_PERSISTENT);
            return sendPage(connection, response, MHD_HTTP_BAD_REQUEST);
        }
        if (strcmp(method, MHD_HTTP_METHOD_GET) == 0)
        {
            char *path = strdup(url);
            path[1] = 's';
            if (access(path + 1, F_OK) == -1)
            {
                printf("No se puede acceder a%s\n", path + 1);
                return MHD_NO;
            }

            char *chatBuff = (char *)malloc(strlen(chatTemplate) + strlen(path) * 2 + 1);
            if (chatBuff == NULL)
                return MHD_NO;
            sprintf(chatBuff, chatTemplate, path, path);
            response = MHD_create_response_from_buffer(strlen(chatBuff), chatBuff, MHD_RESPMEM_PERSISTENT);
            memset(path, 0, strlen(path));
            free(path);
        }
        else
        {
            response = MHD_create_response_from_buffer(strlen(errorpage), (char *)errorpage, MHD_RESPMEM_PERSISTENT);
            return sendPage(connection, response, MHD_HTTP_BAD_REQUEST);
        }
    }
    else if (strncmp(url, "/web", 4) == 0) // requests .css file
    {
        if (!regexValid(url, regexCSS))
        {
            response = MHD_create_response_from_buffer(strlen(errorpage), (char *)errorpage, MHD_RESPMEM_PERSISTENT);
            return sendPage(connection, response, MHD_HTTP_BAD_REQUEST);
        }

        if ((fd = open(url + 1, O_RDONLY)) == -1)
            return MHD_NO;

        struct stat *statbuf = (struct stat *)malloc(sizeof(struct stat));
        fstat(fd, statbuf);
        response = MHD_create_response_from_fd(statbuf->st_size, fd);
        memset(statbuf, 0, sizeof(struct stat));
        free(statbuf);
    }
    else
    {
        response = MHD_create_response_from_buffer(strlen(errorpage), (char *)errorpage, MHD_RESPMEM_PERSISTENT);
        return sendPage(connection, response, MHD_HTTP_BAD_REQUEST);
    }
    return sendPage(connection, response, MHD_HTTP_OK);
}

static void
request_completed(void *cls, struct MHD_Connection *connection,
                  void **con_cls, enum MHD_RequestTerminationCode toe)
{
    (void)cls;        /* Unused. Silent compiler warning. */
    (void)connection; /* Unused. Silent compiler warning. */
    (void)toe;        /* Unused. Silent compiler warning. */
    UserConnection *con_info = (UserConnection *)*con_cls;
    if (NULL == con_info)  return;

    con_info->destroy();
    free(con_info);
    *con_cls = NULL;
}

int main(void)
{

    semaphore = (sem_t *)mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    struct MHD_Daemon *daemon;
    daemon = MHD_start_daemon(MHD_USE_AUTO | MHD_USE_INTERNAL_POLLING_THREAD,
                              PORT, NULL, NULL,
                              &server_response, NULL, MHD_OPTION_NOTIFY_COMPLETED, request_completed,
                              NULL, MHD_OPTION_END);
    sem_init(semaphore, 1, 1);

    if (NULL == daemon)
        return 1;

    (void)getchar();

    MHD_stop_daemon(daemon);
    sem_destroy(semaphore);
    return 0;
}