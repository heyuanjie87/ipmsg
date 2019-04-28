#include <rtthread.h>
#include "ipmsg.h"
#include "ipmsgdef.h"

#include <sys/socket.h>
#include "netdb.h"

#define DBG_TAG "ipmsgfr"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

#ifndef IPMSG_FILERECV_BUFSZ
#define IPMSG_FILERECV_BUFSZ 1024
#endif

#ifdef IPMSG_FILERECV_ENABLE

static int ipmsg_getdata_req(int sk, ipmsg_filehandler_t *fh, ipmsg_fileinfo_t *fi, char *buf, int bufsz)
{
    char *msg;
    char ext[24];
    int len;

    sprintf(ext, "%x:%x:0", fh->packetid, fi->id);
    msg = ipmsg_msg_make(fh->im, IPMSG_GETFILEDATA, ext, NULL, buf, &bufsz);
    len = send(sk, msg, bufsz, 0);

    return (len == bufsz);
}

static int ipmsg_data_recv(int sk, ipmsg_filehandler_t *fh, ipmsg_fileinfo_t *fi, char *buf, int bufsz)
{
    int len;
    int ret = -1;

    if (ipmsg_sock_wait(sk, 2000) <= 0)
        return 0;

    len = recv(sk, buf, bufsz, 0);
    if (len > 0)
    {
        ret = fh->data(fh, buf, len);
        if (ret <= 0)
            ret = 0;
        else if (ret > 0)
            fi->size -= ret;
    }

    if (fi->size == 0)
    {
        fh->notify(fh, IPMSG_FE_COMPLETE, fi);
        ret = 0;
    }

    return (ret == len);
}

static int sock_init(ipmsg_filehandler_t *fh)
{
    struct sockaddr_in server_addr = {0};
    int sock;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        LOG_E("Create socket error");
        return -1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(fh->im->port);
    server_addr.sin_addr.s_addr = fh->ip;

    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1)
    {
        LOG_E("Connect fail!");
        closesocket(sock);
        return -1;
    }

    return sock;
}

static void msg_server(void *p)
{
    int sock;
    char *buf;
    ipmsg_filehandler_t *fh = p;
    int ret;
    int stage = 0;

    fh->notify(fh, IPMSG_FE_ENTER, NULL);

    if ((sock = sock_init(fh)) < 0)
        goto __exit;
    if ((buf = rt_malloc(IPMSG_FILERECV_BUFSZ)) == NULL)
        goto _out;

    while (1)
    {
        switch (stage)
        {
        case 0:
        {
            if (fh->notify(fh, IPMSG_FE_OPEN, fh->fi) != 0)
                goto _out;

            if (ipmsg_getdata_req(sock, fh, fh->fi, buf, IPMSG_FILERECV_BUFSZ))
                stage++;
            else
            {
                //error
                stage = 2;
            }
        }
        break;
        case 1:
        {
            if (!ipmsg_data_recv(sock, fh, fh->fi, buf, IPMSG_FILERECV_BUFSZ))
            {
                stage++;
            }
        }
        break;
        case 2:
        {
            fh->notify(fh, IPMSG_FE_CLOSE, fh->fi);
        }
            goto _out;
        }
    }

_out:
    closesocket(sock);
    rt_free(buf);

__exit:
    fh->notify(fh, IPMSG_FE_EXIT, NULL);
    ipmsg_filehandler_free(fh);
}

int ipmsg_filerecv_start(ipmsg_filehandler_t *h)
{
    rt_thread_t tid;
    int ret = -1;

    tid = rt_thread_create("ipmsg-r",
                           msg_server,
                           h,
                           2048,
                           22,
                           20);

    if (tid)
    {
        ret = rt_thread_startup(tid);
    }

    return ret;
}
#endif
