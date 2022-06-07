/************************************************************************
* ��Ȩ���� (C)2012, ������ͨ����������޹�˾��
* 
* �ļ����ƣ� fSocket.h
* �ļ���ʶ�� // �����ù����ƻ���
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
#ifndef FSOCKET_H_
#define FSOCKET_H_

#include <stdint.h>
#include <stdbool.h>
#include <sys/socket.h>
/**************************************************************************
* �������ƣ� select()
* ���������� �ж��ļ���������д״̬
* ���ʵı���
* �޸ĵı���
* ���������
* ���������
* �� �� ֵ�� ���ļ���������׼���ã������棻���򷵻ؼ�
* ����˵���� ��������
* �޸�����    �汾��     �޸���        �޸�����
* -----------------------------------------------
* 2013/03/27         V1.0       ���巢
**************************************************************************/
#define FSELECT_READ    0x01
#define FSELECT_WRITE   0x02
#define FSELECT_ERROR   0x03
#define FSELECT_SEND    FSELECT_WRITE
#define FSELECT_RECV    FSELECT_READ
static inline bool fSelect(int FD, unsigned int mWaitTime, char selectType)
{
    fd_set set;
    struct timeval tTimeout;
    tTimeout.tv_sec = mWaitTime / 1000;
    tTimeout.tv_usec = (mWaitTime % 1000) * 1000;

    FD_ZERO(&set);
    FD_SET(FD, &set);
    int ret;
    switch (selectType)
    {
    case FSELECT_READ:
        ret = select(FD + 1, &set, NULL, NULL, &tTimeout);
        break;
    case FSELECT_WRITE:
        ret = select(FD + 1, NULL, &set, NULL, &tTimeout);
        break;
    case FSELECT_ERROR:
    default:
        ret = select(FD + 1, NULL, NULL, &set, &tTimeout);
        break;
    }
    if (-1 == ret)
    {
        return false;
    }
    return ret;
}
/**************************************************************************
* �������ƣ� fSocketServer()
* ���������� �����������ӷ���ˣ�������������IP��port�˿�
* ���ʵı��� 
* �޸ĵı��� 
* ��������� 
* ��������� 
* �� �� ֵ�� ���ش򿪵�����������
* ����˵���� 
* �޸�����    �汾��     �޸���	     �޸�����
* -----------------------------------------------
* 2013/03/27	     V1.0	    ���巢	      
**************************************************************************/
int fSocketServer(uint16_t port);
/**************************************************************************
* �������ƣ� fSocketClient()
* ���������� ���ӵ�Զ��
* ���ʵı��� 
* �޸ĵı��� 
* ��������� 
* ��������� 
* �� �� ֵ�� ���ش򿪵�����������
* ����˵���� 
* �޸�����    �汾��     �޸���	     �޸�����
* -----------------------------------------------
* 2013/03/27	     V1.0	    ���巢	      
**************************************************************************/
int fSocketClient(char *IP, uint16_t port);
/**************************************************************************
* �������ƣ� fSocketAccept()
* ���������� �ȴ��ͻ�������
* ���ʵı��� 
* �޸ĵı��� 
* ��������� 
* ��������� 
* �� �� ֵ�� ���ش򿪵�����������
* ����˵���� 
* �޸�����    �汾��     �޸���	     �޸�����
* -----------------------------------------------
* 2013/03/27	     V1.0	    ���巢	      
**************************************************************************/
int fSocketAccept(int fd, struct sockaddr *clientin);
/**************************************************************************
* �������ƣ� fSocketClean()
* ���������� �ر���������
* ���ʵı��� 
* �޸ĵı��� 
* ��������� 
* ��������� 
* �� �� ֵ�� 
* ����˵���� 
* �޸�����    �汾��     �޸���	     �޸�����
* -----------------------------------------------
* 2013/03/27	     V1.0	    ���巢	      
**************************************************************************/
void fSocketClean(int *fd);
/**************************************************************************
* �������ƣ� fSocketSend()
* ���������� ��������
* ���ʵı��� 
* �޸ĵı��� 
* ��������� 
* ��������� 
* �� �� ֵ�� ���ط��͵����ݳ��ȣ�ʧ�ܷ���-1
* ����˵���� 
* �޸�����    �汾��     �޸���	     �޸�����
* -----------------------------------------------
* 2013/03/27	     V1.0	    ���巢	      
**************************************************************************/
int fSocketSend(int fd, char *buffer, size_t size);
/**************************************************************************
* �������ƣ� fSocketRecv()
* ���������� �������ݣ�mTimeWaitΪ�ȴ�ʱ�䣬���mTimeWait
* ���ʵı��� 
* �޸ĵı��� 
* ��������� 
* ��������� 
* �� �� ֵ�� ���ؽ��յ����ݳ��ȣ�ʧ�ܻ򳬹��ȴ�ʱ�䷵��-1
* ����˵���� 
* �޸�����    �汾��     �޸���	     �޸�����
* -----------------------------------------------
* 2013/03/27	     V1.0	    ���巢	      
**************************************************************************/
int fSocketRecv(int fd, char *buffer, size_t size, int mTimeWait);
/**************************************************************************
* �������ƣ� getLocalIP()
* ���������� ��ȡ����IP
* ���ʵı��� 
* �޸ĵı��� 
* ��������� 
* ��������� 
* �� �� ֵ�� ���ر�����ַ�ַ���
* ����˵���� 
* �޸�����    �汾��     �޸���	     �޸�����
* -----------------------------------------------
* 2013/03/27	     V1.0	    ���巢	      
**************************************************************************/
char* getLocalIP();

#endif /* FSOCKET_H_ */