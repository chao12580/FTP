/************************************************************************
* ��Ȩ���� (C)2012, ������ͨ����������޹�˾��
* 
* �ļ����ƣ� fConfigure.c
* �ļ���ʶ�� // �����ù���ƻ���
* ����ժҪ�� ͨ�������ܵ���netSwapͨ��
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
#include <assert.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include "fConfigure.h"

// ȥ�ո񡢻س�������
void fTrim(char *p_str, int len)
{
    int i = 0;
    int n = 0;
    for (i = 0; i < len; ++i)
    {
        // �ѵ��ַ�����β
        if (p_str[i] == '\0')
        {
            break;
        }
        // �����ո񡢻س�������
        if (p_str[i] == ' ' || p_str[i] == '\r' || p_str[i] == '\n')
        {
            continue;
        }
        // �ƶ��ַ�
        p_str[n] = p_str[i];
        ++n;
    }
    p_str[n] = '\0';
}
// ��ȡftp�����ļ�
unsigned int fConfigureRead(T_FTPClientStatus *p_tFTPStatus) {
    assert(p_tFTPStatus);
    // ���ļ�
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
        // ����ע�ͺͿ���
        if ('#' == lineBuffer[0] || '\n' == lineBuffer[0])
        {
            continue;
        }
        fTrim(lineBuffer, sizeof(lineBuffer));
        // ��λ��'='�ַ�λ��
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
    // ����ϴ�Ŀ¼Ϊ��Ŀ¼������ַ���
    if(strcmp(p_tFTPStatus->localDir, "/") == 0)
    {
        p_tFTPStatus->localDir[0] = '\0';
    }
    // ȥ���ַ�����β��'/'
    cursor = strlen(p_tFTPStatus->localDir) - 1;
    if (p_tFTPStatus->localDir[cursor] == '/')
    {
        p_tFTPStatus->localDir[cursor] = '\0';
    }
}


