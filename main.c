/************************************************************************
* 版权所有 (C)2012, 福建视通光电网络有限公司。
* 
* 文件名称： main.c
* 文件标识： // 见配置管理计划书
* 内容摘要： ftp客户端主函数，移植到工控机作了修改
* 其它说明： // 其它内容的说明
* 当前版本： // 输入当前版本
* 作    者： 蔡清发
* 完成日期： 
* 
* 修改记录1：// 修改历史记录，包括修改日期、修改者及修改内容
*    修改日期：2013年04月25日
*    版 本 号：
*    修 改 人：蔡清发
*    修改内容：根据上传目录结构上传文件
* 修改记录2：…
************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <sys/stat.h>
#include <dirent.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <assert.h>
#include "ftpClient.h"
#include "fConfigure.h"
#include <time.h>
#include <pthread.h> 
#include <fcntl.h>


//#define _FDEBUG
#define LOGNAME "log"
#define FIFO_CHANNEL "/var/www/cgi-bin/my_fifo"
//#define FIFO_CHANNEL "/home/ubuntu/Desktop/TEST/xlog/fifo/my_fifo"

// 遍历上传目录的间隔时间，一秒
#define FTP_UPLOAD_SLEEP_TIME      5
// 重登录的间隔时间，一秒
#define FTP_RELOGIN_SLEEP_TIME     5
// FTP超过一定时间会断开连接，该时间为断开连接后重连接的间隔时间，十秒
#define FTP_DISCONNECT_SLEEP_TIME  10

T_FTPClientStatus g_tFTPClientStatus;

/****************************************************************************** 
 * FUNCTION: is_delFile 
 * DESCRIPTION:判断是否删除过期目录或文件
 *  
 * Input:  path:欲判断的文件路径名称;
 *              
 * Output: null
 * Returns: 判断结果
 *  
 * modification history 
 * -------------------- 
 *  修改日期    版本号     修改人	     修改内容
 *
 * -------------------- 
 ******************************************************************************/
int is_delFile(const char *path)
{
	struct stat st,parent_st;
	time_t time_now,time_modify,time_relative;
	
	if((time_now = time(NULL)) == -1)
	{
		perror("Get now time err!\n");
		return -1;
	}	

	if(lstat(path, &st) == -1)
	{
		perror("Get file info err!\n");
		return -1;
	}
	time_modify = st.st_mtime;
	
	time_relative = time_now - time_modify;	

	if(time_relative > g_tFTPClientStatus.dir_dueTime)//删除空目录
	{
		printf("Rm the dir %s\n", path);
		if(rmdir(path) == -1)//非空目录返回错误，以此来判断目录是否为空
		{
			perror("Del dir err:");
			return -1;
		}	
	}
	else
		return -1;
	return 0;
}

// 迭代上传子目录文件
int ftwDir(char *p_localDir, char *p_FTPDir, int isRemoveDir)
{
    assert(p_localDir);
    assert(p_FTPDir);
    char tmpUploadDir[256];
    snprintf(tmpUploadDir, 256, "%s%s", p_localDir, p_FTPDir);
    // 发送目录下的文件
    if (-1 == ftpUpload(&g_tFTPClientStatus, tmpUploadDir, p_FTPDir))
    {
    	printf("%s_%d: ftpUpload failed!\n",__FUNCTION__,__LINE__);
        return -1;
    }
    DIR *p_dir;
    struct dirent *p_dirent;
    p_dir = opendir(tmpUploadDir);
    // 打开上传目录出错
    if(p_dir == NULL)
    {
        perror("open ftwDir");
        return 0;
    }
    int SubFile = 0;
    char nextSubDir[256];
    // 遍历目录并发送文件
    while (1)
    {
        p_dirent = readdir(p_dir);
        // 读取目录到结尾，结束循环
        if(p_dirent == NULL)
        {
            break;
        }
        // 跳过目录"."与".."
        if(strcmp(p_dirent->d_name, ".") == 0 || strcmp(p_dirent->d_name, "..") == 0)
        {
            continue;
        }
        // 有子目录或文件
        ++SubFile;
        // 子目录
        if (p_dirent->d_type == DT_DIR)
        {
            snprintf(nextSubDir, 256, "%s/%s", p_FTPDir, p_dirent->d_name);
            if(-1 == ftwDir(p_localDir, nextSubDir, 1))
            {
                closedir(p_dir);
				printf("%s_%d: ftwDir failed!\n",__FUNCTION__,__LINE__);
                return -1;
            }
        }
    }
    closedir(p_dir);
    // 删除目录
    if(SubFile == 0 && isRemoveDir)
    {
        //rmdir(tmpUploadDir);
        is_delFile(tmpUploadDir);
    }
    return 1;
}

/****************************************************************************** 
 * FUNCTION: getFileSizeSystemCall 
 * DESCRIPTION:获取指定文件的大小  
 *	
 * Input:  strFileName:欲判断的文件路径名称;
 * Output: null
 * Returns: 判断结果(文件大小)
 *	
 * modification history 
 * -------------------- 
 *	修改日期	版本号	   修改人		 修改内容
 *
 * -------------------- 
 ******************************************************************************/
int getFileSizeSystemCall(char * strFileName)    
{   
    struct stat temp;   
    stat(strFileName, &temp);   
    return temp.st_size;   
} 

/****************************************************************************** 
 * FUNCTION: wirteInfoToLog 
 * DESCRIPTION:获取指定文件的大小  
 *	
 * Input:  info:欲输出的日志信息;
 * Output: null
 * Returns: -1,fail;>0,success 
 *	
 * modification history 
 * -------------------- 
 *	修改日期	版本号	   修改人		 修改内容
 *
 * -------------------- 
 ******************************************************************************/
char wirteInfoToLog(char *info)
{
	FILE *fp;
	char buf[256] = {0};
	char logTime[64] = {0};
	int size = 0;
	time_t timer;
	struct tm *tblock;
	
	if(getFileSizeSystemCall(LOGNAME) > 10*1024*1024)
	{
		printf("%s_%d: The log is too big,will rm!\n");
		if(unlink(LOGNAME) == -1)
		{
			perror("Del file err:");
			return -1;
		}
	}

	timer = time(NULL);
	tblock = localtime(&timer);

	sprintf(logTime, "[%04.4u-%02.2u-%02.2u %02.2u:%02.2u:%02.2u] ",\
		1990+tblock->tm_year, tblock->tm_mon + 1, tblock->tm_mday,\
		tblock->tm_hour, tblock->tm_min, tblock->tm_sec);

	snprintf(buf,1024,"%s%s\n",	logTime, info);
	size = strnlen(buf, 256);
	if((fp = fopen(LOGNAME,"a")) == NULL)
	{
		perror("Fopen log failed: \n");
		return -1;
	}
	
	if(fwrite(buf, size, 1, fp) !=1)
	{
		perror("Fwrite log failed: \n");
		return -1;
	}
	
	fclose(fp);
	return 0;
}


// 是否已经开启该程序，是返回1，不是或错误返回0
int HadOpenProgram(int argc, char **argv)
{
    // 判断是否已经开启一个当前进程
    FILE *pFile;
    char tmpStr[256];
    char strPid[32] = { 0 };
    char *pCursor = rindex(argv[0], '/');
    // 获取程序名，生成获取程序PID的命令
    if(pCursor == NULL)
    {
        snprintf(tmpStr, 256, "pidof %s", argv[0]);
    }
    else
    {
        snprintf(tmpStr, 256, "pidof %s", ++pCursor);
    }
    // 匿名管道执行命令
    if((pFile = popen(tmpStr, "r")) == NULL)
    {
        perror("popen");
        return 0;
    }
    // 读取PID
    if(fgets(strPid, 31, pFile) == NULL)
    {
        perror("fgets");
        pclose(pFile);
        return 0;
    }
    // 可能返回多个PID，以空格隔开
    // 如果有多个PID，则表明该程序之前有实例在运行
    if(index(strPid, ' ') == NULL)
    {
        return 0;
    }
    else
    {
        return 1;
    }
}

/****************************************************************************** 
 * FUNCTION: porting_programTimeout 
 * DESCRIPTION:定时处理线程;
 *	
 * Input: null
 * Output: null
 * Returns: -1,fail;>0,success 
 *	
 * modification history 
 * -------------------- 
 *	 
 * -------------------- 
 ******************************************************************************/ 
void* porting_programTimeout(void *param)
{
	time_t time_now,time_run,time_relative;	
	int fd;
	char buf[8];
	int rFifo = 0;

	umask(0);
	
	if(mkfifo(FIFO_CHANNEL,0777)==-1) 
	{
		perror("Can't create FIFO channel");
	}

	if((fd=open(FIFO_CHANNEL,O_RDONLY|O_NONBLOCK))==-1)  /* 以只读方式打开命名管道 */
	{
		perror("Can't open the FIFO");
	}
	
	printf("0x%x\n", pthread_self());

	time_relative = 0;
	time_now = time(NULL);
	while(time_relative < g_tFTPClientStatus.prog_dueTime)
	{
		sleep(5);
		time_run = time(NULL);
		time_relative = time_run - time_now;
		printf("time_relative=%d\n",time_relative);

		if(fd !=-1)
		{
			if(read(fd, buf, sizeof(buf)) < 0)
			{
				perror("Read the FIFO err: ");
			}
		}	

		rFifo = atoi(buf);
		if(rFifo == 2)
			exit(0);	
	}
	if(fd !=-1)	
		close(fd);
	exit(0);
}

/****************************************************************************** 
 * FUNCTION: porting_programFixTime 
 * DESCRIPTION: 定时退出应用程序;
 *	
 * Input: null
 * Output: null
 * Returns: -1,fail;>0,success 
 *	
 * modification history 
 * -------------------- 
 *	 
 * -------------------- 
 ******************************************************************************/ 
int porting_programFixTime(void)
{
	int err = 0;
    pthread_attr_t attr;
	static pthread_t progTimeoutThread;


    pthread_attr_init(&attr);

	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
		
	err = pthread_create(&progTimeoutThread, &attr, porting_programTimeout, 0);

	pthread_attr_destroy(&attr);	
	if(err != 0)
	{
		printf("Create programFixTime task failed!");
		return -1;
	}
	return 0;
}

int main(int argc, char** argv)
{
    if(HadOpenProgram(argc, argv))
    {
        fprintf(stderr, "Had Open A Program\n");
        return 0;
    }
	
    // 忽略SIGPIPE信号
    signal(SIGPIPE, SIG_IGN);
    // 读取参数
    if (!fConfigureRead(&g_tFTPClientStatus))
    {
        exit(0);
    }

	if(g_tFTPClientStatus.prog_dueTime > 0)//是否开启程序定时功能
	{
		if(porting_programFixTime() < 0)
		{
			printf("Notice: program FixTime failed!\n");
		}
	}
	
    // 当必需参数完整时
    while (g_tFTPClientStatus.IP_cmd[0] && g_tFTPClientStatus.user[0] && g_tFTPClientStatus.pass[0] && g_tFTPClientStatus.localDir[0])
    {
        // 该循环为了防止登录失败后退出程序
        while(1)
        {
            // 登录FTP
            if (ftpLogin(&g_tFTPClientStatus) < 1)
            {
                break;
            }
            // 遍历目录并发送文件
            while (1)
            {
                if(-1 == ftwDir(g_tFTPClientStatus.localDir, "", 0))
                {
                    // FTP上传出错，结束循环
                    break;
                }
                printf("ftw once\n");
                sleep(FTP_UPLOAD_SLEEP_TIME);
            }
            // 退出登录
            ftpExit(&g_tFTPClientStatus);
            printf("relogin once\n");
            sleep(FTP_RELOGIN_SLEEP_TIME);
        }
        printf("reconnect once\n");
        sleep(FTP_DISCONNECT_SLEEP_TIME);
    }
    exit(0);
}
