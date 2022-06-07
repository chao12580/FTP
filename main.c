/************************************************************************
* ��Ȩ���� (C)2012, ������ͨ����������޹�˾��
* 
* �ļ����ƣ� main.c
* �ļ���ʶ�� // �����ù���ƻ���
* ����ժҪ�� ftp�ͻ�������������ֲ�����ػ������޸�
* ����˵���� // �������ݵ�˵��
* ��ǰ�汾�� // ���뵱ǰ�汾
* ��    �ߣ� ���巢
* ������ڣ� 
* 
* �޸ļ�¼1��// �޸���ʷ��¼�������޸����ڡ��޸��߼��޸�����
*    �޸����ڣ�2013��04��25��
*    �� �� �ţ�
*    �� �� �ˣ����巢
*    �޸����ݣ������ϴ�Ŀ¼�ṹ�ϴ��ļ�
* �޸ļ�¼2����
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

// �����ϴ�Ŀ¼�ļ��ʱ�䣬һ��
#define FTP_UPLOAD_SLEEP_TIME      5
// �ص�¼�ļ��ʱ�䣬һ��
#define FTP_RELOGIN_SLEEP_TIME     5
// FTP����һ��ʱ���Ͽ����ӣ���ʱ��Ϊ�Ͽ����Ӻ������ӵļ��ʱ�䣬ʮ��
#define FTP_DISCONNECT_SLEEP_TIME  10

T_FTPClientStatus g_tFTPClientStatus;

/****************************************************************************** 
 * FUNCTION: is_delFile 
 * DESCRIPTION:�ж��Ƿ�ɾ������Ŀ¼���ļ�
 *  
 * Input:  path:���жϵ��ļ�·������;
 *              
 * Output: null
 * Returns: �жϽ��
 *  
 * modification history 
 * -------------------- 
 *  �޸�����    �汾��     �޸���	     �޸�����
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

	if(time_relative > g_tFTPClientStatus.dir_dueTime)//ɾ����Ŀ¼
	{
		printf("Rm the dir %s\n", path);
		if(rmdir(path) == -1)//�ǿ�Ŀ¼���ش����Դ����ж�Ŀ¼�Ƿ�Ϊ��
		{
			perror("Del dir err:");
			return -1;
		}	
	}
	else
		return -1;
	return 0;
}

// �����ϴ���Ŀ¼�ļ�
int ftwDir(char *p_localDir, char *p_FTPDir, int isRemoveDir)
{
    assert(p_localDir);
    assert(p_FTPDir);
    char tmpUploadDir[256];
    snprintf(tmpUploadDir, 256, "%s%s", p_localDir, p_FTPDir);
    // ����Ŀ¼�µ��ļ�
    if (-1 == ftpUpload(&g_tFTPClientStatus, tmpUploadDir, p_FTPDir))
    {
    	printf("%s_%d: ftpUpload failed!\n",__FUNCTION__,__LINE__);
        return -1;
    }
    DIR *p_dir;
    struct dirent *p_dirent;
    p_dir = opendir(tmpUploadDir);
    // ���ϴ�Ŀ¼����
    if(p_dir == NULL)
    {
        perror("open ftwDir");
        return 0;
    }
    int SubFile = 0;
    char nextSubDir[256];
    // ����Ŀ¼�������ļ�
    while (1)
    {
        p_dirent = readdir(p_dir);
        // ��ȡĿ¼����β������ѭ��
        if(p_dirent == NULL)
        {
            break;
        }
        // ����Ŀ¼"."��".."
        if(strcmp(p_dirent->d_name, ".") == 0 || strcmp(p_dirent->d_name, "..") == 0)
        {
            continue;
        }
        // ����Ŀ¼���ļ�
        ++SubFile;
        // ��Ŀ¼
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
    // ɾ��Ŀ¼
    if(SubFile == 0 && isRemoveDir)
    {
        //rmdir(tmpUploadDir);
        is_delFile(tmpUploadDir);
    }
    return 1;
}

/****************************************************************************** 
 * FUNCTION: getFileSizeSystemCall 
 * DESCRIPTION:��ȡָ���ļ��Ĵ�С  
 *	
 * Input:  strFileName:���жϵ��ļ�·������;
 * Output: null
 * Returns: �жϽ��(�ļ���С)
 *	
 * modification history 
 * -------------------- 
 *	�޸�����	�汾��	   �޸���		 �޸�����
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
 * DESCRIPTION:��ȡָ���ļ��Ĵ�С  
 *	
 * Input:  info:���������־��Ϣ;
 * Output: null
 * Returns: -1,fail;>0,success 
 *	
 * modification history 
 * -------------------- 
 *	�޸�����	�汾��	   �޸���		 �޸�����
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


// �Ƿ��Ѿ������ó����Ƿ���1�����ǻ���󷵻�0
int HadOpenProgram(int argc, char **argv)
{
    // �ж��Ƿ��Ѿ�����һ����ǰ����
    FILE *pFile;
    char tmpStr[256];
    char strPid[32] = { 0 };
    char *pCursor = rindex(argv[0], '/');
    // ��ȡ�����������ɻ�ȡ����PID������
    if(pCursor == NULL)
    {
        snprintf(tmpStr, 256, "pidof %s", argv[0]);
    }
    else
    {
        snprintf(tmpStr, 256, "pidof %s", ++pCursor);
    }
    // �����ܵ�ִ������
    if((pFile = popen(tmpStr, "r")) == NULL)
    {
        perror("popen");
        return 0;
    }
    // ��ȡPID
    if(fgets(strPid, 31, pFile) == NULL)
    {
        perror("fgets");
        pclose(pFile);
        return 0;
    }
    // ���ܷ��ض��PID���Կո����
    // ����ж��PID��������ó���֮ǰ��ʵ��������
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
 * DESCRIPTION:��ʱ�����߳�;
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

	if((fd=open(FIFO_CHANNEL,O_RDONLY|O_NONBLOCK))==-1)  /* ��ֻ����ʽ�������ܵ� */
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
 * DESCRIPTION: ��ʱ�˳�Ӧ�ó���;
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
	
    // ����SIGPIPE�ź�
    signal(SIGPIPE, SIG_IGN);
    // ��ȡ����
    if (!fConfigureRead(&g_tFTPClientStatus))
    {
        exit(0);
    }

	if(g_tFTPClientStatus.prog_dueTime > 0)//�Ƿ�������ʱ����
	{
		if(porting_programFixTime() < 0)
		{
			printf("Notice: program FixTime failed!\n");
		}
	}
	
    // �������������ʱ
    while (g_tFTPClientStatus.IP_cmd[0] && g_tFTPClientStatus.user[0] && g_tFTPClientStatus.pass[0] && g_tFTPClientStatus.localDir[0])
    {
        // ��ѭ��Ϊ�˷�ֹ��¼ʧ�ܺ��˳�����
        while(1)
        {
            // ��¼FTP
            if (ftpLogin(&g_tFTPClientStatus) < 1)
            {
                break;
            }
            // ����Ŀ¼�������ļ�
            while (1)
            {
                if(-1 == ftwDir(g_tFTPClientStatus.localDir, "", 0))
                {
                    // FTP�ϴ���������ѭ��
                    break;
                }
                printf("ftw once\n");
                sleep(FTP_UPLOAD_SLEEP_TIME);
            }
            // �˳���¼
            ftpExit(&g_tFTPClientStatus);
            printf("relogin once\n");
            sleep(FTP_RELOGIN_SLEEP_TIME);
        }
        printf("reconnect once\n");
        sleep(FTP_DISCONNECT_SLEEP_TIME);
    }
    exit(0);
}
