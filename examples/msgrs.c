#include <rtthread.h>
#include "../ipmsg.h"
#include <string.h>

#define DBG_TAG "msgrs"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

#if (defined(SOC_W600_A8xx))
#include "wm_fwup.h"
static T_BOOTER _booter;

static int file_notify(struct ipmsg_filehandler *h, ipmsg_fileevent_t e, void *arg)
{
    LOG_I("notify %d", e);

    switch (e)
    {
    case IPMSG_FE_OPEN:
        h->usrdata = 0;
        h->param = 0;
        break;
    case IPMSG_FE_COMPLETE:
        tls_fwup_img_update_header(&_booter);
        break;
    }

    return 0;
}

static int file_data(struct ipmsg_filehandler *h, void *buf, int size)
{
    if (h->usrdata == 0)
    {
        memcpy(&_booter, buf, sizeof(_booter));
        tls_fwup_img_write(h->usrdata, (char*)buf + sizeof(_booter), size - sizeof(_booter));
        h->usrdata += (size - sizeof(_booter));
    }
    else
    {
        tls_fwup_img_write(h->usrdata, buf, size);
        h->usrdata += size;
    }

    return size;
}
#else
static int file_notify(struct ipmsg_filehandler *h, ipmsg_fileevent_t e, void *arg)
{
    LOG_I("notify %d", e);

    return 0;
}

static int file_data(struct ipmsg_filehandler *h, void *buf, int size)
{
    LOG_I("data %d", size);

    return size;
}
#endif

static void text_msg(ipmsg_t *im, uint32_t ip, const char *buf)
{
    if (strncmp(buf, "rename ", 7) == 0)
    {
        ipmsg_user_set(im, buf + 7);

        ipmsg_msg_send(im, ip, "YiJingGaiLe");
        return;
    }

    if (strncmp(buf, "reboot", 7) == 0)
    {
        ipmsg_msg_send(im, ip, "ShaoHouChongQi");

        rt_thread_mdelay(100);
        rt_hw_cpu_reset();
        return;
    }

    ipmsg_msg_send(im, ip, "ShouDao");
}

static int fileattach_msg(ipmsg_t *im, ipmsg_filehandler_t *fh)
{
    int ret = 0;

    fh->data = file_data;
    fh->notify = file_notify;

    return ret;
}

static const ipmsg_msghandler_t _h =
{
    text_msg,
    fileattach_msg,
};

static void msg_server(void *p)
{
    ipmsg_t im;
    rt_tick_t t = 0;

    while (1)
    {
        rt_thread_mdelay(5000);

        if (ipmsg_msgserver_init(&im, 2425) == 0)
            break;
    }

    LOG_D("login");
    ipmsg_login(&im);

    while (1)
    {
        if ((rt_tick_get() - t) > (RT_TICK_PER_SECOND * 60))
        {
            t = rt_tick_get();
            ipmsg_login(&im);
        }

        ipmsg_msg_recv(&im, 100, &_h);
    }

    ipmsg_msgserver_deinit(&im);
}

int ipmsg_msgrs_init(void)
{
    rt_thread_t tid;
    int ret = -1;

    tid = rt_thread_create("ipmsg-m",
                           msg_server,
                           0,
                           2048,
                           22,
                           20);

    if (tid)
    {
        ret = rt_thread_startup(tid);
    }

    return ret;
}
INIT_APP_EXPORT(ipmsg_msgrs_init);
