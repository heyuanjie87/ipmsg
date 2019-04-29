#ifndef _IPMSG_H_
#define _IPMSG_H_

#include <stdint.h>

typedef struct
{
    int skudp;
    short port;
    char *msgbuf;
    char *mrbuf;
    char user[24+1];
    char host[16+1];
    char group[16+1];

    long usrdata;
} ipmsg_t;

typedef struct ipmsg_fileinfo
{
    uint32_t id;
    char *name;
    uint32_t size;
    uint32_t attr;

    uint32_t pos;
    struct ipmsg_fileinfo *next;
}ipmsg_fileinfo_t;

typedef enum
{
    IPMSG_FE_ENTER,
    IPMSG_FE_EXIT,
    IPMSG_FE_OPEN,
    IPMSG_FE_CLOSE,
    IPMSG_FE_COMPLETE,
}ipmsg_fileevent_t;

typedef struct ipmsg_filehandler
{
    ipmsg_t *im;
    ipmsg_fileinfo_t *fi;
    uint32_t ip;
    uint32_t packetid;

    /* used by user */
    long usrdata;
    uint32_t param;

    /* init by user */
    int (*notify)(struct ipmsg_filehandler *h, ipmsg_fileevent_t e, void *arg);
    int (*data)(struct ipmsg_filehandler *h, void *buf, int size);
}ipmsg_filehandler_t;

typedef struct
{
    void (*msg)(ipmsg_t *im, uint32_t ip, const char *str);
    int (*fileattach)(ipmsg_t *im, ipmsg_filehandler_t *fh);
}ipmsg_msghandler_t;

int ipmsg_msgserver_init(ipmsg_t *im, short port); /* default port = 2425 */
void ipmsg_msgserver_deinit(ipmsg_t *im);
int ipmsg_login(ipmsg_t *im);
int ipmsg_logout(ipmsg_t *im);
int ipmsg_msg_recv(ipmsg_t *im, int ms, const ipmsg_msghandler_t *h);
int ipmsg_msg_send(ipmsg_t *im, uint32_t ip, const char *str);
int ipmsg_user_set(ipmsg_t *im, const char *name);
void ipmsg_filehandler_free(ipmsg_filehandler_t *h);

char *ipmsg_msg_make(ipmsg_t *im, uint32_t cmd, char *ext, char *group, char *dst, int *len_io);

int ipmsg_sock_wait(int socket, long ms);

int ipmsg_filerecv_start(ipmsg_filehandler_t *h);

#endif
