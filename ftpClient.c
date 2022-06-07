/************************************************************************
* ��Ȩ���� (C)2012, ������ͨ����������޹�˾��
*
* �ļ����ƣ� ftpClient.c
* �ļ���ʶ�� // �����ù���ƻ���
* ����ժҪ�� ftp�ͻ���Ӧ����غ���
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
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <assert.h>
#include "ftpClient.h"
// ��¼FTP������
int ftpLogin(T_FTPClientStatus *p_tStatus)
{
    assert(p_tStatus);
    // ����FTP������
    p_tStatus->fd_cmd = ftpConnect(p_tStatus->IP_cmd, p_tStatus->port_cmd);
    if(p_tStatus->fd_cmd == -1)
    {
    	printf("%s_%d: ftpConnect failed!\n",__FUNCTION__,__LINE__);
        return -1;
    }
    // ��¼
    if (1 == ftpUSER(p_tStatus->fd_cmd, p_tStatus->user)
            && 1 == ftpPASS(p_tStatus->fd_cmd, p_tStatus->pass))
    {
        // ��¼�ɹ�
        printf("Login %s:%d by %s\n", p_tStatus->IP_cmd, p_tStatus->port_cmd, p_tStatus->user);
        // ��ȡ��Ŀ¼
        if (1 == ftpPWD(p_tStatus->fd_cmd) && *ftpGetPWD() != '\0')
        {
            snprintf(p_tStatus->rootDir, sizeof(p_tStatus->rootDir), "%s", ftpGetPWD());
            // ��λ�����ַ���β
            char *p_Cursor = p_tStatus->rootDir + strlen(p_tStatus->rootDir) - 1;
            // ����ַ���β��'/'�ַ���ȥ��
            if(*p_Cursor == '/' && p_tStatus->rootDir[1] != '\0')
            {
                *p_Cursor = '\0';
            }
            return 1;
        }
        else
        {
            // ��ȡʧ�ܣ��˳�
            ftpExit(p_tStatus);
			printf("%s_%d: ftpLogin failed!\n",__FUNCTION__,__LINE__);
            return 0;
        }
    }
    else
    {
        // ��¼ʧ�ܣ��˳�
        ftpExit(p_tStatus);
        fprintf(stderr, "Login Fail!\nUser: %s, Pass: %s\n", p_tStatus->user, p_tStatus->pass);
        return -1;
    }
}

// �жϵ�ǰĿ¼��Ŀ��Ŀ¼�Ƿ�һ�£������һ�£�����Ŀ¼
int CDDirectory(T_FTPClientStatus *p_tStatus, char *p_FTPDir)
{
    assert(p_tStatus);
    assert(p_FTPDir);
    int ftpRet;
    // ��ȡ��ǰĿ¼
    ftpRet = ftpPWD(p_tStatus->fd_cmd);
    // �������
    if(ftpRet == -1)
    {
    	printf("%s_%d: CDDirectory failed!\n",__FUNCTION__,__LINE__);
        return -1;
    }
    char *currentDirectory = ftpGetPWD();
    // ��ȡĿ¼����
    if(ftpRet == 0 || *currentDirectory == '\0')
    {
    	printf("%s_%d: ftpGetPWD failed!\n",__FUNCTION__,__LINE__);
        return 0;
    }
    char tmpStr[256];
    snprintf(tmpStr, 256, "%s%s", p_tStatus->rootDir, p_FTPDir);
    // �Աȵ�ǰĿ¼������Ŀ��Ŀ¼
    if(strcmp(currentDirectory, p_FTPDir) == 0)
    {
        // һ�£��������Ŀ¼��������
        return 1;
    }
    // ��һ�£���Ҫ����Ŀ¼
    // ���ڼ�����Ҫ����Ҫ�Ӹ�Ŀ¼��ʼ���Խ�����Ŀ¼������Ŀ¼
    // �����Ŀ¼
    ftpRet = ftpCWD(p_tStatus->fd_cmd, p_tStatus->rootDir);
    if (ftpRet < 1)
    {
        // �������
        printf("%s_%d: ftpCWD failed!\n",__FUNCTION__,__LINE__);
        return ftpRet;
    }
    else
    {
        // ���������Ŀ¼Ϊ�գ�������Ŀ¼
        if(p_FTPDir[0] == '\0')
        {
            return 1;
        }
        // ����Ŀ��Ŀ¼��Ŀ¼���棬��Ŀ¼�����Ͻ��в���
        char directoryBuffer[strlen(p_FTPDir) + 1];
        memcpy(directoryBuffer, p_FTPDir, strlen(p_FTPDir) + 1);
        char *p_start  = directoryBuffer;
        char *p_cursor = directoryBuffer;
        // ����·����ͷ��'/'
        if (p_start[0] == '/')
        {
            ++p_start;
            p_cursor = p_start;
        }
        // ���Ŀ���Ǹ�Ŀ¼��������
        if(p_start[0] == '\0')
        {
            return 1;
        }
        // ����·���ַ����������ȡĿ¼
        while (p_cursor != NULL )
        {
            // �ҵ���һ��'/'λ�ã���λ������Ϊ�ַ���������
            p_cursor = index(p_start, '/');
            if (p_cursor != NULL)
            {
                *p_cursor = '\0';
            }
            ftpRet = ftpCWD(p_tStatus->fd_cmd, p_start);
            // �������
            if(ftpRet == -1)
            {
            	printf("%s_%d: ftpCWD failed!\n",__FUNCTION__,__LINE__);
                return -1;
            }
            // Ŀ¼�޷�����
            if (ftpRet == 0)
            {
                // ���Խ���Ŀ¼
                ftpRet = ftpMKD(p_tStatus->fd_cmd, p_start);
                if (ftpRet < 1)
                {
                    // ����Ŀ¼ʧ�ܣ����ؼ�
                    return ftpRet;
                }
                // ����Ŀ¼
                ftpRet = ftpCWD(p_tStatus->fd_cmd, p_start);
                if (ftpRet < 1)
                {
                    // ����ʧ�ܣ����ؼ�
                    printf("%s_%d: ftpCWD failed!\n",__FUNCTION__,__LINE__);
                    return ftpRet;
                }
            }
            // ����ɹ�����λ����һ��Ŀ¼
            p_start = p_cursor + 1;
        };
        return 1;
    }
}

// ��ȡĿ¼���ļ������ϴ���FTP
int ftpUpload(T_FTPClientStatus *p_tStatus, char *p_UploadDir, char *p_FTPSubDir)
{
    assert(p_tStatus);
    assert(p_UploadDir);
    // ��Ŀ¼
    DIR *p_dir;
    p_dir = opendir(p_UploadDir);
    if (p_dir == NULL)
    {
        perror("Open Upload Directory");
        return 0;
    }
    // ����Ŀ¼
    struct dirent *p_dirent;
    char fullfilename[256];
    int  uploadNumber = 0;
    int  ftpRet;
    while (1)
    {
        p_dirent = readdir(p_dir);
        // ������Ŀ¼��ͷ
        if (p_dirent == NULL)
        {
            break;
        }
        // ����Ŀ¼"."��".."
        if(strcmp(p_dirent->d_name, ".") == 0 || strcmp(p_dirent->d_name, "..") == 0)
        {
            continue;
        }
        // ��������Ŀ¼
        if (p_dirent->d_type == DT_DIR)
        {
            continue;
        }
        // �������ϴ�Ŀ¼��Ŀ¼һ����FTPĿ¼
        if(-1 == CDDirectory(p_tStatus, p_FTPSubDir))
        {
            // ʧ�ܣ����ô����룬����ѭ��
            uploadNumber = -1;
			printf("%s_%d: CDDirectory failed!\n",__FUNCTION__,__LINE__);
            break;
        }
        // ���ñ���ģʽ����ȡ��������IP�Ͷ˿�
        ftpRet = ftpPASV(p_tStatus->fd_cmd, p_tStatus->IP_data, &p_tStatus->port_data, sizeof(p_tStatus->IP_data));
        if (ftpRet < 1)
        {
            // ʧ�ܣ����ô����룬����ѭ��
            uploadNumber = -1;
			printf("%s_%d: ftpPASV failed!\n",__FUNCTION__,__LINE__);
            break;
        }
        // ��������
        p_tStatus->fd_data = ftpSocketConnectData(p_tStatus->IP_data,
                p_tStatus->port_data);
        if (p_tStatus->fd_data == -1)
        {
            // ʧ�ܣ����ô����룬����ѭ��
            uploadNumber = -1;
			printf("%s_%d: ftpSocketConnectData failed!\n",__FUNCTION__,__LINE__);
            break;

        }
        // �����ļ���
        ftpRet = ftpSTOR(p_tStatus->fd_cmd, p_dirent->d_name);
        if (ftpRet < 1)
        {
        	close(p_tStatus->fd_data);
            // ʧ�ܣ����ô����룬����ѭ��			
            uploadNumber = -1;
			printf("%s_%d: ftpSTOR failed!\n",__FUNCTION__,__LINE__);
            break;
        }
        // �����ļ�
        snprintf(fullfilename, sizeof(fullfilename), "%s/%s", p_UploadDir, p_dirent->d_name);
        ftpRet = ftpSendFile(p_tStatus->fd_cmd, p_tStatus->fd_data, fullfilename);
		if (ftpRet == 0)
		{
			wirteInfoToLog("Ftp send file fail!\n");
		    continue;
		}
        if (ftpRet == -1)
        {
        	close(p_tStatus->fd_data);
            // ʧ�ܣ����ô����룬����ѭ��			
            uploadNumber = -1;
			printf("%s_%d: ftpSendFile failed!\n",__FUNCTION__,__LINE__);
            break;

        }
        // ���ͳɹ���ɾ���ļ������ϴ�����+1
        remove(fullfilename);
        printf("upload && remove success!\n    file: %s\n\n", fullfilename);
        ++uploadNumber;
    }
    // �ر�Ŀ¼
    closedir(p_dir);
    return uploadNumber;
}

void ftpExit(T_FTPClientStatus *p_tStatus)
{	
    ftpQUIT(p_tStatus->fd_cmd);
    if (p_tStatus->fd_cmd != -1)
    {
        close(p_tStatus->fd_cmd);
        p_tStatus->fd_cmd = -1;
    }
    if (p_tStatus->fd_data != -1)
    {
        close(p_tStatus->fd_data);
        p_tStatus->fd_data = -1;
    }
}

