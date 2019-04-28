# ipmsg(飞鸽传书) implementation in RT-Thread

## 1. 功能
- 收发文本消息
- 接收文件（单个每次）

## 2. 配置
```
//rtconfig.h

#define IPMSG_MSGRS_ENABLE    //启用ipmsg
#define IPMSG_FILERECV_ENABLE //启用文件接收服务

#define IPMSG_MSG_BUFSZ 300   //消息缓冲区，不定义默认300字节
#define IPMSG_FILERECV_BUFSZ   1024 //文件接收缓冲区，不定义默认1024字节

```

## 3. 示例
- [消息收发服务](examples/msgrs.c)
