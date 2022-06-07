/************************************************************************
* 版权所有 (C)2012, 福建视通光电网络有限公司。
*
* 文件名称： ftpClient.c
* 文件标识： // 见配置管理计划书
* 内容摘要： ftp客户端应用相关函数
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
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <assert.h>
#include "ftpClient.h"
// 登录FTP服务器
int ftpLogin(T_FTPClientStatus *p_tStatus)
{
    assert(p_tStatus);
    // 连接FTP服务器
    p_tStatus->fd_cmd = ftpConnect(p_tStatus->IP_cmd, p_tStatus->port_cmd);
    if(p_tStatus->fd_cmd == -1)
    {
    	printf("%s_%d: ftpConnect failed!\n",__FUNCTION__,__LINE__);
        return -1;
    }
    // 登录
    if (1 == ftpUSER(p_tStatus->fd_cmd, p_tStatus->user)
            && 1 == ftpPASS(p_tStatus->fd_cmd, p_tStatus->pass))
    {
        // 登录成功
        printf("Login %s:%d by %s\n", p_tStatus->IP_cmd, p_tStatus->port_cmd, p_tStatus->user);
        // 读取根目录
        if (1 == ftpPWD(p_tStatus->fd_cmd) && *ftpGetPWD() != '\0')
        {
            snprintf(p_tStatus->rootDir, sizeof(p_tStatus->rootDir), "%s", ftpGetPWD());
            // 定位到该字符串尾
            char *p_Cursor = p_tStatus->rootDir + strlen(p_tStatus->rootDir) - 1;
            // 如果字符串尾是'/'字符，去掉
            if(*p_Cursor == '/' && p_tStatus->rootDir[1] != '\0')
            {
                *p_Cursor = '\0';
            }
            return 1;
        }
        else
        {
            // 读取失败，退出
            ftpExit(p_tStatus);
			printf("%s_%d: ftpLogin failed!\n",__FUNCTION__,__LINE__);
            return 0;
        }
    }
    else
    {
        // 登录失败，退出
        ftpExit(p_tStatus);
        fprintf(stderr, "Login Fail!\nUser: %s, Pass: %s\n", p_tStatus->user, p_tStatus->pass);
        return -1;
    }
}

// 判断当前目录与目标目录是否一致，如果不一致，进入目录
int CDDirectory(T_FTPClientStatus *p_tStatus, char *p_FTPDir)
{
    assert(p_tStatus);
    assert(p_FTPDir);
    int ftpRet;
    // 读取当前目录
    ftpRet = ftpPWD(p_tStatus->fd_cmd);
    // 网络错误
    if(ftpRet == -1)
    {
    	printf("%s_%d: CDDirectory failed!\n",__FUNCTION__,__LINE__);
        return -1;
    }
    char *currentDirectory = ftpGetPWD();
    // 读取目录错误
    if(ftpRet == 0 || *currentDirectory == '\0')
    {
    	printf("%s_%d: ftpGetPWD failed!\n",__FUNCTION__,__LINE__);
        return 0;
    }
    char tmpStr[256];
    snprintf(tmpStr, 256, "%s%s", p_tStatus->rootDir, p_FTPDir);
    // 对比当前目录与完整目标目录
    if(strcmp(currentDirectory, p_FTPDir) == 0)
    {
        // 一致，不需更改目录，返回真
        return 1;
    }
    // 不一致，需要更改目录
    // 鉴于兼容性要求，需要从根目录开始尝试进入新目录并创建目录
    // 进入根目录
    ftpRet = ftpCWD(p_tStatus->fd_cmd, p_tStatus->rootDir);
    if (ftpRet < 1)
    {
        // 进入错误
        printf("%s_%d: ftpCWD failed!\n",__FUNCTION__,__LINE__);
        return ftpRet;
    }
    else
    {
        // 如果想进入的目录为空，则进入根目录
        if(p_FTPDir[0] == '\0')
        {
            return 1;
        }
        // 复制目标目录到目录缓存，在目录缓存上进行操作
        char directoryBuffer[strlen(p_FTPDir) + 1];
        memcpy(directoryBuffer, p_FTPDir, strlen(p_FTPDir) + 1);
        char *p_start  = directoryBuffer;
        char *p_cursor = directoryBuffer;
        // 跳过路径开头的'/'
        if (p_start[0] == '/')
        {
            ++p_start;
            p_cursor = p_start;
        }
        // 如果目标是根目录，返回真
        if(p_start[0] == '\0')
        {
            return 1;
        }
        // 遍历路径字符串，逐个提取目录
        while (p_cursor != NULL )
        {
            // 找到下一个'/'位置，该位置设置为字符串结束符
            p_cursor = index(p_start, '/');
            if (p_cursor != NULL)
            {
                *p_cursor = '\0';
            }
            ftpRet = ftpCWD(p_tStatus->fd_cmd, p_start);
            // 网络错误
            if(ftpRet == -1)
            {
            	printf("%s_%d: ftpCWD failed!\n",__FUNCTION__,__LINE__);
                return -1;
            }
            // 目录无法进入
            if (ftpRet == 0)
            {
                // 尝试建立目录
                ftpRet = ftpMKD(p_tStatus->fd_cmd, p_start);
                if (ftpRet < 1)
                {
                    // 建立目录失败，返回假
                    return ftpRet;
                }
                // 进入目录
                ftpRet = ftpCWD(p_tStatus->fd_cmd, p_start);
                if (ftpRet < 1)
                {
                    // 进入失败，返回假
                    printf("%s_%d: ftpCWD failed!\n",__FUNCTION__,__LINE__);
                    return ftpRet;
                }
            }
            // 进入成功，定位到下一级目录
            p_start = p_cursor + 1;
        };
        return 1;
    }
}

// 读取目录下文件，并上传到FTP
int ftpUpload(T_FTPClientStatus *p_tStatus, char *p_UploadDir, char *p_FTPSubDir)
{
    assert(p_tStatus);
    assert(p_UploadDir);
    // 打开目录
    DIR *p_dir;
    p_dir = opendir(p_UploadDir);
    if (p_dir == NULL)
    {
        perror("Open Upload Directory");
        return 0;
    }
    // 遍历目录
    struct dirent *p_dirent;
    char fullfilename[256];
    int  uploadNumber = 0;
    int  ftpRet;
    while (1)
    {
        p_dirent = readdir(p_dir);
        // 遍历到目录尽头
        if (p_dirent == NULL)
        {
            break;
        }
        // 跳过目录"."与".."
        if(strcmp(p_dirent->d_name, ".") == 0 || strcmp(p_dirent->d_name, "..") == 0)
        {
            continue;
        }
        // 跳过其他目录
        if (p_dirent->d_type == DT_DIR)
        {
            continue;
        }
        // 建立与上传目录子目录一样的FTP目录
        if(-1 == CDDirectory(p_tStatus, p_FTPSubDir))
        {
            // 失败，设置错误码，跳出循环
            uploadNumber = -1;
			printf("%s_%d: CDDirectory failed!\n",__FUNCTION__,__LINE__);
            break;
        }
        // 设置被动模式，读取数据连接IP和端口
        ftpRet = ftpPASV(p_tStatus->fd_cmd, p_tStatus->IP_data, &p_tStatus->port_data, sizeof(p_tStatus->IP_data));
        if (ftpRet < 1)
        {
            // 失败，设置错误码，跳出循环
            uploadNumber = -1;
			printf("%s_%d: ftpPASV failed!\n",__FUNCTION__,__LINE__);
            break;
        }
        // 数据连接
        p_tStatus->fd_data = ftpSocketConnectData(p_tStatus->IP_data,
                p_tStatus->port_data);
        if (p_tStatus->fd_data == -1)
        {
            // 失败，设置错误码，跳出循环
            uploadNumber = -1;
			printf("%s_%d: ftpSocketConnectData failed!\n",__FUNCTION__,__LINE__);
            break;

        }
        // 发送文件名
        ftpRet = ftpSTOR(p_tStatus->fd_cmd, p_dirent->d_name);
        if (ftpRet < 1)
        {
        	close(p_tStatus->fd_data);
            // 失败，设置错误码，跳出循环			
            uploadNumber = -1;
			printf("%s_%d: ftpSTOR failed!\n",__FUNCTION__,__LINE__);
            break;
        }
        // 发送文件
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
            // 失败，设置错误码，跳出循环			
            uploadNumber = -1;
			printf("%s_%d: ftpSendFile failed!\n",__FUNCTION__,__LINE__);
            break;

        }
        // 发送成功，删除文件，已上传数量+1
        remove(fullfilename);
        printf("upload && remove success!\n    file: %s\n\n", fullfilename);
        ++uploadNumber;
    }
    // 关闭目录
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

