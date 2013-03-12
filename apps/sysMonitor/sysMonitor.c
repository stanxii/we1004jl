#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

#include <public.h>
#include <boardapi.h>

#include "sm2dbsMutex.h"
#include "sysMonitor.h"
#include "at91ButtonProcessor.h"
#include "sysindiProcessor.h"
#include "sysledProcessor.h"
#include "wdtProcessor.h"
#include "atmProcessor.h"

/********************************************************************************************
*	��������:sysMonitorSignalHandler
*	��������:ģ���쳣����������
*	����:frank
*	ʱ��:2010-08-13
*********************************************************************************************/
void sysMonitorSignalHandler(int n)
{
	printf("sysMonitorSignalHandler : module sysMonitor exit !\n");
	dbs_mutex_sys_log(DBS_LOG_INFO, "sysMonitorSignalHandler : module sysMonitor exit");
	destroy_systemStatusLock();
	destroy_sm2dbs();
	exit(0);
}

int main(void)
{	
	int ret = 0;
	int fd = 0;
#ifdef __AT30TK175STK__
	pthread_t thread_id[5];
#else
	pthread_t thread_id[4];
#endif		

	/*���������ݿ�ģ�黥��ͨѶ���ⲿSOCKET�ӿ�*/
	if( CMM_SUCCESS != init_sm2dbs(MID_SYSMONITOR) )
	{
		fprintf(stderr,"ERROR: sysMonitor->init_sm2dbs error, exited !\n");
		return CMM_CREATE_SOCKET_ERROR;
	}
	
	/* ��ʼ��g_systemStatus ������ʵĻ�����*/
	init_systemStatusLock();

	/* ע���쳣�˳��������*/
	signal(SIGTERM, sysMonitorSignalHandler);

	/* Waiting for all modus init */
	dbsMutexWaitModule(MF_MMEAD|MF_ALARM|MF_TM|MF_CMM|MF_REGI);
	//dbsWaitModule(MF_ALARM|MF_CMM);	

	/* ��������߳�*/
	ret = pthread_create( &thread_id[0], NULL, (void *)at91ButtonProcessor, NULL );
	if( ret == -1 )
	{
		fprintf(stderr, "Cannot create thread at91buttonProcessor\n");
		dbs_mutex_sys_log(DBS_LOG_ERR, "Cannot create thread at91buttonProcessor");
		destroy_systemStatusLock();
		destroy_sm2dbs();
		return CMM_FAILED;
	}
	
	/* ϵͳ�ƹ����߳�*/
	ret = pthread_create( &thread_id[1], NULL, (void *)sysledProcessor, NULL );
	if( ret == -1 )
	{
		fprintf(stderr, "Cannot create thread sysledProcessor\n");
		dbs_mutex_sys_log(DBS_LOG_ERR, "Cannot create thread sysledProcessor");
		destroy_systemStatusLock();
		destroy_sm2dbs();
		return CMM_FAILED;
	}

	/* WDT ����߳�*/
	ret = pthread_create( &thread_id[2], NULL, (void *)wdtProcessor, NULL );
	if( ret == -1 )
	{
		fprintf(stderr, "Cannot create thread wdtProcessor\n");
		dbs_mutex_sys_log(DBS_LOG_ERR, "Cannot create thread wdtProcessor");
		destroy_systemStatusLock();
		destroy_sm2dbs();
		return CMM_FAILED;
	}

	/* �ⲿ������Ϣ��Ӧ�߳�*/
	ret = pthread_create( &thread_id[3], NULL, (void *)sysindiProcessor, NULL );
	if( ret == -1 )
	{
		fprintf(stderr, "Cannot create thread sysindiProcessor\n");
		dbs_mutex_sys_log(DBS_LOG_ERR, "Cannot create thread sysindiProcessor");
		destroy_systemStatusLock();
		destroy_sm2dbs();
		return CMM_FAILED;
	}
	
#ifdef __AT30TK175STK__
	/* ϵͳ������ؽ���*/
	ret = pthread_create( &thread_id[4], NULL, (void *)atmProcessor, NULL );
	if( ret == -1 )
	{
		fprintf(stderr, "Cannot create thread atmProcessor\n");
		dbs_mutex_sys_log(DBS_LOG_ERR, "Cannot create thread atmProcessor");
		destroy_systemStatusLock();
		destroy_sm2dbs();
		return CMM_FAILED;
	}
#endif	

#ifdef __AT30TK175STK__
	pthread_join( thread_id[4], NULL );
#endif
	pthread_join( thread_id[3], NULL );
	pthread_join( thread_id[2], NULL );
	pthread_join( thread_id[0], NULL );
	pthread_join( thread_id[1], NULL );	
	
	printf("module sysMonitor exit !\n");
	destroy_systemStatusLock();
	destroy_sm2dbs();
	return 0;
}


