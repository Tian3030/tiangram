struct conn_info
{
    struct MHD_PostProcessor *pp;
    short connectiontype;
    char * str [3]; // 0 alias, 1 name, 2 pwd
    int fd;     //File descriptor to close as soon as the request is solved
    
    const char * url;
    char * message;//with a regex we'll regulate a max of 256 chars
};

char *getHexHash(struct conn_info *user, unsigned char *hash);
struct MHD_Response * user_register(struct conn_info *user);
void closeConn(struct conn_info *user,int code);
short regexValid(const char * url,const char * regexStr);
int readChat(struct conn_info *client);
int writeChat(struct conn_info *client,void * semaphore);
unsigned char * generateToken(int len);

extern const char POST_SUCCESS[];
extern const char * chatTemplate;
extern const char *errorpage;
