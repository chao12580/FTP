/************************************************************************
* 版权所有 (C)2012, 福建视通光电网络有限公司。
* 
* 文件名称： FTPCC.c
* 文件标识： // 见配置管理计划书
* 内容摘要： ftp客户端基本命令及操作
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
// TCP/IP
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <sys/select.h>
// 文件读取
#include <fcntl.h>
#include <sys/ioctl.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <errno.h>
#include <assert.h>
#include "FTPCC.h"
#include "fSocket.h"

// 接收应答的等待时间，毫秒
#define FTP_RECV_MWAITTIME   10000

// 命令缓存长度
#define SEND_BUFFER_SIZE    256
#define RECV_BUFFER_SIZE    1024
// 发送命令缓存
char g_SendCommandBuffer[SEND_BUFFER_SIZE];
// 接收命令缓存
char g_RecvCommandBuffer[RECV_BUFFER_SIZE];
// 生成完整发送命令
char* makeFullCommand(char* p_cmd, char* p_ctx)
{
    assert(p_cmd);
    assert(p_ctx);
    snprintf(g_SendCommandBuffer, SEND_BUFFER_SIZE, "%s%s\r\n", p_cmd, p_ctx);
    return g_SendCommandBuffer;
}
// 取得发送命令缓存地址，同时赋值
#define getSendBuffer(cmd, ctx) makeFullCommand(cmd, ctx)
// 取得接收命令缓存地址
#define getRecvBuffer()         (g_RecvCommandBuffer)

// 定义当前目录缓存
#define PATH_SIZE           256
char g_currentDirectory[PATH_SIZE] = { 0 };

// 提取服务器应答码
int ftpResponseCode(char *p_Response)
{
    assert(p_Response);
    int responseCode = 0;
    sscanf(p_Response, "%d %*s\r\n", &responseCode);
    return responseCode;
}

// 建立与FTP服务器的连接
int ftpConnect(char *p_IP, uint16_t port)
{
    assert(p_IP);
    // 连接未建立，建立连接
    int fd = fSocketClient(p_IP, port);
    // 建立连接失败，返回假
    if (fd == -1)
    {
        perror("Socket");
        return -1;
    }
    // 建立连接成功，接收欢迎信息
    // 获取接收缓存地址
    char* p_cmd = getRecvBuffer();
	    // 等待网络连接可读
    int selectRet = fSelect(fd, FTP_RECV_MWAITTIME, FSELECT_READ);
    if (selectRet < 1)
    {
        perror("Select Connect");
		close(fd);
        return -1;
    }
    // 接收欢迎信息
    int ret = recv(fd, p_cmd, RECV_BUFFER_SIZE - 1, 0);
    if (ret == -1 || ret == 0)
    {
	fprintf(stderr, "FTP connect: %d\n", errno);
        perror("Receive FTP Welcome Message");
        close(fd);
        return -1;
    }
    // 字符串结尾加结束符
    p_cmd[ret] = '\0';
    ret = ftpResponseCode(p_cmd);
    if (220 == ret)
    {
        return fd;
    }
    else
    {
        // FTP出错，打印错误信息
        fprintf(stderr, "FTP connect: %s\n", p_cmd);
        close(fd);
        return -1;
    }
}

// 向FTP服务器发收一次命令
int ExchangeCMDOnce(int fd, char *p_type, char *p_ctx) {
    assert(p_type);
    assert(p_ctx);
    char* p_cmd;
    // 生成发送命令
    p_cmd = getSendBuffer(p_type, p_ctx);
#ifdef _FDEBUG
    // 打印发送命令
    printf("request: %s\n", p_cmd);
#endif
    // 发送命令
    int sendRet = send(fd, p_cmd, strlen(p_cmd), 0);
    if (sendRet < 1)
    {
        perror("Send Command");
        return -1;
    }
    // 等待网络连接可读
    int selectRet = fSelect(fd, FTP_RECV_MWAITTIME, FSELECT_READ);
    if (selectRet < 1)
    {
        perror("Select Connect");
        return -1;
    }
    p_cmd = getRecvBuffer();
    // 接收FTP服务器应答
    int recvRet = recv(fd, p_cmd, RECV_BUFFER_SIZE - 1, 0);
    if (recvRet == -1)
    {
        perror("Receive Reply");
        return -1;
    }
    // 字符串结尾加结束符
    p_cmd[recvRet] = '\0';
#ifdef _FDEBUG
    // 打印应答信息
    printf("response: %s\n", p_cmd);
#endif
    // 提取应答码
    return ftpResponseCode(p_cmd);
}

// 验证用户
int ftpUSER(int fd, char *p_user)
{
    assert(p_user);
    // 用户验证
    int ret = ExchangeCMDOnce(fd, "USER ", p_user);
    // 网络错误
    if(ret == -1)
    {
    	printf("%s_%d: ExchangeCMDOnce failed!\n",__FUNCTION__,__LINE__);
        return -1;
    }
    // 应答码错误
    if (331 != ret)
    {
        char *p_cmd = getRecvBuffer();
        fprintf(stderr, "USER: %s\n", p_cmd);
        return 0;
    }
    return 1;
}

// 验证密码
int ftpPASS(int fd, char *p_pass)
{
    assert(p_pass);
    // 密码验证
    int ret = ExchangeCMDOnce(fd, "PASS ", p_pass);
    // 网络错误
    if(ret == -1)
    {
    	printf("%s_%d: ExchangeCMDOnce failed!\n",__FUNCTION__,__LINE__);
        return -1;
    }
    // 应答码错误
    if (230 != ret)
    {
        char *p_cmd = getRecvBuffer();
        fprintf(stderr, "PASS: %s\n", p_cmd);
        return 0;
    }
    return 1;
}

// 退出登录
int ftpQUIT(int fd)
{
    int ret = ExchangeCMDOnce(fd, "QUIT", "");
    // 网络错误
    if(ret == -1)
    {
        return -1;
    }
    // 应答码错误
    if (221 != ret)
    {
        char *p_cmd = getRecvBuffer();
        fprintf(stderr, "QUIT: %s\n", p_cmd);
        return 0;
    }
    return 1;
}

// 读取当前目录
int ftpPWD(int fd)
{
    g_currentDirectory[0] = '\0';
    int ret = ExchangeCMDOnce(fd, "PWD", "");
    // 网络错误
    if(ret == -1)
    {
    	printf("%s_%d: ExchangeCMDOnce failed!\n",__FUNCTION__,__LINE__);
        return -1;
    }
    // 应答码错误
    if (257 != ret)
    {
        char *p_cmd = getRecvBuffer();
        fprintf(stderr, "PWD: %s\n", p_cmd);
        return 0;
    }
    // 截取返回内容中的目录信息，该目录信息放在两个双引号之间
    char *p_start;
    char *p_end;
    char *p_cmd = getRecvBuffer();
    // 定位第一个引号位置
    p_start = index(p_cmd, '\"');
    ++p_start;
    // 定位第二个引号位置
    p_end = index(p_start, '\"');
    // 判断目录名长度，比缓存大则取缓存大小截短
    int dirSize = p_end - p_start;
    if(dirSize >= sizeof(g_currentDirectory))
    {
        dirSize = sizeof(g_currentDirectory) - 1;
    }
    memcpy(g_currentDirectory, p_start, dirSize);
    // 字符串结尾加结束符
    g_currentDirectory[dirSize] = '\0';
    return 1;
}

char* ftpGetPWD()
{
    return g_currentDirectory;
}

// 进入FTP目录
int ftpCWD(int fd, char *p_dir)
{
    assert(p_dir);
    int ret = ExchangeCMDOnce(fd, "CWD ", p_dir);
    // 网络错误
    if(ret == -1)
    {
        return -1;
    }
    // 应答码错误
    if (250 != ret)
    {
        char *p_cmd = getRecvBuffer();
        fprintf(stderr, "CWD: %s\n", p_cmd);
        return 0;

    }
    return 1;
}

// 创建目录
int ftpMKD(int fd, char *p_dir)
{
    assert(p_dir);
    int ret = ExchangeCMDOnce(fd, "MKD ", p_dir);
    // 网络错误
    if(ret == -1)
    {
        return -1;
    }
    // 应答码错误
    if (257 != ret)
    {
        char *p_cmd = getRecvBuffer();
        fprintf(stderr, "MKD: %s\n", p_cmd);
        return 0;
    }
    return 1;
}

// 设置数据传输为被动模式，并读取IP与端口信息
int ftpPASV(int fd, char *p_IP, uint16_t *p_port, size_t IPSize)
{
    assert(p_IP);
    assert(p_port);
    int ret = ExchangeCMDOnce(fd, "PASV", "");
    // 网络错误
    if(ret == -1)
    {
    	printf("%s_%d: ExchangeCMDOnce failed!\n",__FUNCTION__,__LINE__);
        return -1;
    }
    // 应答码错误
    if(227 != ret)
    {
        char *p_cmd = getRecvBuffer();
        fprintf(stderr, "PASV: %s\n", p_cmd);
        return 0;
    }
    char *cmd = getRecvBuffer();
    /* FTP返回的内容中包括数据连接的IP与端口信息
     * 格式为(ip1,ip2,ip3,ip4,port1,port2)
     * 正确的端口号为256*port1+port2
     */
    // 定位第一个'('的位置
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

// 建立与FTP服务器的数据连接
int ftpSocketConnectData(char *p_IP, uint16_t port)
{
    assert(p_IP);
    // 建立与FTP服务器的数据连接
    return fSocketClient(p_IP, port);
}

// 发送文件名
int ftpSTOR(int fd, char *p_filename)
{
    assert(p_filename);
    int ret = ExchangeCMDOnce(fd, "STOR ", p_filename);
    // 网络错误
    if(ret == -1)
    {
    	printf("%s_%d: ExchangeCMDOnce failed!\n",__FUNCTION__,__LINE__);
        return -1;
    }
    // 应答码错误
    if (150 != ret)
    {
        char *p_cmd = getRecvBuffer();
        fprintf(stderr, "STOR: %s\n", p_cmd);
        return 0;

    }
    return 1;
}

// 发送文件
int ftpSendFile(int fd_cmd, int fd_data, char *p_filename)
{
	int i=0;

    assert(p_filename);
    char g_NetBuffer[4096];
    // 打开文件
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
    size1 = lseek(fd_file,0,SEEK_END);   ///将文件指针移动文件结尾
	usleep(50*1000);
	size2 = lseek(fd_file,0,SEEK_END); 
    if (size1 != size2)
    {
		fprintf(stdout, "size1 != size2: %d,%d\n", size1,size2);
        // 关闭文件
        close(fd_file);
        // 关闭数据连接
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
        // 读取文件
        readRet = read(fd_file, g_NetBuffer, 4096);
        // 计算读取总字节数
        readNum += readRet;
        // 读取文件错误
        if (readRet == -1)
        {
            perror("Read Send File");
            break;
        }
        // 读到文件尾
        if (readRet == 0)
        {
            break;
        }
        // 发送
        sendRet = send(fd_data, g_NetBuffer, readRet, 0);
        // 计算发送总字节数
        sendNum += sendRet;
        // 发送错误
        if (sendRet == -1)
        {
            perror("Transfer Send file");
            //break;
			// 关闭文件
			close(fd_file);
			// 关闭数据连接
			close(fd_data);	            
            return -1;	// 处理服务器删掉文件夹导致上传失败的问题
        }
    }	
    // 关闭文件
    close(fd_file);
    // 关闭数据连接
    close(fd_data);	
    // 接收FTP服务器应答
    // 判断命令连接是否可读
    if (fSelect(fd_cmd, 500, FSELECT_RECV) < 0)
    {
    	perror("select");
        return -1;
    }
    int recvRet = recv(fd_cmd, g_NetBuffer, sizeof(g_NetBuffer) - 1, 0);
    // 接收错误，recvRet等于0或-1
    if (recvRet < 1)
    {
        perror("Receive Transfer Reply");
        return -1;
    }
    // 应答码正确
    if (226 == ftpResponseCode(g_NetBuffer))
    {
        // 判断是否完整发送
        if(readNum == sendNum && readNum > 0)
        {
			//fprintf(stdout, "total:%d\n",sendNum);
            return 1;
        }
        else if(readNum == 0)//3次判断0KB文件，若还为0kb，则删除文件
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
    // 应答码错误
    else
    {
        char *p_cmd = getRecvBuffer();
        fprintf(stderr, "Transfer: %s\n", p_cmd);
        return 0;
    }
}




