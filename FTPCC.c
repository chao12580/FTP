/************************************************************************
* ��Ȩ���� (C)2012, ������ͨ����������޹�˾��
* 
* �ļ����ƣ� FTPCC.c
* �ļ���ʶ�� // �����ù���ƻ���
* ����ժҪ�� ftp�ͻ��˻����������
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
// TCP/IP
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <sys/select.h>
// �ļ���ȡ
#include <fcntl.h>
#include <sys/ioctl.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <errno.h>
#include <assert.h>
#include "FTPCC.h"
#include "fSocket.h"

// ����Ӧ��ĵȴ�ʱ�䣬����
#define FTP_RECV_MWAITTIME   10000

// ����泤��
#define SEND_BUFFER_SIZE    256
#define RECV_BUFFER_SIZE    1024
// ���������
char g_SendCommandBuffer[SEND_BUFFER_SIZE];
// ���������
char g_RecvCommandBuffer[RECV_BUFFER_SIZE];
// ����������������
char* makeFullCommand(char* p_cmd, char* p_ctx)
{
    assert(p_cmd);
    assert(p_ctx);
    snprintf(g_SendCommandBuffer, SEND_BUFFER_SIZE, "%s%s\r\n", p_cmd, p_ctx);
    return g_SendCommandBuffer;
}
// ȡ�÷���������ַ��ͬʱ��ֵ
#define getSendBuffer(cmd, ctx) makeFullCommand(cmd, ctx)
// ȡ�ý���������ַ
#define getRecvBuffer()         (g_RecvCommandBuffer)

// ���嵱ǰĿ¼����
#define PATH_SIZE           256
char g_currentDirectory[PATH_SIZE] = { 0 };

// ��ȡ������Ӧ����
int ftpResponseCode(char *p_Response)
{
    assert(p_Response);
    int responseCode = 0;
    sscanf(p_Response, "%d %*s\r\n", &responseCode);
    return responseCode;
}

// ������FTP������������
int ftpConnect(char *p_IP, uint16_t port)
{
    assert(p_IP);
    // ����δ��������������
    int fd = fSocketClient(p_IP, port);
    // ��������ʧ�ܣ����ؼ�
    if (fd == -1)
    {
        perror("Socket");
        return -1;
    }
    // �������ӳɹ������ջ�ӭ��Ϣ
    // ��ȡ���ջ����ַ
    char* p_cmd = getRecvBuffer();
	    // �ȴ��������ӿɶ�
    int selectRet = fSelect(fd, FTP_RECV_MWAITTIME, FSELECT_READ);
    if (selectRet < 1)
    {
        perror("Select Connect");
		close(fd);
        return -1;
    }
    // ���ջ�ӭ��Ϣ
    int ret = recv(fd, p_cmd, RECV_BUFFER_SIZE - 1, 0);
    if (ret == -1 || ret == 0)
    {
	fprintf(stderr, "FTP connect: %d\n", errno);
        perror("Receive FTP Welcome Message");
        close(fd);
        return -1;
    }
    // �ַ�����β�ӽ�����
    p_cmd[ret] = '\0';
    ret = ftpResponseCode(p_cmd);
    if (220 == ret)
    {
        return fd;
    }
    else
    {
        // FTP������ӡ������Ϣ
        fprintf(stderr, "FTP connect: %s\n", p_cmd);
        close(fd);
        return -1;
    }
}

// ��FTP����������һ������
int ExchangeCMDOnce(int fd, char *p_type, char *p_ctx) {
    assert(p_type);
    assert(p_ctx);
    char* p_cmd;
    // ���ɷ�������
    p_cmd = getSendBuffer(p_type, p_ctx);
#ifdef _FDEBUG
    // ��ӡ��������
    printf("request: %s\n", p_cmd);
#endif
    // ��������
    int sendRet = send(fd, p_cmd, strlen(p_cmd), 0);
    if (sendRet < 1)
    {
        perror("Send Command");
        return -1;
    }
    // �ȴ��������ӿɶ�
    int selectRet = fSelect(fd, FTP_RECV_MWAITTIME, FSELECT_READ);
    if (selectRet < 1)
    {
        perror("Select Connect");
        return -1;
    }
    p_cmd = getRecvBuffer();
    // ����FTP������Ӧ��
    int recvRet = recv(fd, p_cmd, RECV_BUFFER_SIZE - 1, 0);
    if (recvRet == -1)
    {
        perror("Receive Reply");
        return -1;
    }
    // �ַ�����β�ӽ�����
    p_cmd[recvRet] = '\0';
#ifdef _FDEBUG
    // ��ӡӦ����Ϣ
    printf("response: %s\n", p_cmd);
#endif
    // ��ȡӦ����
    return ftpResponseCode(p_cmd);
}

// ��֤�û�
int ftpUSER(int fd, char *p_user)
{
    assert(p_user);
    // �û���֤
    int ret = ExchangeCMDOnce(fd, "USER ", p_user);
    // �������
    if(ret == -1)
    {
    	printf("%s_%d: ExchangeCMDOnce failed!\n",__FUNCTION__,__LINE__);
        return -1;
    }
    // Ӧ�������
    if (331 != ret)
    {
        char *p_cmd = getRecvBuffer();
        fprintf(stderr, "USER: %s\n", p_cmd);
        return 0;
    }
    return 1;
}

// ��֤����
int ftpPASS(int fd, char *p_pass)
{
    assert(p_pass);
    // ������֤
    int ret = ExchangeCMDOnce(fd, "PASS ", p_pass);
    // �������
    if(ret == -1)
    {
    	printf("%s_%d: ExchangeCMDOnce failed!\n",__FUNCTION__,__LINE__);
        return -1;
    }
    // Ӧ�������
    if (230 != ret)
    {
        char *p_cmd = getRecvBuffer();
        fprintf(stderr, "PASS: %s\n", p_cmd);
        return 0;
    }
    return 1;
}

// �˳���¼
int ftpQUIT(int fd)
{
    int ret = ExchangeCMDOnce(fd, "QUIT", "");
    // �������
    if(ret == -1)
    {
        return -1;
    }
    // Ӧ�������
    if (221 != ret)
    {
        char *p_cmd = getRecvBuffer();
        fprintf(stderr, "QUIT: %s\n", p_cmd);
        return 0;
    }
    return 1;
}

// ��ȡ��ǰĿ¼
int ftpPWD(int fd)
{
    g_currentDirectory[0] = '\0';
    int ret = ExchangeCMDOnce(fd, "PWD", "");
    // �������
    if(ret == -1)
    {
    	printf("%s_%d: ExchangeCMDOnce failed!\n",__FUNCTION__,__LINE__);
        return -1;
    }
    // Ӧ�������
    if (257 != ret)
    {
        char *p_cmd = getRecvBuffer();
        fprintf(stderr, "PWD: %s\n", p_cmd);
        return 0;
    }
    // ��ȡ���������е�Ŀ¼��Ϣ����Ŀ¼��Ϣ��������˫����֮��
    char *p_start;
    char *p_end;
    char *p_cmd = getRecvBuffer();
    // ��λ��һ������λ��
    p_start = index(p_cmd, '\"');
    ++p_start;
    // ��λ�ڶ�������λ��
    p_end = index(p_start, '\"');
    // �ж�Ŀ¼�����ȣ��Ȼ������ȡ�����С�ض�
    int dirSize = p_end - p_start;
    if(dirSize >= sizeof(g_currentDirectory))
    {
        dirSize = sizeof(g_currentDirectory) - 1;
    }
    memcpy(g_currentDirectory, p_start, dirSize);
    // �ַ�����β�ӽ�����
    g_currentDirectory[dirSize] = '\0';
    return 1;
}

char* ftpGetPWD()
{
    return g_currentDirectory;
}

// ����FTPĿ¼
int ftpCWD(int fd, char *p_dir)
{
    assert(p_dir);
    int ret = ExchangeCMDOnce(fd, "CWD ", p_dir);
    // �������
    if(ret == -1)
    {
        return -1;
    }
    // Ӧ�������
    if (250 != ret)
    {
        char *p_cmd = getRecvBuffer();
        fprintf(stderr, "CWD: %s\n", p_cmd);
        return 0;

    }
    return 1;
}

// ����Ŀ¼
int ftpMKD(int fd, char *p_dir)
{
    assert(p_dir);
    int ret = ExchangeCMDOnce(fd, "MKD ", p_dir);
    // �������
    if(ret == -1)
    {
        return -1;
    }
    // Ӧ�������
    if (257 != ret)
    {
        char *p_cmd = getRecvBuffer();
        fprintf(stderr, "MKD: %s\n", p_cmd);
        return 0;
    }
    return 1;
}

// �������ݴ���Ϊ����ģʽ������ȡIP��˿���Ϣ
int ftpPASV(int fd, char *p_IP, uint16_t *p_port, size_t IPSize)
{
    assert(p_IP);
    assert(p_port);
    int ret = ExchangeCMDOnce(fd, "PASV", "");
    // �������
    if(ret == -1)
    {
    	printf("%s_%d: ExchangeCMDOnce failed!\n",__FUNCTION__,__LINE__);
        return -1;
    }
    // Ӧ�������
    if(227 != ret)
    {
        char *p_cmd = getRecvBuffer();
        fprintf(stderr, "PASV: %s\n", p_cmd);
        return 0;
    }
    char *cmd = getRecvBuffer();
    /* FTP���ص������а����������ӵ�IP��˿���Ϣ
     * ��ʽΪ(ip1,ip2,ip3,ip4,port1,port2)
     * ��ȷ�Ķ˿ں�Ϊ256*port1+port2
     */
    // ��λ��һ��'('��λ��
    char *p_cursor;
    p_cursor = index(cmd, '(');
    char IP0[4];
    char IP1[4];
    char IP2[4];
    char IP3[4];
    unsigned int port1, port2;
    sscanf(p_cursor, "(%[0-9],%[0-9],%[0-9],%[0-9],%d,%d)%*s", IP0, IP1, IP2,
            IP3, &port1, &port2);
    snprintf(p_IP, IPSize, "%s.%s.%s.%s", IP0, IP1, IP2, IP3);
    *p_port = port1 * 256 + port2;
    return 1;
}

// ������FTP����������������
int ftpSocketConnectData(char *p_IP, uint16_t port)
{
    assert(p_IP);
    // ������FTP����������������
    return fSocketClient(p_IP, port);
}

// �����ļ���
int ftpSTOR(int fd, char *p_filename)
{
    assert(p_filename);
    int ret = ExchangeCMDOnce(fd, "STOR ", p_filename);
    // �������
    if(ret == -1)
    {
    	printf("%s_%d: ExchangeCMDOnce failed!\n",__FUNCTION__,__LINE__);
        return -1;
    }
    // Ӧ�������
    if (150 != ret)
    {
        char *p_cmd = getRecvBuffer();
        fprintf(stderr, "STOR: %s\n", p_cmd);
        return 0;

    }
    return 1;
}

// �����ļ�
int ftpSendFile(int fd_cmd, int fd_data, char *p_filename)
{
	int i=0;

    assert(p_filename);
    char g_NetBuffer[4096];
    // ���ļ�
    FILE *P;
    int fd_file = open(p_filename, O_RDONLY);
    if (fd_file == -1)
    {
        perror("Open Send File");
        return 0;
    }
    int readRet = 0;
    int sendRet = 0;
    int readNum = 0;
    int sendNum = 0;
	//printf("%s_%d: p_filename %s\n",__FUNCTION__,__LINE__,p_filename);
	int size1 = 0;
	int size2 = 0;
    size1 = lseek(fd_file,0,SEEK_END);   ///���ļ�ָ���ƶ��ļ���β
	usleep(50*1000);
	size2 = lseek(fd_file,0,SEEK_END); 
    if (size1 != size2)
    {
		fprintf(stdout, "size1 != size2: %d,%d\n", size1,size2);
        // �ر��ļ�
        close(fd_file);
        // �ر���������
        close(fd_data);
        return -1;
    }
    else
    {
        lseek(fd_file,0,SEEK_SET);
        //fseek(fd_file, 0, SEEK_SET);
    }
    while (1)
    {
        // ��ȡ�ļ�
        readRet = read(fd_file, g_NetBuffer, 4096);
        // �����ȡ���ֽ���
        readNum += readRet;
        // ��ȡ�ļ�����
        if (readRet == -1)
        {
            perror("Read Send File");
            break;
        }
        // �����ļ�β
        if (readRet == 0)
        {
            break;
        }
        // ����
        sendRet = send(fd_data, g_NetBuffer, readRet, 0);
        // ���㷢�����ֽ���
        sendNum += sendRet;
        // ���ʹ���
        if (sendRet == -1)
        {
            perror("Transfer Send file");
            //break;
			// �ر��ļ�
			close(fd_file);
			// �ر���������
			close(fd_data);	            
            return -1;	// ���������ɾ���ļ��е����ϴ�ʧ�ܵ�����
        }
    }	
    // �ر��ļ�
    close(fd_file);
    // �ر���������
    close(fd_data);	
    // ����FTP������Ӧ��
    // �ж����������Ƿ�ɶ�
    if (fSelect(fd_cmd, 500, FSELECT_RECV) < 0)
    {
    	perror("select");
        return -1;
    }
    int recvRet = recv(fd_cmd, g_NetBuffer, sizeof(g_NetBuffer) - 1, 0);
    // ���մ���recvRet����0��-1
    if (recvRet < 1)
    {
        perror("Receive Transfer Reply");
        return -1;
    }
    // Ӧ������ȷ
    if (226 == ftpResponseCode(g_NetBuffer))
    {
        // �ж��Ƿ���������
        if(readNum == sendNum && readNum > 0)
        {
			//fprintf(stdout, "total:%d\n",sendNum);
            return 1;
        }
        else if(readNum == 0)//3���ж�0KB�ļ�������Ϊ0kb����ɾ���ļ�
        {
#if 1        
			fd_file = open(p_filename, O_RDONLY);
        	for(i=0;i<3;i++)
        	{
        		if(lseek(fd_file, 0, SEEK_SET) == -1)
        		{
					perror("Lseek file err:");
					close(fd_file);
					return 0;
				}
        		readNum = read(fd_file, g_NetBuffer, 256);
				if(readNum > 0)
					break;
				else
					sleep(1);
        	}	
			close(fd_file);
			if(readNum == 0)
			{
				printf("%s_%d: The file is 0kb,will rm!\n",__FUNCTION__, __LINE__);
				if(unlink(p_filename) == -1)
				{
					perror("Del file err:");
					return 0;
				}
			}
			return 0;
#endif			
        }
		else
		{
            return 0;
        }
    }
    // Ӧ�������
    else
    {
        char *p_cmd = getRecvBuffer();
        fprintf(stderr, "Transfer: %s\n", p_cmd);
        return 0;
    }
}




