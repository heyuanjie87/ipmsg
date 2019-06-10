
/*
 * Copyright (c) 2018, Real-Thread Information Technology Ltd
 * All rights reserved
 *
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2019-04-24     heyuanjie87  the first version
 */

#include <sys/socket.h>
#include "netdb.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>

#include "ipmsg.h"
#include "ipmsgdef.h"

#define DBG_TAG "ipmsg"
#define DBG_LVL DBG_INFO /* DBG_ERROR */
#include <rtdbg.h>

#ifndef IPMSG_MSG_BUFSZ
#define IPMSG_MSG_BUFSZ 300
#endif

typedef struct
{
    char *ver;
    char *id;
    char *user;
    char *host;
    uint32_t cmd;
    char *ext;
    char *attach;
    int asz;
} ipmsg_msgfm_t;

static int udp_send(ipmsg_t *im, uint32_t ip, void *buf, int size)
{
    struct sockaddr_in addr;

    memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_port = htons(im->port);
    addr.sin_addr.s_addr = ip;

    // Send string to address to
    if ((sendto(im->skudp, buf, size, 0, (struct sockaddr *)&addr, sizeof(struct sockaddr))) == -1)
    {
        LOG_E("udp send");
        return -1;
    }

    return 0;
}

static int udp_recv(ipmsg_t *im, uint32_t *ip, void *buf, int size)
{
    struct sockaddr_in addr;
    socklen_t al = sizeof(struct sockaddr);
    int ret;

    ret = recvfrom(im->skudp, buf, size, 0, (struct sockaddr *)&addr, &al);
    *ip = addr.sin_addr.s_addr;

    return ret;
}

static char *strtokc(char *str, char d, char **save)
{
    char *p;
    char *ret;

    if (str)
        p = str;
    else
        p = *save;
    ret = p;

    while (*p)
    {
        if (*p == d)
        {
            *p = 0;
            p++;
            *save = p;
            break;
        }
        p++;
    }

    if (ret == p)
        ret = NULL;

    return ret;
}

static int msg_unpack(char *buf, int size, ipmsg_msgfm_t *fm)
{
    char *str = buf;
    char *save = NULL;
    char *p;
    int item = 0;
    int s1len;

    while ((item < 6) && ((p = strtokc(str, ':', &save)) != NULL))
    {
        switch (item)
        {
        case 1:
            fm->id = p;
        case 4:
            fm->cmd = (uint32_t)atoi(p);
            break;
        case 5:
            fm->ext = p;
            break;
        }

        item++;
        str = NULL;
    }

    if (item == 5)
    {
        fm->ext = save;
        item++;
    }

    s1len = save - buf;
    if (size > s1len)
    {
        fm->attach = ++save;
        fm->asz = size - s1len;
    }
    else
    {
        fm->attach = NULL;
        fm->asz = 0;
    }

    return (item == 6);
}

#ifdef IPMSG_FILERECV_ENABLE
static uint32_t htoud(char *str)
{
    uint32_t ret = 0;
    int len;
    char *p;
    int pos;

    len = strlen(str);
    p = str + len;
    if (len > 8)
        len = 8;

    for (pos = 0; pos < len; pos++)
    {
        uint32_t v = 0;

        p--;
        if ((*p >= '0') && (*p <= '9'))
            v = *p - '0';
        else if ((*p >= 'a') && (*p <= 'f'))
            v = *p - 'a' + 10;
        else if ((*p >= 'A') && (*p <= 'F'))
            v = *p - 'A' + 10;

        v = (v << (pos * 4));
        ret |= v;
    }

    return ret;
}

static int _fileinfo_get(ipmsg_fileinfo_t *fi, char *src, int *pos)
{
    char *str;
    char *save = NULL;
    char *p;
    int item = 0;

    str = &src[*pos];
    while ((item < 5) && ((p = strtokc(str, ':', &save)) != NULL))
    {
        switch (item)
        {
        case 0:
            fi->id = (uint32_t)atoi(p);
        case 1:
            fi->name = p;
            break;
        case 2:
            fi->size = htoud(p);
            break;
        case 3:
            break;
        case 4:
            fi->attr = htoud(p);
            break;
        }

        item++;
        str = NULL;
    }

    return (item >= 5);
}

static ipmsg_filehandler_t *fileinfo_unpack(char *str, int size)
{
    ipmsg_fileinfo_t fi = {0};
    int pos = 0;
    ipmsg_filehandler_t *fh = NULL;

    LOG_D("[file] %s \n[%d]", str, size);

    if ((fh = rt_calloc(1, sizeof(*fh))) == NULL)
        return NULL;

    if (_fileinfo_get(&fi, str, &pos))
    {
        ipmsg_fileinfo_t *p;
        /* only surpport FILE */
        if (fi.attr != IPMSG_FILE_REGULAR)
            goto _out;

        p = rt_malloc(sizeof(*p) + strlen(fi.name) + 1);
        if (p)
        {
            *p = fi;
            p->name = (char *)((char *)p + sizeof(*p));
            strcpy(p->name, fi.name);

            fh->fi = p;
        }
    }

_out:
    if (fh->fi == NULL)
    {
        ipmsg_filehandler_free(fh);
        fh = NULL;
    }

    return fh;
}
#endif

static char *make_msg(ipmsg_t *im, uint32_t cmd, char *ext, char *group, int *len)
{
    *len = IPMSG_MSG_BUFSZ;
    return ipmsg_msg_make(im, cmd, ext, group, im->msgbuf, len);
}

static int msg_send(ipmsg_t *im, uint32_t ip, uint32_t cmd, char *ext, char *group)
{
    char *str;
    int size;

    str = make_msg(im, cmd, ext, group, &size);
    if (str == NULL)
        return 0;

    return udp_send(im, ip, str, size);
}

int ipmsg_sock_wait(int socket, long ms)
{
    struct timeval timeout;
    fd_set fds;

    timeout.tv_sec = 0;
    timeout.tv_usec = ms * 1000;

    FD_ZERO(&fds);
    FD_SET(socket, &fds);

    return select(socket + 1, &fds, 0, 0, &timeout);
}

char *ipmsg_msg_make(ipmsg_t *im, uint32_t cmd, char *ext, char *group, char *dst, int *len_io)
{
    uint32_t id;
    int n, l = 0;
    char *p;
    int bufsz = *len_io;

    p = dst;
    id = rt_tick_get();
    n = snprintf(p, *len_io, "1:%d:%s:%s:%d:",
                 id, im->user, im->host, cmd);

    if (ext)
    {
        l = strlen(ext);
        if ((n + l) > bufsz)
            return NULL;
        p += n;
        strcat(p, ext);
        p += l;
        *p = 0;
        p++;
        l += 1;

        *len_io = n + l;
    }

    if (group)
    {
        l += strlen(group);
        if ((n + l) <= bufsz)
        {
            *p = 0;
            strcat(p, group);
            l += 1;

            *len_io = n + l;
        }
    }

    return dst;
}

int ipmsg_msgserver_init(ipmsg_t *im, short port)
{
    struct sockaddr_in server_addr;
    int broadcast = 1;

    strcpy(im->user, "user");
    strcpy(im->host, "rtthread");
    strcpy(im->group, "iot");
    im->port = port;

    if ((im->skudp = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
    {
        LOG_E("Create socket error");
        return -1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    memset(&(server_addr.sin_zero), 0, sizeof(server_addr.sin_zero));

    if (bind(im->skudp, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1)
    {
        LOG_E("Unable to bind");
        goto _exit;
    }

    // Enable broadcast option
    if ((setsockopt(im->skudp, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast))) == -1)
    {
        LOG_E("setsockopt(udp, SO_BROADCAST)");
        goto _exit;
    }

    if ((im->msgbuf = rt_malloc(IPMSG_MSG_BUFSZ * 2 + 2)) == 0)
    {
        LOG_E("no mem");
        goto _exit;
    }
    im->mrbuf = im->msgbuf + IPMSG_MSG_BUFSZ + 1;

    return 0;

_exit:
    closesocket(im->skudp);

    return -1;
}

void ipmsg_msgserver_deinit(ipmsg_t *im)
{
    closesocket(im->skudp);
    rt_free(im->msgbuf);
}

int ipmsg_login(ipmsg_t *im)
{
    int ret;

    ret = msg_send(im, 0xFFFFFFFF, IPMSG_BR_ENTRY, im->user, im->group);

    return ret;
}

int ipmsg_logout(ipmsg_t *im)
{
    int ret;

    ret = msg_send(im, 0xFFFFFFFF, IPMSG_BR_EXIT, im->user, im->group);

    return ret;
}

int ipmsg_msg_recv(ipmsg_t *im, int ms, const ipmsg_msghandler_t *h)
{
    int ret;

    ret = ipmsg_sock_wait(im->skudp, ms);
    if (ret > 0)
    {
        uint32_t ip;
        ipmsg_msgfm_t fm;

        ret = udp_recv(im, &ip, im->mrbuf, IPMSG_MSG_BUFSZ + 1);
        if (ret <= 0)
            return -1;
        if (ret == (IPMSG_MSG_BUFSZ + 1))
            return ret;

        im->mrbuf[ret] = 0;
        if (!msg_unpack(im->mrbuf, ret, &fm))
        {
            LOG_W("frame err");
            return ret;
        }

        switch (fm.cmd & 0xff)
        {
        case IPMSG_BR_ENTRY:
        {
            msg_send(im, ip, IPMSG_ANSENTRY, im->user, im->group);
        }
        break;
        case IPMSG_ANSENTRY:
        {
        }
        break;
        case IPMSG_SENDMSG:
        {
            msg_send(im, ip, IPMSG_RECVMSG, fm.id, im->group);

            if (fm.cmd & IPMSG_FILEATTACHOPT)
            {
#ifdef IPMSG_FILERECV_ENABLE
                if (h->fileattach)
                {
                    ipmsg_filehandler_t *fh;

                    if ((fh = fileinfo_unpack(fm.attach, fm.asz)) == NULL)
                    {
                        LOG_E("fileinfo");
                        break;
                    }

                    fh->ip = ip;
                    fh->im = im;
                    fh->packetid = (uint32_t)atoi(fm.id);
                    ret = h->fileattach(im, fh);
                    if ((ret == 0) && fh->notify && fh->data)
                        ret = ipmsg_filerecv_start(fh);
                    else
                        ret = 1;

                    if (ret != 0)
                        ipmsg_filehandler_free(fh);
                }
#endif
            }
            else
            {
                if (h->msg)
                    h->msg(im, ip, fm.ext);
            }
        }
        break;
        case IPMSG_RECVMSG:
        {
        }
        break;
        case IPMSG_BR_EXIT:
        {
        }
        break;
        case IPMSG_READMSG:
        {
        }
        break;
        default:
        {
            LOG_W("msgrecv cmd 0x%X", fm.cmd);
        }
        break;
        }
    }

    return ret;
}

void ipmsg_filehandler_free(ipmsg_filehandler_t *h)
{
    rt_free(h->fi);
    rt_free(h);
}

int ipmsg_msg_send(ipmsg_t *im, uint32_t ip, const char *str)
{
    return msg_send(im, ip, IPMSG_SENDMSG, (char *)str, NULL);
}

int ipmsg_user_set(ipmsg_t *im, const char *name)
{
    if (name == NULL)
        return -1;
    strncpy(im->user, name, 24);

    return 0;
}
