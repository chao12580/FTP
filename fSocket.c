/************************************************************************
* 版权所有 (C)2012, 福建视通光电网络有限公司。
* 
* 文件名称： fSocket.c
* 文件标识： // 见配置管理计划书
* 内容摘要： 网络连接
* 其它说明： // 其它内容的说明
* 当前版本： // 输入当前版本
* 作    者： 蔡清发
* 完成日期： 
* 
* 修改记录1：// 修改历史记录，包括修改日期、修改者及修改内容
*    修改日期：
*    版 本 号：
*    修 改 人：
*    修改内容： 
* 修改记录2：…
************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <netdb.h>
#include "fSocket.h"
#include <errno.h>

void fSocketClean(int *FD)
{
    if (*FD != -1)
    {
        close(*FD);
        *FD = -1;
    }
}

int fSocketServer(uint16_t port)
{
    int fd = -1, ret;
    struct sockaddr_in sockaddrin;
    // 建立网络连接
    fd = socket(PF_INET, SOCK_STREAM, 0);
    if (fd == -1)
    {
        return -1;
    }
    // 设置网络连接属性
    sockaddrin.sin_addr.s_addr = htonl(INADDR_ANY );
    sockaddrin.sin_family = AF_INET;
    sockaddrin.sin_port = htons(port);
    // 绑定属性
    ret = bind(fd, (struct sockaddr*) &sockaddrin, sizeof(struct sockaddr));
    if (ret == -1)
    {
        close(fd);
        return -1;
    }
    // 监听端口
    ret = listen(fd, 3);
    if (ret == -1)
    {
        close(fd);
        return -1;
    }
    // 返回
    return fd;
}

int fSocketClient(char *IP, uint16_t port)
{
    int FD;
    int ret;
    struct sockaddr_in sockaddrin = { 0 };
    // 初始化网络连接参数
    sockaddrin.sin_family = AF_INET;
    sockaddrin.sin_port = htons(port);
    sockaddrin.sin_addr.s_addr = inet_addr(IP);
    // 建立网络连接
    FD = socket(PF_INET, SOCK_STREAM, 0);
    if (FD == -1)
    {
        return -1;
    }
    // 设置为非阻塞
	unsigned long u1 = 1;
    //ioctl(FD,FIONBIO, (unsigned long *) &u1);

    // 连接远端
    ret = connect(FD, (struct sockaddr*) &sockaddrin, sizeof(struct sockaddr));
    if (ret == -1)
    {
		if(errno != EINPROGRESS)
		{
			perror("connect");
			close(FD);
			return -1;
		}
    }
	ioctl(FD,FIONBIO, (unsigned long *) &u1);

	ret = fSelect(FD, 3000, FSELECT_WRITE);
	if(ret <= 0)
	{
		errno = ETIMEDOUT;
		perror("select");
		return -1;
	}
	// 设置回阻塞模式
	unsigned long u0 = 0;
	ioctl(FD, FIONBIO, (unsigned long*)&u0); 
	
    // 返回描述符
    return FD;
}

int fSocketAccept(int FD, struct sockaddr* clientin)
{
    int fd = -1;
    unsigned int sockLen = sizeof(struct sockaddr);
    struct sockaddr tClientin;
    // 等待客户端连接
    if (-1 == (fd = accept(FD, &tClientin, &sockLen)))
    {
        return -1;
    }
    // 保存客户端信息
    if (clientin != NULL )
    {
        memmove(clientin, &tClientin, sockLen);
    }
    // 返回客户端FD
    return fd;
}

int fSocketSend(int FD, char *buffer, size_t size)
{
    // 发送数据
    return send(FD, buffer, size, 0);
}

int fSocketRecv(int FD, char *buffer, size_t size, int mTimeWait)
{
    // 如果mTimeWait<0，接收阻塞
    if (mTimeWait < 0)
        return recv(FD, buffer, size, 0);
    // 如果mTimeWait>=0，等待mTimeWait时间
    if (fSelect(FD, mTimeWait, FSELECT_RECV))
    {
        // 接收数据
        return recv(FD, buffer, size, 0);
    }
    return -1;
}

char localIP[64];
char* getLocalIP()
{
    memset(localIP, 0, sizeof(localIP));
    // 获得本机IP地址
    struct ifconf ifc;
    struct ifreq *ifreq;
    char buf[512];
    char* localip = NULL;
    int fd = -1;

    if (-1 == (fd = socket(PF_INET, SOCK_STREAM, 0)))
    {
        return localIP;
    }

    // 初始化ifconf
    ifc.ifc_len = 512;
    ifc.ifc_buf = buf;

    if (ioctl(fd, SIOCGIFCONF, &ifc))
    {
        close(fd);
        return localIP;
    }

    ifreq = (struct ifreq*) buf;
    int i;
    for (i = (ifc.ifc_len / sizeof(struct ifreq)); i > 0; --i)
    {
        localip = inet_ntoa(
                ((struct sockaddr_in*) &(ifreq->ifr_addr))->sin_addr);
        // 跳过“127.0.0.1”
        if (strcmp(localip, "127.0.0.1") == 0)
        {
            ++ifreq;
            continue;
        }
        else
        {
            memcpy(localIP, localip, strlen(localip));
            break;
        }
    }

    close(fd);
    return localIP;
}

