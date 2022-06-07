/************************************************************************
* ��Ȩ���� (C)2012, ������ͨ����������޹�˾��
* 
* �ļ����ƣ� fSocket.c
* �ļ���ʶ�� // �����ù���ƻ���
* ����ժҪ�� ��������
* ����˵���� // �������ݵ�˵��
* ��ǰ�汾�� // ���뵱ǰ�汾
* ��    �ߣ� ���巢
* ������ڣ� 
* 
* �޸ļ�¼1��// �޸���ʷ��¼�������޸����ڡ��޸��߼��޸�����
*    �޸����ڣ�
*    �� �� �ţ�
*    �� �� �ˣ�
*    �޸����ݣ� 
* �޸ļ�¼2����
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
    // ������������
    fd = socket(PF_INET, SOCK_STREAM, 0);
    if (fd == -1)
    {
        return -1;
    }
    // ����������������
    sockaddrin.sin_addr.s_addr = htonl(INADDR_ANY );
    sockaddrin.sin_family = AF_INET;
    sockaddrin.sin_port = htons(port);
    // ������
    ret = bind(fd, (struct sockaddr*) &sockaddrin, sizeof(struct sockaddr));
    if (ret == -1)
    {
        close(fd);
        return -1;
    }
    // �����˿�
    ret = listen(fd, 3);
    if (ret == -1)
    {
        close(fd);
        return -1;
    }
    // ����
    return fd;
}

int fSocketClient(char *IP, uint16_t port)
{
    int FD;
    int ret;
    struct sockaddr_in sockaddrin = { 0 };
    // ��ʼ���������Ӳ���
    sockaddrin.sin_family = AF_INET;
    sockaddrin.sin_port = htons(port);
    sockaddrin.sin_addr.s_addr = inet_addr(IP);
    // ������������
    FD = socket(PF_INET, SOCK_STREAM, 0);
    if (FD == -1)
    {
        return -1;
    }
    // ����Ϊ������
	unsigned long u1 = 1;
    //ioctl(FD,FIONBIO, (unsigned long *) &u1);

    // ����Զ��
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
	// ���û�����ģʽ
	unsigned long u0 = 0;
	ioctl(FD, FIONBIO, (unsigned long*)&u0); 
	
    // ����������
    return FD;
}

int fSocketAccept(int FD, struct sockaddr* clientin)
{
    int fd = -1;
    unsigned int sockLen = sizeof(struct sockaddr);
    struct sockaddr tClientin;
    // �ȴ��ͻ�������
    if (-1 == (fd = accept(FD, &tClientin, &sockLen)))
    {
        return -1;
    }
    // ����ͻ�����Ϣ
    if (clientin != NULL )
    {
        memmove(clientin, &tClientin, sockLen);
    }
    // ���ؿͻ���FD
    return fd;
}

int fSocketSend(int FD, char *buffer, size_t size)
{
    // ��������
    return send(FD, buffer, size, 0);
}

int fSocketRecv(int FD, char *buffer, size_t size, int mTimeWait)
{
    // ���mTimeWait<0����������
    if (mTimeWait < 0)
        return recv(FD, buffer, size, 0);
    // ���mTimeWait>=0���ȴ�mTimeWaitʱ��
    if (fSelect(FD, mTimeWait, FSELECT_RECV))
    {
        // ��������
        return recv(FD, buffer, size, 0);
    }
    return -1;
}

char localIP[64];
char* getLocalIP()
{
    memset(localIP, 0, sizeof(localIP));
    // ��ñ���IP��ַ
    struct ifconf ifc;
    struct ifreq *ifreq;
    char buf[512];
    char* localip = NULL;
    int fd = -1;

    if (-1 == (fd = socket(PF_INET, SOCK_STREAM, 0)))
    {
        return localIP;
    }

    // ��ʼ��ifconf
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
        // ������127.0.0.1��
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

