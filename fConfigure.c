/************************************************************************
* 版权所有 (C)2012, 福建视通光电网络有限公司。
* 
* 文件名称： fConfigure.c
* 文件标识： // 见配置管理计划书
* 内容摘要： 通过具名管道与netSwap通信
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
#include <assert.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include "fConfigure.h"

// 去空格、回车、换行
void fTrim(char *p_str, int len)
{
    int i = 0;
    int n = 0;
    for (i = 0; i < len; ++i)
    {
        // 已到字符串结尾
        if (p_str[i] == '\0')
        {
            break;
        }
        // 跳过空格、回车、换行
        if (p_str[i] == ' ' || p_str[i] == '\r' || p_str[i] == '\n')
        {
            continue;
        }
        // 移动字符
        p_str[n] = p_str[i];
        ++n;
    }
    p_str[n] = '\0';
}
// 读取ftp配置文件
unsigned int fConfigureRead(T_FTPClientStatus *p_tFTPStatus) {
    assert(p_tFTPStatus);
    // 打开文件
    FILE *file = fopen(FTP_CONFIFURE_FILE_NAME, "r");
    if (file == NULL )
    {
        return FALSE;
    }
    char lineBuffer[256];
    char *p_key;
    char *p_value;
    char *p_cursor = 0;
    memset(p_tFTPStatus, 0, sizeof(T_FTPClientStatus));
    while ((fgets(lineBuffer, sizeof(lineBuffer), file)) != NULL ) {
        // 跳过注释和空行
        if ('#' == lineBuffer[0] || '\n' == lineBuffer[0])
        {
            continue;
        }
        fTrim(lineBuffer, sizeof(lineBuffer));
        // 定位到'='字符位置
        p_cursor = index(lineBuffer, '=');
        if (p_cursor == NULL)
        {
            continue;
        }
        else
        {
            *p_cursor = '\0';
            p_cursor += 1;
            p_key = lineBuffer;
            p_value = p_cursor;
        }
        printf("key: %s, value: %s\n", p_key, p_value);
        if (strcmp(p_key, "user") == 0)
        {
            snprintf(p_tFTPStatus->user, 63, "%s", p_value);
        }
        else if (strcmp(p_key, "pass") == 0)
        {
            snprintf(p_tFTPStatus->pass, 63, "%s", p_value);
        }
        else if (strcmp(p_key, "IP") == 0)
        {
            snprintf(p_tFTPStatus->IP_cmd, 15, "%s", p_value);
        }
        else if (strcmp(p_key, "port") == 0)
        {
            p_tFTPStatus->port_cmd = atoi(p_value);
        }
        else if (strcmp(p_key, "localDir") == 0)
        {
            if(p_value[0] != '/')
            {
                snprintf(p_tFTPStatus->localDir, 255, "/%s", p_value);
            }
            else
            {
                snprintf(p_tFTPStatus->localDir, 255, "%s", p_value);
            }
        }
        else if (strcmp(p_key, "dir_dueTime") == 0)
        {
			p_tFTPStatus->dir_dueTime= atoi(p_value)*3600;
        }	
        else if (strcmp(p_key, "prog_dueTime") == 0)
        {
			p_tFTPStatus->prog_dueTime= atoi(p_value)*3600;
        }		
        else
        {
            printf("unknown configure!\n");
        }
    }
    fclose(file);
    fConfigureFormat(p_tFTPStatus);
    return TRUE;
}

void fConfigureFormat(T_FTPClientStatus *p_tFTPStatus)
{
    int cursor;
    if (p_tFTPStatus->port_cmd == 0)
    {
        p_tFTPStatus->port_cmd = 21;
    }
    // 如果上传目录为根目录，清空字符串
    if(strcmp(p_tFTPStatus->localDir, "/") == 0)
    {
        p_tFTPStatus->localDir[0] = '\0';
    }
    // 去掉字符串结尾的'/'
    cursor = strlen(p_tFTPStatus->localDir) - 1;
    if (p_tFTPStatus->localDir[cursor] == '/')
    {
        p_tFTPStatus->localDir[cursor] = '\0';
    }
}


