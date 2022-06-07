/************************************************************************
* ��Ȩ���� (C)2012, ������ͨ����������޹�˾��
* 
* �ļ����ƣ� fConfigure.h
* �ļ���ʶ�� // �����ù����ƻ���
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
#ifndef FCONFIGURE_H_
#define FCONFIGURE_H_

#include <stdint.h>
#define TRUE    1
#define FALSE   0

// ���ݽṹ����ǰFTP�ͻ���״̬������
typedef struct _T_FTPClientStatus
{
    // ����������
    int fd_cmd;
    int fd_data;
    // ��������
    char IP_cmd[16];
    char IP_data[16];
    uint16_t port_cmd;
    uint16_t port_data;
    // ��¼
    char user[64];
    char pass[64];
    // �ϴ�Ŀ¼
    char localDir[256];
    // FTP��Ŀ¼
    char rootDir[256];
	// ��Ŀ¼����ʱ��
    long dir_dueTime;
    long prog_dueTime;	
} T_FTPClientStatus;

// ��ȡ�������ļ�����
#define FTP_CONFIFURE_FILE_NAME     ("ftpConfigure.cfg")
/**************************************************************************
* �������ƣ� fConfigureRead()
* ���������� ���ļ��ж�ȡ����
* ���ʵı��� 
* �޸ĵı��� 
* ��������� 
* ��������� 
* �� �� ֵ�� �ɹ�д�뷵���棬���򷵻ؼ�
* ����˵���� 
* �޸�����    �汾��     �޸���	     �޸�����
* -----------------------------------------------
* 2013/04/25 V1.1	  ���巢      �洢��ȡ�����ṹ�����޸�
**************************************************************************/
unsigned int fConfigureRead(T_FTPClientStatus *p_tFTPStatus);
/**************************************************************************
* �������ƣ� fConfigureFormat()
* ���������� �Բ�������΢������ȡ���д��ǰ����
* ���ʵı��� 
* �޸ĵı��� 
* ��������� 
* ��������� 
* �� �� ֵ��
* ����˵���� 
* �޸�����    �汾��     �޸���	     �޸�����
* -----------------------------------------------
* 2013/04/25 V1.1	  ���巢      ɾ�����ػ�FTP�ϴ��ò����Ĳ�����������ص�
**************************************************************************/
void fConfigureFormat(T_FTPClientStatus *p_tFTPStatus);

#endif /* FCONFIGURE_H_ */