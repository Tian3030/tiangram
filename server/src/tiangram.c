
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
/* Feel free to use this exFample code in any way
   you see fit (Public Domain) */
// Compile: gcc hellobrowser.c -o hellobrowser `pkg-config --cflags --libs libmicrohttpd`
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

#define PORT 8888

#define BUFFER_MAX 256

#define GET 0
#define POST 1
const char *regexCSS = "^/web/[[:lower:]]+\\.css$";
const char *regexChat = "^/chat/[A-F0-9]{40}$";
const char *regexShats = "^/shat/[A-F0-9]{40}$";
const char *regexMessage = "^[a-zA-ZáéíóúüñÑ0-9[:blank:]\\.,!\\?;:\\(\\){}@#\\$\%&\\*\\+\\-\\=~\\|\\^<>]{1,255}$";

const char *httpOnly = "; HttpOnly";
//
// Hola estuve ayér júgándo por alli! QUE TAL LOS TUYOS??????

// ayé falla si hay \\[\\]
//  <>
sem_t *semaphore; // Makes impossible to write-conflict (But not to the read-write ones)
// MHD_NO might only be used when serious error happen
int tokenSize = 15;

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

    struct conn_info *con_info = coninfo_cls;
    // Data  sanitization
    //   ...
    printf("entered to: %s\n", con_info->url);

    if (strcmp(con_info->url, "/login") == 0)
    {
        if (0 == strcmp(key, "alias"))
        {
            con_info->str[0] = strdup(data);
        }
        else if (0 == strcmp(key, "name"))
        {
            con_info->str[1] = strdup(data);
        }
        else if (0 == strcmp(key, "password"))
        {
            con_info->str[2] = strdup(data);
        }
    }
    else if (regexValid(con_info->url, regexShats))
    {
        if (0 == strcmp(key, "message"))
        {
            if (!regexValid(data, regexMessage))
                return MHD_NO;

            con_info->message = strdup(data);
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
    if (!response) return MHD_NO;
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

    if (*con_cls == NULL)
    {
        // printf("Enters first\n");
        struct conn_info *myClientInfo = malloc(sizeof(struct conn_info));
        if (myClientInfo == NULL)
            return MHD_NO;
        myClientInfo->str[0] = NULL;
        myClientInfo->str[1] = NULL;
        myClientInfo->str[2] = NULL;
        myClientInfo->url = url;
        myClientInfo->message = NULL;

        if (strcmp(method, MHD_HTTP_METHOD_POST) == 0)
        {

            myClientInfo->pp = MHD_create_post_processor(connection, 512, &iterate_post, (void *)myClientInfo);

            if (myClientInfo->pp == NULL)
                return MHD_NO;

            myClientInfo->connectiontype = POST;
        }
        else
        {
            myClientInfo->connectiontype = GET;
        }
        *con_cls = (void *)myClientInfo;
        return MHD_YES;
    }

    struct conn_info *myClientInfo = *con_cls;
    int fd = -1;
    if (strcmp(url, "/login") == 0)
    {
        if (strcmp(method, MHD_HTTP_METHOD_GET) == 0)
        { // LOGIN GET
            fd = open("web/index.html", O_RDONLY);
            if (fd == -1)
                return MHD_NO;

            struct stat *statbuf = malloc(sizeof(struct stat));
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
                MHD_post_process(myClientInfo->pp, upload_data, *upload_data_size);
                *upload_data_size = 0;
                return MHD_YES;
            }
            else if (myClientInfo->str[0] != NULL && myClientInfo->str[1] != NULL && myClientInfo->str[2] != NULL)
            {
                response = user_register(myClientInfo);
                unsigned char *token = generateToken(tokenSize);
                //********* */
                token = realloc(token, tokenSize * 2 + strlen(httpOnly) + 1);
                if (token == NULL)
                    return MHD_NO;
                strcpy((char *)token + tokenSize * 2, httpOnly);
                printf("The token is %s\n", token);

                // Sets the cookie
                if (MHD_add_response_header(response, "Session-pwd", (char *)token) == MHD_NO)
                {
                    printf("Could not load the token %s\n", token);
                    memset(token, 0, strlen((const char *)token));
                    free(token);
                    return MHD_NO;
                }
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
            if ((fd = readChat(myClientInfo)) == -1)
                return MHD_NO;

            myClientInfo->fd = fd;
            struct stat *statbuf = malloc(sizeof(struct stat));
            fstat(fd, statbuf);
            response = MHD_create_response_from_fd(statbuf->st_size, fd);
            memset(statbuf, 0, sizeof(struct stat));
            free(statbuf);
        }
        else if (strcmp(method, MHD_HTTP_METHOD_POST) == 0)
        { // Escribir en chat
            if (*upload_data_size != 0)
            {
                MHD_post_process(myClientInfo->pp, upload_data, *upload_data_size);
                *upload_data_size = 0;
                return MHD_YES;
            }
            else if (myClientInfo->message != NULL && strlen(myClientInfo->message) < BUFFER_MAX)
            {
                if ((fd = writeChat(myClientInfo, semaphore)) == -1)
                    return MHD_NO;

                myClientInfo->fd = fd;
                response = MHD_create_response_from_buffer(strlen(POST_SUCCESS), (char *)POST_SUCCESS, MHD_RESPMEM_PERSISTENT);
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

            char *chatBuff = malloc(strlen(chatTemplate) + strlen(path) * 2 + 1);
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

        myClientInfo->fd = fd;
        struct stat *statbuf = malloc(sizeof(struct stat));
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
    struct conn_info *con_info = *con_cls;
    if (NULL == con_info)
        return;

    if (con_info->connectiontype == POST)
    {
        if (con_info->fd != -1)
            close(con_info->fd);

        MHD_destroy_post_processor(con_info->pp);
        printf("Alias: %s Name: %s Pwd: %s Message: %s \n", con_info->str[0], con_info->str[1], con_info->str[2], con_info->message);
        if (con_info->str[0])
        {
            memset(con_info->str[0], 0, strlen(con_info->str[0]));
            free(con_info->str[0]);
        }
        if (con_info->str[1])
        {
            memset(con_info->str[1], 0, strlen(con_info->str[1]));
            free(con_info->str[1]);
        }
        if (con_info->str[2])
        {
            memset(con_info->str[2], 0, strlen(con_info->str[2]));
            free(con_info->str[2]);
        }
        if (con_info->message != NULL)
        {
            memset(con_info->message, 0, strlen(con_info->message));
            free(con_info->message);
        }
    }

    memset(con_info, 0, sizeof(struct conn_info));

    free(con_info);
    *con_cls = NULL;
}

int main(void)
{

    semaphore = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
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