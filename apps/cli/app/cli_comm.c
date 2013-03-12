/**********************************************************************
 Copyright (c), 1991-2007, Prevail ELECTRONICS(SHENTHEN) Co., Ltd.
 �ļ����� : cli_comm.c
 �ļ����� : �����н��̵�ͨ��ģ�鴦��


 �����б� :


 �޶���¼ :
          1 ���� : frank
            ���� : 2007-11-6
            ���� :

**********************************************************************/

#include "../cli_pub.h"
#include "cli_cmm.h"
#include <dbsapi.h>

ULONG CLI_CommDestroy(void)
{
	dbs_sys_log(DBS_LOG_INFO, "module cli exited");
	destroy_cli_cmm();
	dbsClose();
	return CMM_SUCCESS;
}

/*=========================================================================*/
/*  ��������: CLI_CommInit                                                 */
/*  ��������: ������ͨ�ų�ʼ��                                             */
/*  �������:                                                              */
/*  ��  ��  : �ɹ���ʧ��                                                   */
/*  ��  ��  : frank / 2007-11-6                                         */
/*=========================================================================*/
ULONG CLI_CommInit()
{
	/* �����ݿ�ģ��ӿ�*/
	if( CMM_SUCCESS != dbsNoWaitOpen(MID_CLI) )
	{
		fprintf(stderr, "ERROR: CLI_CommInit->dbsNoWaitOpen failed !\n");
		return CMM_FAILED;
	}
	/* ��Ϣģ���ע�� */
	if( init_cli_cmm() != CMM_SUCCESS )  /* ͨѶ��ʼ��ʧ�� */
	{
		dbs_sys_log(DBS_LOG_EMERG, "module cli init_cli_cmm error, exited !");
		fprintf(stderr, "ERROR: CLI_CommInit->init_cli_cmm failed !\n");
		dbsClose();
		return CMM_FAILED;
	}
	dbs_sys_log(DBS_LOG_INFO, "starting module cli success");
	return CMM_SUCCESS;
}

