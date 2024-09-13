/*
struct conn_info
{
    struct MHD_PostProcessor *pp;
    UserConnection * user;
};
*/

char *getHexHash(unsigned char *hash);
//struct MHD_Response * user_register(struct conn_info *user);
//void closeConn(struct conn_info *user,int code);
short regexValid(const char * url,const char * regexStr);
//int readChat(struct conn_info *client);
//int writeChat(struct conn_info *client,void * semaphore);
unsigned char * generateToken(int len);

extern const char POST_SUCCESS[];
extern const char * chatTemplate;
extern const char *redirectTemplate;
extern const char *errorpage;
extern const char SHATS_PATH[];
