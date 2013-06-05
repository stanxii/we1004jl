/*****************************************************************************************
  �ļ����� : register.c
  �ļ����� : registerģ������ں���
  				��ģ��Ϊ��̨���̣������ڻ�ȡ������Ϣ��������
  				���ݿ⣻�������˱�Ǩʱ֪ͨ�澯����ģ�鴦��澯
  				�������Զ����ú��Զ��������̡�
  				���漰�����ֽ����������ֽ����ת��
  �޶���¼ :
           1 ���� : frank
             ���� : 2010-07-29
             ���� : �����ļ�

 *****************************************************************************************/
#include <assert.h>
#include <signal.h>
#include <fcntl.h>
#include <dbsapi.h>
#include <boardapi.h>
#include "register.h"
#include "reg_dbs.h"
#include "reg_alarm.h"
#include "reg_tm.h"
#include "reg_mmead.h"

T_UDP_SK_INFO SK_REGI;
static BBLOCK_QUEUE bblock;
T_TOPOLOGY_INFO topEntry;
uint8_t cltFlags[MAX_CLT_AMOUNT_LIMIT] = {0};
uint8_t cnuFlags[MAX_CLT_AMOUNT_LIMIT*MAX_CNU_AMOUNT_LIMIT] = {0};
int REGISTER_DEBUG_ENABLE = 0;

/********************************************************************************************
*	��������:debug_dump_msg
*	��������:������API����ʮ�����Ƶķ�ʽ�������������������
*				   �ļ�ָ��fpָ����豸�ļ�
*	����:frank
*	ʱ��:2010-08-13
*********************************************************************************************/
void debug_dump_msg(const unsigned char memory [], size_t length, FILE *fp)
{
	if(REGISTER_DEBUG_ENABLE)
	{
		debug_printf("----------------------------------------------------------------------\n");
		hexdump(memory, length, fp);
		debug_printf("\n----------------------------------------------------------------------\n");
	}
}

/********************************************************************************************
*	��������:debug_print_top
*	��������:������API����ӡ��MMEADģ���ȡ��ʵʱ������Ϣ
*	����:frank
*	ʱ��:2010-08-13
*********************************************************************************************/
void debug_print_top(T_MMEAD_TOPOLOGY *plist)
{	
	if( REGISTER_DEBUG_ENABLE )
	{
		int i = 0;	
		if( NULL == plist ) return;
		debug_printf("=============================================================================\n");
		debug_printf( "clt.Mac = [%02X:%02X:%02X:%02X:%02X:%02X], clt.NumStas = [%d], clt.DevType = [%d]\n", 
			plist->clt.Mac[0], plist->clt.Mac[1], plist->clt.Mac[2], plist->clt.Mac[3], plist->clt.Mac[4], plist->clt.Mac[5], 
			plist->clt.NumStas, plist->clt.DevType );
		if( plist->clt.NumStas > 0 )
		{
			for( i=0; i<plist->clt.NumStas; i++ )
			{
				debug_printf( "	-- cnu[%d].Mac = [%02X:%02X:%02X:%02X:%02X:%02X], TX/RX = [%d/%d], DevType = [%d]\n", 
					i, 
					plist->cnu[i].Mac[0], plist->cnu[i].Mac[1], plist->cnu[i].Mac[2], 
					plist->cnu[i].Mac[3], plist->cnu[i].Mac[4], plist->cnu[i].Mac[5], 
					plist->cnu[i].AvgPhyTx, plist->cnu[i].AvgPhyRx, plist->cnu[i].DevType );
			}
		}
		debug_printf("=============================================================================\n");
	}
}

/********************************************************************************************
*	��������:SignalProcessHandle
*	��������:ע��ģ���쳣����������
*	����:frank
*	ʱ��:2010-08-13
*********************************************************************************************/
void RegSignalProcessHandle(int n)
{
	/* ����ϵͳ�رյĸ澯*/
	cbat_system_sts_notification(0);
	/* �ر�socket�ӿ� */
	dbs_sys_log(dbsdev, DBS_LOG_INFO, "SignalProcessHandle : module register exit");
	msg_mmead_destroy();
	msg_alarm_destroy();
	msg_tm_destroy();
	reg_dbsClose();
	//printf("\nRegister exited !\n");
	exit(0);
}

BOOLEAN isCnuIndexOnUsed(uint32_t clt_index, uint32_t cnu_index)
{
	uint32_t onUsed;
	
	if( BOOL_FALSE == topEntry.tb_cnu[cnu_index-1].OnUsed )
	{
		return BOOL_FALSE;
	}
	else if( CMM_SUCCESS == db_get_user_onused(1, cnu_index, &onUsed))
	{
		if( 0 == onUsed )
		{
			return BOOL_FALSE;
		}
		else
		{
			return BOOL_TRUE;
		}
	}
	else
	{
		fprintf(stderr, "ERROR: isCnuIndexOnUsed->db_get_user_onused !\n");
		return BOOL_TRUE;
	}
}

int set_cnu_pro_sync(uint32_t clt_index, uint32_t cnu_index, BOOLEAN status)
{
	DB_INTEGER_V st_iValue;
	BOOLEAN flag = status?BOOL_TRUE:BOOL_FALSE;
	
	st_iValue.ci.tbl = DBS_SYS_TBL_ID_CNU;
	st_iValue.ci.row = (clt_index-1)*MAX_CNU_AMOUNT_LIMIT+cnu_index;
	st_iValue.ci.col = DBS_SYS_TBL_CNU_COL_ID_SYNCH;
	st_iValue.ci.colType = DBS_INTEGER;
	st_iValue.len = sizeof(uint32_t);
	st_iValue.integer = flag;
	
	if( CMM_SUCCESS != dbsUpdateInteger(dbsdev, &st_iValue))
	{
		perror("ERROR: set_cnu_pro_sync->dbsUpdateInteger !\n");
		return CMM_FAILED;
	}
	else
	{
		db_fflush();
		return CMM_SUCCESS;
	}
}

/********************************************************************************************
*	��������:find_idle
*	��������:��CNU�б��в��ҵ�һ�����е�index
*	����ֵ:�ɹ�:����һ�����õ�index, ʧ��:����0
*	����:frank
*	ʱ��:2010-07-23
*********************************************************************************************/
int find_idle()
{
	int i = 0;
	int id = 0;
	
	for( i=0; i<MAX_CNU_AMOUNT_LIMIT; i++ )
	{
		if( BOOL_FALSE == isCnuIndexOnUsed(1, i+1))
		{
			id = i+1;
			break;
		}
		usleep(2000);
	}
	return id;
}

void refresh_signon_cnu(uint32_t clt_index, uint32_t cnu_index, T_MMEAD_CNU_INFO *activeCnu)
{
	//int invalidCnuAccessEn = BOOL_FALSE;
	st_dbsCnu cnu;
	T_TOPOLOGY_INFO *this = &topEntry;

	if( CMM_SUCCESS != db_get_cnu(cnu_index, &cnu) )
	{
		perror("ERROR: refresh_signon_cnu->db_get_cnu !\n");
		return;
	}

	/*****************************************************************************************/
	/* �������Ǳ�ڵ�©��*/
	/* ���MAC��ַ��ͬ�ķǷ��û�������Ҳ�ܻ��ȥ*/
	/* ���ﲻӦ�ø����豸���ͣ�������ֳ�ͻ��澯����*/
	/*****************************************************************************************/
	if( cnu.col_model != activeCnu->DevType )
	{
		cnu.col_sts = DEV_STS_OFFLINE;
		cnu.col_rx = 0;
		cnu.col_tx = 0;
		this->tb_cnu[cnu_index-1].online = DEV_STS_OFFLINE;
		this->tb_cnu[cnu_index-1].RxRate = 0;
		this->tb_cnu[cnu_index-1].TxRate = 0;
		db_update_cnu(cnu_index, &cnu);
		/* дϵͳ��־*/
		dbs_sys_log(dbsdev, DBS_LOG_WARNING, "refresh_signon_cnu encountered cnu with conflict mac address");
		/* ���͸澯*/
		fprintf(stderr, "refresh_signon_cnu mac address conflicting !\n");
	}
	/*****************************************************************************************/
	/*****************************************************************************************/
	else
	{
		/* ������Ԫ���ݿ�*/
		cnu.id = cnu_index;
		cnu.col_sts = DEV_STS_ONLINE;
		//cnu.col_model = activeCnu->DevType;
		cnu.col_rx = activeCnu->AvgPhyRx;
		cnu.col_tx = activeCnu->AvgPhyTx;
		if( CMM_SUCCESS == db_update_cnu(cnu_index, &cnu))
		{
			/* ͬ���������ڴ����ݱ�*/
			//this->tb_cnu[cnu_index-1].DevType = activeCnu->DevType;
			this->tb_cnu[cnu_index-1].online = DEV_STS_ONLINE;
			this->tb_cnu[cnu_index-1].RxRate = activeCnu->AvgPhyRx;
			this->tb_cnu[cnu_index-1].TxRate = activeCnu->AvgPhyTx;
			/* ֪ͨ�澯����ģ��*/
			cnu_sts_transition_notification(1, cnu_index, DEV_STS_ONLINE);
		}
		else
		{
			perror("ERROR: refresh_signon_cnu->db_update_cnu !\n");
		}
		return;
	}
}

void do_cnu_auto_config(uint32_t clt_index, uint32_t cnu_index, T_MMEAD_CNU_INFO *activeCnu)
{
	uint32_t PIB_CRC = 0;
	//uint32_t MOD_CRC = 0;
	//uint32_t tid = 0;
	st_dbsCnu cnu;
	DB_INTEGER_V iValue;
	T_TOPOLOGY_INFO *this = &topEntry;

	/* ���MAC��ַ��ͬ�����豸���Ͳ�ͬ��CNU �Զ����õ�BUG */
	/* ��:�ڸ�CBAT��������MACΪ3071b2000010��WEC-3702I���룬���豸
	����֮������ͬ��MAC��ַ�������豸����ΪWEC-604���豸
	ע�ᣬ���ڴ�ʱ���豸�����ͻ�û��д�����ݿ⣬TMCoreģ��
	��Ȼ��Ϊ���豸ΪWEC-3702I��������CRC�Լ�PIBʱ����Ȼ��Ϊ
	���豸ΪWEC-3702I�������ʱ�����ɵ�PIBд���豸���������
	�豸ԭʼ�豸���͸���ΪWEC-3702I��BUG��*/
	/* ���������İ취:�·�����֮ǰ������ݿ��е��豸����
	�ֶ��Ƿ���activeCnu�ṹ�е�һ�£���һ�����·����ã�����
	һ�����͸澯����ֹ�·�����*/
	if( CMM_SUCCESS != db_get_cnu(cnu_index, &cnu) )
	{
		perror("ERROR: do_cnu_auto_config->db_get_cnu !\n");
		/* ����CNU�����Զ����õĸ澯*/
		cnu_abort_auto_config_notification(clt_index, cnu_index);
		return;
	}
	else
	{
		if( cnu.col_model != activeCnu->DevType )
		{
			/*****************************************************************************************/
			/* �������Ǳ�ڵ�©��*/
			/* ���MAC��ַ��ͬ�ķǷ��û�������Ҳ�ܻ��ȥ*/
			/*****************************************************************************************/
			#if 0
			/* ���ﻹ��Ǳ�ڵ�©��*/
			/* ���MAC��ַ��ͬ�ķǷ��û�������Ҳ�ܻ��ȥ*/
			refresh_signon_cnu(clt_index, cnu_index, activeCnu);
			/* ����CNU�����Զ����õĸ澯*/
			cnu_abort_auto_config_notification(clt_index, cnu_index);
			return;
			#endif
			
			/* ������ȷ������Ӧ����: */
			
			/* 1. �����ݿ���ڴ��и��豸��״̬����Ϊ����״̬��*/
			this->tb_cnu[cnu_index-1].online = DEV_STS_OFFLINE;
			this->tb_cnu[cnu_index-1].RxRate = 0;
			this->tb_cnu[cnu_index-1].TxRate = 0;

			cnu.col_sts = DEV_STS_OFFLINE;
			cnu.col_rx = 0;
			cnu.col_tx = 0;
			
			/* 2. ������ֵ��豸���ͺϷ���������豸�����ͣ�*/
			if( boardapi_isCnuSupported(activeCnu->DevType) )
			{
				this->tb_cnu[cnu_index-1].DevType = activeCnu->DevType;
				cnu.col_model = activeCnu->DevType;
				/* ����ٶ��豸���ͱ����һ�������·�����*/
				cnu.col_synch = 0;
				if( CMM_SUCCESS != dbsUpdateCnu(dbsdev, cnu_index, &cnu) )
				{
					perror("ERROR: do_cnu_auto_config->dbsUpdateCnu !\n");
					/* ����CNU�����Զ����õĸ澯*/
					cnu_abort_auto_config_notification(clt_index, cnu_index);
					return;
				}
				/* �����WEC701ϵ�У�����Ҫ����profile->base_pib */
				switch(activeCnu->DevType)
				{
					case WEC701_C2:
					case WEC701_C4:
					{
						iValue.ci.tbl = DBS_SYS_TBL_ID_CNUPRO;
						iValue.ci.row = cnu_index;
						iValue.ci.col = DBS_SYS_TBL_PROFILE_COL_ID_BASE;
						iValue.ci.colType = DBS_INTEGER;
						iValue.integer = 12;
						iValue.len = sizeof(int32_t);
						if( CMM_SUCCESS != dbsUpdateInteger(dbsdev, &iValue) )
						{
							perror("ERROR: do_cnu_auto_config->dbsUpdateInteger !\n");
							/* ����CNU�����Զ����õĸ澯*/
							cnu_abort_auto_config_notification(clt_index, cnu_index);
							return;
						}
						break;
					}
					default:
						break;
				}
				printf("Warnning: fixed non-matched cnu type %d at index %d/%d\n", 
					activeCnu->DevType, clt_index, cnu_index);
				/* дϵͳ��־*/
				dbs_sys_log(dbsdev, DBS_LOG_WARNING, "do_cnu_auto_config encountered cnu with conflict mac address");
				/* ��������������Զ���������*/
			}			
			/* 3. ������ֵ��豸���Ͳ��Ϸ��������ߣ�*/
			else
			{
				/* ���ͷǷ��豸����澯*/
				/* ֪ͨ�澯����ģ��*/
				lllegal_cnu_register_notification(1, activeCnu->Mac);
				#if 0
				/* ��ֹ��Ӹ��豸*/				
				if( CMM_SUCCESS != msg_reg_mmead_bootout_dev(this->tb_clt.Mac, activeCnu->Mac) )
				{					
					lllegal_cnu_kick_off_notification(1, activeCnu->Mac, BOOL_FALSE);
				}
				else
				{
					lllegal_cnu_kick_off_notification(1, activeCnu->Mac, BOOL_TRUE);
				}
				#endif
				/* дϵͳ��־*/
				dbs_sys_log(dbsdev, DBS_LOG_WARNING, "do_cnu_auto_config encountered invalid cnu with conflict mac address");
				/* �˳��Զ���������*/
				return;
			}
			/*****************************************************************************************/
			/*****************************************************************************************/
		}
	}

	if( !boardapi_isCnuSupported(activeCnu->DevType) )
	{
		/* �������͵�CNU���������κ��Զ�����*/
		refresh_signon_cnu(clt_index, cnu_index, activeCnu);
		/* ����CNU�����Զ����õĸ澯*/
		cnu_abort_auto_config_notification(clt_index, cnu_index);
		return;
	}
	
	/* ��ģ�����ģ���ȡ���û���PIB CRC */
	if( CMM_SUCCESS != tm_get_user_pib_crc(clt_index, cnu_index, &PIB_CRC) )
	{
		perror("ERROR: do_cnu_auto_config->tm_get_user_pib_crc !\n");
		/* ����CNU�����Զ����õĸ澯*/
		cnu_abort_auto_config_notification(clt_index, cnu_index);
		return;
	}

	/* for debug */
	//printf("\ndo_cnu_auto_config(): PIB_CRC = 0x%X, activeCnu->CRC = 0x%X\n", PIB_CRC, activeCnu->CRC[0]);
	
	/* �Ƚ�PIB CRC У����*/
	if( PIB_CRC == activeCnu->CRC[0] )
	//if(1)	/* PIB CRC ������ȷ��ȡ�������θöδ���*/
	{
		#if 0
		/* ��ģ�����ģ���ȡ���û���MOD CRC */
		if( CMM_SUCCESS != tm_get_user_mod_crc(clt_index, cnu_index, &MOD_CRC) )
		{
			perror("ERROR: do_cnu_auto_config->tm_get_user_mod_crc !\n");
			return;
		}
		
		/* �Ƚ�MOD CRC У����*/
		if( MOD_CRC == activeCnu->CRC[1] )
		#endif
		
		if( BOOL_TRUE == cnu.col_synch )
		{
			refresh_signon_cnu(clt_index, cnu_index, activeCnu);
		}
		else
		{
			/* ֪ͨģ�����ģ���Զ����ɸ��û���MOD */
			if( CMM_SUCCESS != tm_gen_user_mod(clt_index, cnu_index) )
			{
				perror("ERROR: do_cnu_auto_config->tm_gen_user_mod !\n");
				/* ����CNU�����Զ����õĸ澯*/
				cnu_abort_auto_config_notification(clt_index, cnu_index);
				return;
			}
			/* �Զ��·�MOD */
			//if( CMM_SUCCESS != tm_get_cnu_tid(clt_index, cnu_index, &tid) )
			//{
			//	perror("ERROR: do_cnu_auto_config->tm_get_cnu_tid !\n");
			//	return;
			//}
			//printf("auto send mod(pro %d) configuration to clt %d cnu %d\n", tid, clt_index, cnu_index);
			//printf("\r\n  auto send mod configuration to clt %d cnu %d\n", clt_index, cnu_index);
			if( CMM_SUCCESS == msg_reg_mmead_wr_user_mod(activeCnu->DevType, activeCnu->Mac) )
			{
				/* ���͸澯*/
				cnu_auto_config_notification(clt_index, cnu_index, cnu_index, 1, BOOL_TRUE);
				/* ����־λcsyncStatus ��1 */
				set_cnu_pro_sync(clt_index, cnu_index, BOOL_TRUE);
			}
			else
			{
				/* ���͸澯*/
				cnu_auto_config_notification(clt_index, cnu_index, cnu_index, 1, BOOL_FALSE);
			}
			/* ֪ͨģ�����ģ�����ٸ��û�������*/
			if( CMM_SUCCESS != tm_distroy_user_mod(clt_index, cnu_index) )
			{
				perror("ERROR: do_cnu_auto_config->tm_distroy_user_mod !\n");
				return;
			}			
		}
	}
	else
	{
		/* ֪ͨģ�����ģ���Զ����ɸ��û���PIB */
		if( CMM_SUCCESS != tm_gen_user_pib(clt_index, cnu_index) )
		{
			perror("ERROR: do_cnu_auto_config->tm_gen_user_pib !\n");
			return;
		}
		/* �Զ��·�PIB */
		//if( CMM_SUCCESS != tm_get_cnu_tid(clt_index, cnu_index, &tid) )
		//{
		//	perror("ERROR: do_cnu_auto_config->tm_get_cnu_tid !\n");
		//	return;
		//}
		//printf("auto send pib(pro %d) configuration to clt %d cnu %d\n", tid, clt_index, cnu_index);
		//printf("\r\n  auto send pib configuration to clt %d cnu %d\n", clt_index, cnu_index);
		if( CMM_SUCCESS == msg_reg_mmead_wr_user_pib(activeCnu->DevType, activeCnu->Mac) )
		{
			/* ���͸澯*/
			cnu_auto_config_notification(clt_index, cnu_index, cnu_index, 0, BOOL_TRUE);
		}
		else
		{
			/* ���͸澯*/
			cnu_auto_config_notification(clt_index, cnu_index, cnu_index, 0, BOOL_FALSE);
		}
		/* ֪ͨģ�����ģ�����ٸ��û�������*/
		if( CMM_SUCCESS != tm_distroy_user_pib(clt_index, cnu_index) )
		{
			perror("ERROR: do_cnu_auto_config->tm_distroy_user_pib !\n");
			return;
		}		
	}
}

void refresh_active_cnu(uint32_t clt_index, uint32_t cnu_index, T_MMEAD_CNU_INFO activeCnu)
{
	st_dbsCnu cnu;
	T_TOPOLOGY_INFO *this = &topEntry;
	
	if( (this->tb_cnu[cnu_index-1].RxRate != activeCnu.AvgPhyRx) || 
	     (this->tb_cnu[cnu_index-1].TxRate != activeCnu.AvgPhyTx))
	{
		db_get_cnu(cnu_index, &cnu);
		
		cnu.id = cnu_index;
		cnu.col_rx = activeCnu.AvgPhyRx;
		cnu.col_tx = activeCnu.AvgPhyTx;
		if( CMM_SUCCESS == db_update_cnu(cnu_index, &cnu))
		{
			/* ͬ������*/
			this->tb_cnu[cnu_index-1].RxRate = activeCnu.AvgPhyRx;
			this->tb_cnu[cnu_index-1].TxRate = activeCnu.AvgPhyTx;
		}		
	}

	/* ���CNU���ٰ���Դ���µ�BUG */
	//if( !isCnuAuthorized(clt_index, cnu_index) )
	//{
	//	msg_reg_mmead_block_user(activeCnu.DevType, activeCnu.Mac);
	//}	
	
}

/********************************************************************************************
*	��������:do_clt_register
*	��������:CLTע�ắ�����ú������CLT����ʱ��ע�����
*					1. ���CLT��Ϣ�����ݿ��е�ע��;
*					2. ��ע���ȡ��ʵʱ��Ϣͬ�����ڴ�;
*					3. ��ɸ澯֪ͨ��
*					4. ����Զ����ã�
*	ע��:���øú�������ע����豸һ�������ݿ���Ԫ����
*			�Ѿ����ڵ��豸�������·��ֵ��豸�ڵ��øú���
*			����ע��ʱ����Ҫ�Ƚ��豸�ı�ʶ��Ϣд�����ݿ�
*	����:frank
*	ʱ��:2010-08-13
*********************************************************************************************/
void do_clt_register(uint32_t clt_index, T_MMEAD_CLT_INFO activeClt)
{
	st_dbsClt clt;
	T_TOPOLOGY_INFO *this = &topEntry;

	db_get_clt(clt_index, &clt);

	if( DEV_STS_OFFLINE == this->tb_clt.online )
	{
		clt.id = clt_index;
		clt.col_model = activeClt.DevType;
		clt.col_sts = DEV_STS_ONLINE;
		clt.col_numStas = activeClt.NumStas;		
		
		if( CMM_SUCCESS == db_update_clt(1, &clt))
		{
			/* ͬ������*/
			this->tb_clt.online = DEV_STS_ONLINE;
			this->tb_clt.DevType = activeClt.DevType;
			this->tb_clt.NumStas = activeClt.NumStas;
			/* ֪ͨ�澯����ģ��*/
			clt_sts_transition_notification(1, DEV_STS_ONLINE);			
			/* ֪ͨ�Զ�����ģ��*/
			/* ֪ͨ�Զ�����ģ��*/
			/* ����ȱʧ*//* �ݲ�֧��CLT�Զ�����*/
		}
		else
		{
			perror("ERROR: do_clt_register->db_update_clt !\n");
		}
	}
	else
	{
		/* ������Ԫ���ݱ��е�NumStas �ֶ�*/
		if( this->tb_clt.NumStas != activeClt.NumStas )
		{
			clt.id = clt_index;
			clt.col_sts = DEV_STS_ONLINE;
			clt.col_numStas = activeClt.NumStas;
			if( CMM_SUCCESS == db_update_clt(1, &clt))
			{
				/* ͬ������*/
				this->tb_clt.NumStas = activeClt.NumStas;
			}
		}
	}
}

/********************************************************************************************
*	��������:do_clt_unregister
*	��������:CLT��ע�ắ�����ú������CLT����ʱ�ķ�ע�����
*					1. ���CLT��Ϣ�����ݿ��еķ�ע��;
*					2. ����ע����Ϣͬ�����ڴ�;
*					3. ��ɸ澯֪ͨ��
*	ע��:���øú������з�ע����豸һ�������ݿ���Ԫ����
*			�Ѿ����ڵ��豸
*	����:frank
*	ʱ��:2010-08-13
*********************************************************************************************/
void do_clt_unregister(uint32_t clt_index)
{
	T_TOPOLOGY_INFO *this = &topEntry;
	
	if( DEV_STS_OFFLINE != this->tb_clt.online )
	{
		/* д���ݿ�*/
		if( CMM_SUCCESS == db_unregister_clt(1) )
		{
			/* ͬ������*/
			this->tb_clt.online = DEV_STS_OFFLINE;
			this->tb_clt.NumStas = 0;
			/* ֪ͨ�澯����ģ��*/
			clt_sts_transition_notification(1, DEV_STS_OFFLINE);
		}
		else
		{
			perror("ERROR: do_clt_unregister->db_unregister_clt !\n");
		}
	}
}

/********************************************************************************************
*	��������:do_cnu_register
*	��������:CNUע�ắ�����ú������CNU����ʱ��ע�����
*					1. ���CNU��Ϣ�����ݿ��е�ע��;
*					2. ��ע���ȡ��ʵʱ��Ϣͬ�����ڴ�;
*					3. ��ɸ澯֪ͨ��
*					4. ����Զ����ã�
*	ע��:���øú�������ע����豸һ�������ݿ���Ԫ����
*			�Ѿ����ڵ��豸�������·��ֵ��豸�ڵ��øú���
*			����ע��ʱ����Ҫ�Ƚ��豸�ı�ʶ��Ϣд�����ݿ�
*	����:frank
*	ʱ��:2010-08-13
*********************************************************************************************/
void do_cnu_register(uint32_t clt_index, uint32_t cnu_index, T_MMEAD_CNU_INFO activeCnu)
{
	int autoCfgSts = 0;
	//uint32_t userType = 0;
	//st_dbsCnu cnu;
	T_TOPOLOGY_INFO *this = &topEntry;
	
	/* ���øú�������ע���CNUһ���ǺϷ����豸����Ϊ�Ƿ�
	** CNU��ע��֮ǰ�ͽ�ֹ������û����������ᴥ��ע�����*/

	/* ������ⲿ����ע������*/
	if( REG_CNURESET == cnuFlags[cnu_index-1] )
	{
		//printf("\r\n  register event call : reset clt %d cnu %d\n", clt_index, cnu_index);
		
		/* ����MME����CNU*/
		msg_reg_mmead_reset_cnu(activeCnu.DevType, activeCnu.Mac);

		/* ����CNUǿ������*/
		do_cnu_unregister(clt_index, cnu_index);
		
		//��ԭ��־λ
		cnuFlags[cnu_index-1] = 0;

		return;
	}
	else if( REG_CNU_FORCE_REGISTRATION == cnuFlags[cnu_index-1] )
	{
		//printf("\r\n  register event call : force clt %d cnu %d re-registration\n", clt_index, cnu_index);
		
		/* ����CNUǿ������ע��ĸ澯*/
		cnu_re_register_notification(clt_index, cnu_index);
		
		/* ��ȡ��CNU����Ȩ״̬*/
		//if( isCnuAuthorized(clt_index, cnu_index) )
		//{
			//����CNU�û��˿�
		//	msg_reg_mmead_enable_user(activeCnu.DevType, activeCnu.Mac);
		//}
		//else
		//{
			//�ر�CNU�û��˿�
		//	msg_reg_mmead_block_user(activeCnu.DevType, activeCnu.Mac);
		//}
		/* ����CNUǿ������*/
		do_cnu_unregister(clt_index, cnu_index);
		/* ����־λcsyncStatus ��0 */
		//set_cnu_pro_sync(clt_index, cnu_index, BOOL_FALSE);

		//��ԭ��־λ
		cnuFlags[cnu_index-1] = 0;
		
		return;
	}
	
	
	/* ������豸��״̬��Ǩ����д���*/
	if( DEV_STS_OFFLINE == this->tb_cnu[cnu_index-1].online )
	{
		#if 0
		/* ��ȡ��CNU���û�����*/
		if( CMM_SUCCESS == db_get_user_type(clt_index, cnu_index, &userType))
		{
			if( 0 == userType )
			{
				/* �����û�ע��*/
				printf("\nclt %d cnu %d do anonymous registration\n", clt_index, cnu_index);
				do_anonymous_register(clt_index, cnu_index, activeCnu);
			}
			else
			{
				/* ֪���û�ע��*/
				printf("\nclt %d cnu %d do registration\n", clt_index, cnu_index);
				do_user_register(clt_index, cnu_index, activeCnu);
			}
		}
		else
		{
			perror("ERROR: do_cnu_register->db_get_user_type !\n");
			return;
		}
		#endif
		/* ��ȡȫ���Զ�����ʹ��״̬*/
		if( CMM_SUCCESS != db_get_auto_config_sts(&autoCfgSts))
		{
			perror("ERROR: do_cnu_register->db_get_auto_config_sts !\n");
			return ;
		}
		else
		{
			if( !boardapi_isCnuSupported(activeCnu.DevType) )
			{
				cnu_abort_auto_config_notification(clt_index, cnu_index);
				refresh_signon_cnu(clt_index, cnu_index, &activeCnu);
			}			
			else if( 0 == autoCfgSts )
			{
				/* ���ﻹ��Ǳ�ڵ�©��*/
				/* ���MAC��ַ��ͬ�ķǷ��û�������Ҳ�ܻ��ȥ*/
				refresh_signon_cnu(clt_index, cnu_index, &activeCnu);
			}
			else
			{
				/* �Զ�����*/
				do_cnu_auto_config(clt_index, cnu_index, &activeCnu);
			}	
		}
	}
	else
	{
		/* ������Ԫ���ݿ��е�ʵʱ����*/
		/* ���ﻹ��Ǳ�ڵ�©��*//* ����Ҫ�����ܿ���ܴ���*/
		/* ���MAC��ַ��ͬ�ķǷ��û�������Ҳ�ܻ��ȥ*/
		refresh_active_cnu(clt_index, cnu_index, activeCnu);
	}
}

void do_cnu_delete(uint32_t clt_index, uint32_t cnu_index)
{
	T_TOPOLOGY_INFO *this = &topEntry;

	/* ͬ������*/
	this->tb_cnu[cnu_index-1].online = DEV_STS_OFFLINE;
	this->tb_cnu[cnu_index-1].RxRate = 0;
	this->tb_cnu[cnu_index-1].TxRate = 0;
	this->tb_cnu[cnu_index-1].OnUsed = 0;
	bzero(this->tb_cnu[cnu_index-1].Mac, 6);

	//db_unregister_cnu(cnu_index);	
	/* ���ݱ�����TM���Ѿ���ɾ��*/
	//db_delete_cnu(cnu_index);
	db_fflush();
}


/********************************************************************************************
*	��������:do_cnu_unregister
*	��������:CNU��ע�ắ�����ú������CNU����ʱ�ķ�ע�����
*					1. ���CNU��Ϣ�����ݿ��еķ�ע��;
*					2. ����ע����Ϣͬ�����ڴ�;
*					3. ��ɸ澯֪ͨ��
*	ע��:���øú������з�ע����豸һ�������ݿ���Ԫ����
*			�Ѿ����ڵ��豸
*	����:frank
*	ʱ��:2010-08-13
*********************************************************************************************/
void do_cnu_unregister(uint32_t clt_index, uint32_t cnu_index)
{
	T_TOPOLOGY_INFO *this = &topEntry;
	
	if( DEV_STS_OFFLINE != this->tb_cnu[cnu_index-1].online )
	{
		/* д���ݿ�*/
		if( CMM_SUCCESS == db_unregister_cnu(cnu_index) )
		{
			/* ͬ������*/
			this->tb_cnu[cnu_index-1].online = DEV_STS_OFFLINE;
			this->tb_cnu[cnu_index-1].RxRate = 0;
			this->tb_cnu[cnu_index-1].TxRate = 0;
			/* ֪ͨ�澯����ģ��*/
			cnu_sts_transition_notification(1, cnu_index, DEV_STS_OFFLINE);
		}
		else
		{
			perror("ERROR: do_cnu_unregister->db_unregister_cnu !\n");
		}
	}
}

int __isNewCnuMacaddr(uint8_t mac[])
{
	int i = 0;
	DB_TEXT_V strValue;
	uint8_t MA[6] = {0x00,0x00,0x00,0x00,0x00,0x00};
	uint8_t MB[6] = {0xff,0xff,0xff,0xff,0xff,0xff};
	uint8_t MR[6] = {0};
	
	/* ������00:00:00:00:00:00 */
	if( memcmp(mac, MA, 6) == 0 )
	{
		return 0;
	}
	/* ������FF:FF:FF:FF:FF:FF */
	if( memcmp(mac, MB, 6) == 0 )
	{
		return 0;
	}
	/* Ѱ��CNU ����ֹ�ظ�*/
	for( i=1; i<MAX_CNU_AMOUNT_LIMIT; i++ )
	{
		strValue.ci.tbl = DBS_SYS_TBL_ID_CNU;
		strValue.ci.row = i;
		strValue.ci.col = DBS_SYS_TBL_CNU_COL_ID_MAC;
		strValue.ci.colType = DBS_TEXT;
		if( CMM_SUCCESS != dbsGetText(dbsdev, &strValue) )
		{
			return 0;
		}
		else if( CMM_SUCCESS != boardapi_macs2b(strValue.text, MR))
		{
			return 0;
		}
		else if( memcmp(mac, MR, 6) == 0 )
		{
			return 0;
		}
		usleep(5000);
	}
	return 1;
}

void do_create_cnu(uint8_t macaddr[])
{
	int idle = 0;	
	st_dbsCnu cnu;
	T_TOPOLOGY_INFO *this = &topEntry;

	/* �ٶ�Ĭ������豸����*/
	//cnu.col_model = WEC_604;
	cnu.col_model = WEC701_C4;
	sprintf(cnu.col_mac, "%02X:%02X:%02X:%02X:%02X:%02X", 
		macaddr[0], macaddr[1], macaddr[2], 
		macaddr[3], macaddr[4], macaddr[5]
	);
	cnu.col_sts = 0;
	cnu.col_auth = 0;
	strcpy(cnu.col_ver, "V4.1.0.1");
	cnu.col_rx = 0;
	cnu.col_tx = 0;
	strcpy(cnu.col_snr, "0%");
	strcpy(cnu.col_bpc, "0%");
	strcpy(cnu.col_att, "0dB");
	cnu.col_synch = BOOL_FALSE;
	cnu.col_row_sts = BOOL_TRUE;

	/* ���ж�MAC ��ַ�Ƿ��ͻ*/
	if( !__isNewCnuMacaddr(macaddr) )
	{
		printf("\r\n\r\n  create entry for cnu %s failed !\n", cnu.col_mac);
		dbs_sys_log(dbsdev, DBS_LOG_ERR, "register create cnu error: mac conflict !");
		return;
	}

	/* �ж��Ƿ�ﵽ�û�����*/
	idle = find_idle();	
	if( 0 == idle )
	{		
		/* CNU ����������ֹ���*/
		printf("\r\n\r\n  create entry for cnu %s failed !\n", cnu.col_mac);
		dbs_sys_log(dbsdev, DBS_LOG_ERR, "register create cnu error: no cnu entry !");
		return;
	}
	else
	{
		/* ��Ҫ��������ݿ��Լ��ڴ���*/
		cnu.id = idle;		
		if( CMM_SUCCESS == db_new_cnu(idle, &cnu))
		{
			/* ͬ������*/
			this->tb_cnu[idle-1].DevType = WEC_604;
			memcpy((char *)(this->tb_cnu[idle-1].Mac), (const char *)(macaddr), 6);			
			this->tb_cnu[idle-1].online = 0;
			this->tb_cnu[idle-1].RxRate = 0;
			this->tb_cnu[idle-1].TxRate = 0;
			this->tb_cnu[idle-1].OnUsed = BOOL_TRUE;
			db_fflush();
			dbs_sys_log(dbsdev, DBS_LOG_INFO, "module register create entry for cnu success !");
			return;
		}
		else
		{
			printf("\r\n\r\n  create entry for cnu %s error !\n", cnu.col_mac);
			dbs_sys_log(dbsdev, DBS_LOG_ERR, "register create cnu error: system error !");
			return;
		}
	}	
}

int try_to_add_cnu(T_MMEAD_CNU_INFO activeCnu)
{
	int idle = 0;
	int invalidCnuAccessEn = BOOL_TRUE;
	uint8_t supCnuMac0[6] = {0x30, 0x71, 0xB2, 0x00, 0x00, 0x10};
	uint8_t supCnuMac1[6] = {0x00, 0x1E, 0xE3, 0x20, 0x11, 0x01};
	uint8_t MA[6] = {0x00,0x00,0x00,0x00,0x00,0x00};
	uint8_t MB[6] = {0xff,0xff,0xff,0xff,0xff,0xff};
	st_dbsCnu cnu;
	T_TOPOLOGY_INFO *this = &topEntry;

	//printf("\n@@try_to_add_cnu\n");

	/* ������00:00:00:00:00:00 */
	if( memcmp(activeCnu.Mac, MA, 6) == 0 )
	{
		return 0;
	}
	/* ������FF:FF:FF:FF:FF:FF */
	if( memcmp(activeCnu.Mac, MB, 6) == 0 )
	{
		return 0;
	}
	
	/* �ж��Ƿ�Ϊ�Ƿ��û�*/
	if( ! boardapi_isCnuSupported(activeCnu.DevType) )
	{		
		/* ���ͷǷ��豸����澯*/
		/* ֪ͨ�澯����ģ��*/
		lllegal_cnu_register_notification(1, activeCnu.Mac);		
	}

	idle = find_idle();
	
	if( 0 == idle )
	{
		/* ����CNU�û��������޵ĸ澯*/
		cnu_exceed_notification(1);
		/* ��ֹ��Ӹ��豸*/
		if( CMM_SUCCESS != msg_reg_mmead_bootout_dev(this->tb_clt.Mac, activeCnu.Mac) )
		{
			
			lllegal_cnu_kick_off_notification(1, activeCnu.Mac, BOOL_FALSE);
		}
		else
		{
			lllegal_cnu_kick_off_notification(1, activeCnu.Mac, BOOL_TRUE);
		}
	}
	else
	{
		/* �Ƚ���CNU����Ϣ�������Ԫ���ݿ�*/
		cnu.id = idle;
		cnu.col_model = activeCnu.DevType;
		sprintf(cnu.col_mac, "%02X:%02X:%02X:%02X:%02X:%02X", 
			activeCnu.Mac[0], activeCnu.Mac[1], activeCnu.Mac[2], 
			activeCnu.Mac[3], activeCnu.Mac[4], activeCnu.Mac[5]
		);
		//memcpy((char *)(cnu.mac), (const char *)(activeCnu.Mac), 6);
		cnu.col_sts = 0;		
		/* ��������ǳ����ն����Զ����Ϊ�����û�*/
		if( (memcmp(supCnuMac0, activeCnu.Mac, 6) == 0) || (memcmp(supCnuMac1, activeCnu.Mac, 6) == 0))
		{
			cnu.col_auth = 1;	
			cnu.col_synch = BOOL_FALSE;
		}
		else
		{
			cnu.col_auth = 0;
			/* ����ƻ������ûᱻ�ֶ���Ϊ�����û������ǵ�����*/
			/* �û�ϣ����A�ֵ��¿�֮ͨ��ֱ���õ�B�ֵ��¾Ϳ���ʹ��*/
			/* ����2��������������ƻ�ʹ��*/
			/* 1. �ն������ֵ�Bʱ�����һ�����û�����ʱ���ò��Ḳ��*/
			/* 2. �ն������ֵ�B�������һ��δ���������õ������û�ʱ*/
			/* ����2�������Ȼ�����ƻ������ñ����ǵ����:*/
			/* 1. �ն��ھֵ�B���Ѿ���һ�������û������ҽ��й���������*/
			/* 2. �ն��ھֵ�B���Ѿ���һ��Ԥ�������û�*/
			cnu.col_synch = BOOL_TRUE;
		}
		switch(activeCnu.DevType)
		{
			case WEC701_C2:
			case WEC701_C4:
				strcpy(cnu.col_ver, "v7.1.1-FINAL");
				break;
			default:
				strcpy(cnu.col_ver, "V4.1.0.1");
				break;
		}		
		cnu.col_rx = 0;
		cnu.col_tx = 0;
		strcpy(cnu.col_snr, "0%");
		strcpy(cnu.col_bpc, "0%");
		strcpy(cnu.col_att, "0dB");
		//cnu.col_synch = BOOL_FALSE;
		cnu.col_row_sts = BOOL_TRUE;

		if( 1 == cnu.col_auth )
		{
			if( CMM_SUCCESS == db_new_su(idle, &cnu))
			{
				/* ͬ������*/
				this->tb_cnu[idle-1].online = DEV_STS_OFFLINE;
				this->tb_cnu[idle-1].DevType = activeCnu.DevType;
				memcpy((char *)(this->tb_cnu[idle-1].Mac), (const char *)(activeCnu.Mac), 6);
				this->tb_cnu[idle-1].OnUsed = BOOL_TRUE;
				db_fflush();
			}
			else
			{
				idle = 0;
			}			
		}
		else
		{
			if( CMM_SUCCESS == db_new_cnu(idle, &cnu))
			{
				/* ͬ������*/
				this->tb_cnu[idle-1].online = DEV_STS_OFFLINE;
				this->tb_cnu[idle-1].DevType = activeCnu.DevType;
				memcpy((char *)(this->tb_cnu[idle-1].Mac), (const char *)(activeCnu.Mac), 6);
				this->tb_cnu[idle-1].OnUsed = BOOL_TRUE;
				db_fflush();
			}
			else
			{
				idle = 0;
			}
		}
	}
	return idle;
} 

/********************************************************************************************
*	��������:try_to_register_new_cun
*	��������:����·����豸��ע��
*					1. ���·��ֵ��豸������Ϣ�������Ԫ���ݿ�
*					2. ����do_cnu_register��������豸ע��
*	����ֵ:
*	����:frank
*	ʱ��:2010-07-23
*********************************************************************************************/
void try_to_register_new_cun(T_MMEAD_CNU_INFO activeCnu)
{
	int idle = 0;
	
	idle = try_to_add_cnu(activeCnu);
	if( idle )
	{
		do_cnu_register(1, idle, activeCnu);
	}
}

/********************************************************************************************
*	��������:do_clt_discorver
*	��������:
*	����ֵ:
*	����:frank
*	ʱ��:2010-07-23
*********************************************************************************************/
void do_clt_discorver(T_MMEAD_TOPOLOGY *plist)
{
	do_clt_register(1, plist->clt);
}

/********************************************************************************************
*	��������:do_cnu_dropped
*	��������:
*	����ֵ:
*	����:frank
*	ʱ��:2010-07-23
*********************************************************************************************/
void do_cnu_dropped(T_MMEAD_TOPOLOGY *plist)
{
	int i = 0;
	int j = 0;
	BOOLEAN find_cnu = BOOL_FALSE;
	BOOLEAN isCnuOnused;
	T_TOPOLOGY_INFO *this = &topEntry;
	uint8_t null_mac[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

	/* ���֮ǰ���ߵ�CNU�豸��plist���Ҳ�������˵�����豸������*/
	
	for( i=0; i<MAX_CNU_AMOUNT_LIMIT; i++ )
	{
		isCnuOnused = isCnuIndexOnUsed(1, i+1);
		/* Ѱ��ǰһ�����������ߵ�CNU�豸*/
		if( (memcmp((const char *)(this->tb_cnu[i].Mac), (const char *)null_mac, 6) != 0) 
			&& (BOOL_TRUE == isCnuOnused) 
			&& (DEV_STS_ONLINE == this->tb_cnu[i].online) )
		{
			find_cnu = BOOL_FALSE;
			for( j=0; j<plist->clt.NumStas; j++ )
			{
				if( memcmp((const char *)(plist->cnu[j].Mac), (const char *)(this->tb_cnu[i].Mac), 6) == 0 )
				{
					find_cnu = BOOL_TRUE;
					break;
				}
			}
			if( !find_cnu )
			{
				/* ��˵�����豸������*/				
				do_cnu_unregister(1, i+1);
			}
		}
		usleep(2000);
	}
}

/********************************************************************************************
*	��������:do_cnu_discorver
*	��������:
*	����ֵ:
*	����:frank
*	ʱ��:2010-07-23
*********************************************************************************************/
void do_cnu_discorver(T_MMEAD_TOPOLOGY *plist)
{
	int i = 0;
	int j = 0;	
	BOOLEAN isCnuOnused;
	BOOLEAN discover_new = BOOL_TRUE;		
	//uint8_t null_mac[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
	T_TOPOLOGY_INFO *this = &topEntry;
	
	/* �����ֵ��豸��Ϊ2������: */
	/* 1, �·��ֵ��豸
	** 2, ��ǰΪoff-line�����ڱ�Ϊonline���豸*/
	for( i=0; i<plist->clt.NumStas; i++ )
	{
		discover_new = BOOL_TRUE;
		for( j=0; j<MAX_CNU_AMOUNT_LIMIT; j++ )
		{
			if( 0 != memcmp((const char *)(plist->cnu[i].Mac), (const char *)(this->tb_cnu[j].Mac), 6) )
			{
				continue;
			}
			else
			{
				isCnuOnused = isCnuIndexOnUsed(1, j+1);
				if( BOOL_TRUE == isCnuOnused )
				{
					/* ��ʾ�ⲻ��һ���·��ֵ��豸*/
					discover_new = BOOL_FALSE;
					do_cnu_register(1, j+1, plist->cnu[i]);
					usleep(2000);
					break;
				}
			}
			#if 0
			/* ��δ���Ч�ʵͣ����Բ���select ���ݿ�һ�ξ����*/			
			usleep(2000);
			if( (memcmp((const char *)(plist->cnu[i].Mac), (const char *)(this->tb_cnu[j].Mac), 6) == 0) 
				&& (BOOL_TRUE == isCnuOnused) )
			{
				/* ��ʾ�ⲻ��һ���·��ֵ��豸*/
				discover_new = BOOL_FALSE;
				do_cnu_register(1, j+1, plist->cnu[i]);
				break;
			}
			#endif
		}

		/* ������һ����CNU�豸*/
		if( discover_new )
		{
			try_to_register_new_cun(plist->cnu[i]);
		}
	}
}

/********************************************************************************************
*	��������:pro_clt_dropped
*	��������:
*	����ֵ:
*	����:frank
*	ʱ��:2010-07-23
*********************************************************************************************/
void pro_clt_dropped(uint32_t clt_index)
{
	int i = 0;
	
	for( i=0; i<(MAX_CLT_AMOUNT_LIMIT*MAX_CNU_AMOUNT_LIMIT); i++ )
	{
		do_cnu_unregister(1, i+1);
		usleep(5000);
	}

	do_clt_unregister(1);
}

/********************************************************************************************
*	��������:pro_top_sts_transition
*	��������:
*	����ֵ:
*	����:frank
*	ʱ��:2010-07-23
*********************************************************************************************/
void pro_top_sts_transition(T_MMEAD_TOPOLOGY *plist)
{
	/* �����������,CLTһ������*/
	/* ���֮ǰCLT�����ߣ�����Ҫд����*/
	do_clt_discorver(plist);
	
	/* ���CNU��֮ǰ��״̬��Ǩ����Ҫ����Ӧ�߼�����*/
	/* �ȴ������ߵ��豸*/	
	do_cnu_dropped(plist);	

	/* �ٴ������ߵ��豸����Ϊ2��:
	�·��ֵ��豸����ǰΪoff-line�����ڱ�Ϊonline���豸*/
	do_cnu_discorver(plist);
}

/********************************************************************************************
*	��������:ProcessTopologyChange
*	��������:
*	����ֵ:
*	����:frank
*	ʱ��:2010-07-23
*********************************************************************************************/
void ProcessTopologyChange(T_MMEAD_TOPOLOGY *plist)
{
	if( NULL == plist )
	{
		/* �˷�֧������ζ��CLT�������ˣ���������������ٷ�����*/
		/* ֻ��Ҫ���������ߵ��豸д���߲����澯*/
		pro_clt_dropped(1);
		return;
	}
	else
	{
		/* ����˷�֧��ζ��CLTһ�����ߣ���CNU����������״̬���*/
		/* plist �����������ߵ��豸*/	
		debug_print_top(plist);
		pro_top_sts_transition(plist);
		return;		
	}
}

/********************************************************************************************
*	��������:ProcessRegist
*	��������:
*	����ֵ:
*	����:frank
*	ʱ��:2010-07-23
*********************************************************************************************/
void ProcessRegist(void)
{
	/*  �ڽ��з�������ʱ���߿�����ֶ�ʧ�������ڴ�����߿�
	**  ��ʧ����������������ʮ���߿���ʧ���ǲ���Ϊ�������Ķ�ʧ
	**  ���ϣ���ʱ�ٽ����߿������߼��Ĵ���*/
	int iFlag = 0;
	int cltLossTimes = 0;
	T_MMEAD_TOPOLOGY nelist;
	
	while(1)
	{		
		if(iFlag)
		{
			sleep(5);
			/* ��������*/
			reg2alarm_send_heartbeat_notification();
		}
		else
		{
			/* ע��ģ���ʼ��ʱ����������*/
			iFlag = 1;
		}
		
		sleep(REGISTER_POLL_INT);
		
		/* ��ȡ�ⲿģ�������¼�*/
		ProcessExtReq();

		/* ���REG_CLT_RESET ����λ���˴�ֻ������1��CLT��λ*/
		if( REG_CLT_RESET == cltFlags[0] )
		{
			//printf("\n-->register event call : reset clt\n");
			
			//��ԭ��־λ
			cltFlags[0] = 0;
			
			/* ����MME����CLT */
			msg_reg_mmead_reset_cnu(topEntry.tb_clt.DevType, topEntry.tb_clt.Mac);
			
			/* �豸���� */
			ProcessTopologyChange(NULL);
			
			continue;
		}
		
		/* ��MMEAD��ȡ�����豸�б�*/
		if( msg_reg_mmead_get_nelist(topEntry.tb_clt.Mac, &nelist) != CMM_SUCCESS )
		{
			/* ������ζ��CLT�������ˣ���������������ٷ�����*/
			cltLossTimes++;
			if( cltLossTimes > 10 )
			{
				perror("ProcessRegist->msg_reg_mmead_get_nelist failed. Maybe CLT is off-line\n");
				ProcessTopologyChange(NULL);
			}
			continue;
		}
		else
		{
			/*  ��ԭ������*/
			if( 0 != cltLossTimes )
			{
				/*  ����һ���߿���ʱ��ʧ�ĸ澯*/
				clt_heartbeat_loss_notification(1, cltLossTimes);
				cltLossTimes = 0;
			}
			/* ����һ�ε�������Ϣ��Ƚϣ�����״̬��Ǩ�Ľڵ�*/
			ProcessTopologyChange(&nelist);
			if( CMM_SUCCESS != db_real_fflush() )
			{
				perror("ProcessRegist->db_real_fflush !\n");
			}
		}
	}
}

int get_req_ext(void)
{
	BBLOCK_QUEUE *this = &bblock;
	T_Msg_CMM *req = NULL;
	stTmUserInfo *pIndex = NULL;
	int FromAddrSize = 0;	
	fd_set fdsr;
	int maxsock;
	struct timeval tv;

	memcpy((char *)&(this->sk), (const char *)&SK_REGI, sizeof(T_UDP_SK_INFO));
	bzero(this->b, MAX_UDP_SIZE);

	// initialize file descriptor set
	FD_ZERO(&fdsr);
	FD_SET(this->sk.sk, &fdsr);

	// timeout setting
	tv.tv_sec = 0;
	tv.tv_usec = 0;

	maxsock = this->sk.sk;
	if( select(maxsock + 1, &fdsr, NULL, NULL, &tv) > 0 )
	{
		if (FD_ISSET(this->sk.sk, &fdsr))
		{
			FromAddrSize = sizeof(this->sk.skaddr);
			this->blen = recvfrom(this->sk.sk, this->b, MAX_UDP_SIZE, 0, 
				(struct sockaddr *)&(this->sk.skaddr), &FromAddrSize);
			if(this->blen>0)
			{
				req = (T_Msg_CMM *)(this->b);
				pIndex = (stTmUserInfo *)(req->BUF);
				if(req->HEADER.usMsgType == REG_CNURESET)
				{
					//����CNUǿ��������־λ
					cnuFlags[pIndex->cnu-1] = REG_CNURESET;
					return CMM_SUCCESS;
				}
				else if( REG_CNU_FORCE_REGISTRATION == req->HEADER.usMsgType )
				{
					//����CNUǿ������ע���־λ
					cnuFlags[pIndex->cnu-1] = REG_CNU_FORCE_REGISTRATION;
					/* ����־λcsyncStatus ��0 */
					set_cnu_pro_sync(pIndex->clt, pIndex->cnu, BOOL_FALSE);
					return CMM_SUCCESS;
				}
				else if( REG_CNU_DELETE == req->HEADER.usMsgType )
				{
					//printf("\r\n  register event call : delete clt %d cnu %d\n", pIndex->clt, pIndex->cnu);
					/* ���ڴ���ɾ�����豸����Ϣ*/
					do_cnu_delete(pIndex->clt, pIndex->cnu);
					cnuFlags[pIndex->cnu-1] = 0;
					return CMM_SUCCESS;					
				}
				else if( REG_CLT_RESET == req->HEADER.usMsgType )
				{
					/* ����CLT ǿ��������־λ*/
					cltFlags[pIndex->clt-1] = REG_CLT_RESET;
					return CMM_SUCCESS;
				}
				else if( REG_CNU_CREATE == req->HEADER.usMsgType )
				{
					uint8_t new_cnu_mac[6] = {0};
					memcpy(new_cnu_mac, req->BUF, 6);
					/* ��CNU �������Ԫ���ݿ�*/
					do_create_cnu(new_cnu_mac);
					return CMM_SUCCESS;
				}
			}
		}
	}
	
	return CMM_FAILED;
}

void ProcessExtReq(void)
{
	int ret = CMM_SUCCESS;

	//��ձ�־λ
	bzero(cltFlags, sizeof(cltFlags));
	bzero(cnuFlags, sizeof(cnuFlags));
	//printf("\n======== begin ProcessExtReq ========\n");
	do
	{
		ret = get_req_ext();
	}while( CMM_SUCCESS == ret );
	//printf("\n======== end ProcessExtReq ========\n");	
}

int msg_regi_init(void)
{
	T_UDP_SK_INFO *sk = &SK_REGI;
	struct sockaddr_in server_addr;
	//int iMode = 1;

	/*����UDP SOCKET�ӿ�*/
	if( ( sk->sk = socket(PF_INET, SOCK_DGRAM, 0) ) < 0 )
	{
		return CMM_CREATE_SOCKET_ERROR;
	}	
	
	bzero((char *)&(sk->skaddr), sizeof(sk->skaddr));
	server_addr.sin_family = PF_INET;
	server_addr.sin_port = htons(REG_CORE_LISTEN_PORT);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(sk->sk, (struct sockaddr*)&server_addr, sizeof(server_addr))<0)
	{
		return CMM_CREATE_SOCKET_ERROR;
	}

	/* ����SOCKETΪ������ģʽ*/
	//fcntl(sk->sk, F_SETFL, O_NONBLOCK);
	
	return CMM_SUCCESS;
}

/********************************************************************************************
*	��������:init_nelib
*	��������:
*	����ֵ:
*	����:frank
*	ʱ��:2010-07-23
*********************************************************************************************/
int init_nelib(void)
{
	int i = 0;
	uint8_t null_mac[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
	st_dbsClt clt;
	T_TOPOLOGY_INFO *topology = &topEntry;

	bzero( topology, sizeof(T_TOPOLOGY_INFO));
	
	/* �����ݿ��н�ԭʼ��Ԫ��Ϣ��ʼ����ȫ������*/
	if( db_init_nelib(topology) != CMM_SUCCESS )
	{
		printf("init_nelib : CMM_FAILED");
		return CMM_FAILED;
	}
	else
	{
		/* ��Ԫ���ݿ��м�¼���豸��ʼ״̬Ҫȫ����ʼ��Ϊoff-line*/
		/* ��Ԫ��������ʵʱ�Ķ�̬����ҲҪ��ʼ��Ϊ0 */
		if( CMM_SUCCESS != db_init_all() )
		{
			printf("init_nelib : CMM_FAILED");
			return CMM_FAILED;
		}
		else
		{
			topology->tb_clt.online = 0;
			topology->tb_clt.NumStas = 0;
			for( i=0; i<(MAX_CLT_AMOUNT_LIMIT*MAX_CNU_AMOUNT_LIMIT); i++ )
			{
				topology->tb_cnu[i].online = DEV_STS_OFFLINE;
				topology->tb_cnu[i].RxRate = 0;
				topology->tb_cnu[i].TxRate = 0;
			}
		}
	}
	
	/* ���Ǽ���ֻ��1��CLT�߿�����CBAT����֮��CLTһ�����ߣ�
	����ϵͳ��һ������ʱCBAT����֪��CLT��MAC����ʱϵͳ
	��Ҫ��ȡCLT��MAC��д�����ݿ⣬�´������Ͳ����ٴ�
	��ȡCLT��MAC��ַ��ֻ��Ҫ�����ݿ�ȡ����*/
	if( memcmp(topology->tb_clt.Mac, null_mac, 6) == 0 )
	{
		printf("no clt detected in dbs, try auto scanning......\n");
		/* ��ȡCLT��MAC��ַ��д�����ݿ�*/
		if( msg_reg_mmead_get_clt(&(topology->tb_clt)) != CMM_SUCCESS )
		{
			/* ���Ͳ��ܷ����߿����쳣�澯*/
			clt_cannot_finded_notification(1);
			printf("init_nelib : CMM_FAILED");
			return CMM_FAILED;
		}
		else
		{
			printf("register: [discovered clt for the first time]\n");
			/* �����ﲢ�����豸���ߵĴ���*/
			/* ����ֻ�ǽ�CLT��MAC��ַд�����ݿ�*/
			clt.id = 1;
			clt.col_model = topology->tb_clt.DevType;
			sprintf(clt.col_mac, "%02X:%02X:%02X:%02X:%02X:%02X", 
				topology->tb_clt.Mac[0], topology->tb_clt.Mac[1], topology->tb_clt.Mac[2], 
				topology->tb_clt.Mac[3], topology->tb_clt.Mac[4], topology->tb_clt.Mac[5]
			);
			clt.col_sts = DEV_STS_OFFLINE;
			clt.col_maxStas = MAX_CNU_AMOUNT_LIMIT;			
			clt.col_numStas = 0;
			strcpy(clt.col_swVersion, "v7.1.1-FINAL");
			clt.col_synch = 0;
			clt.col_row_sts = 1;				
			if( CMM_SUCCESS != db_update_clt(1, &clt))
			{
				printf("init_nelib : CMM_FAILED");
				return CMM_FAILED;
			}
			else
			{
				db_fflush();
			}
		}
	}	
	return CMM_SUCCESS;
}

/********************************************************************************************
*	��������:main
*	��������:
*	����ֵ:
*	����:frank
*	ʱ��:2010-07-23
*********************************************************************************************/
int main(void)
{	
	/*���������ݿ�ģ��ͨѶ���ⲿSOCKET�ӿ�*/
	dbsdev = reg_dbsOpen();
	if( NULL == dbsdev )
	{
		fprintf(stderr,"ERROR: register->dbsOpen error, exited !\n");
		return CMM_CREATE_SOCKET_ERROR;
	}
	
	/* Waiting for mmead init */
	dbsWaitModule(dbsdev, MF_CMM|MF_MMEAD|MF_ALARM|MF_TM);
	
	/*������MMEADģ��ͨѶ���ⲿSOCKET�ӿ�*/
	if( CMM_SUCCESS != msg_mmead_init() )
	{
		perror("Register->msg_mmead_init error, exited !\n");
		dbs_sys_log(dbsdev, DBS_LOG_EMERG, "module register msg_mmead_init error, exited !");
		reg_dbsClose();
		return CMM_CREATE_SOCKET_ERROR;
	}

	/*������澯ģ��ͨѶ���ⲿSOCKET�ӿ�*/
	if( CMM_SUCCESS != msg_alarm_init() )
	{
		perror("Register->msg_alarm_init error, exited !\n");
		dbs_sys_log(dbsdev, DBS_LOG_EMERG, "module register msg_alarm_init error, exited !");
		reg_dbsClose();
		return CMM_CREATE_SOCKET_ERROR;
	}	

	/*������ģ�����ģ��ͨѶ���ⲿSOCKET�ӿ�*/
	if( CMM_SUCCESS != msg_tm_init() )
	{
		perror("Register->msg_tm_init error, exited !\n");
		dbs_sys_log(dbsdev, DBS_LOG_EMERG, "module register msg_tm_init error, exited !");
		reg_dbsClose();
		return CMM_CREATE_SOCKET_ERROR;
	}

	/* ���������ⲿ�����SOCKET�ӿ�*/
	if( CMM_SUCCESS != msg_regi_init() )
	{
		perror("Register->msg_regi_init error, exited !\n");
		dbs_sys_log(dbsdev, DBS_LOG_EMERG, "module register msg_regi_init error, exited !");
		reg_dbsClose();
		return CMM_CREATE_SOCKET_ERROR;
	}

	/* step 1:ģ������ʱ��ʼ������ȡDB��ԭʼ������Ϣ*/
	if( init_nelib() != CMM_SUCCESS )
	{
		perror("Register->init_nelib error, exited !\n");
		dbs_sys_log(dbsdev, DBS_LOG_EMERG, "module register init_nelib error, exited !");
		reg_dbsClose();
		return CMM_FAILED;
	}
	
	/* ע���쳣�˳��������*/
	signal(SIGTERM, RegSignalProcessHandle);	

	/* ����ϵͳ�����ĸ澯*/
	cbat_system_sts_notification(1);

	fprintf(stderr, "Starting module Register	......		[OK]\n\n");
	fprintf(stderr, "====================================================================\n");
	fprintf(stderr, "				SUCCESS\n");
	fprintf(stderr, "====================================================================\n# \n# \n# ");

	dbs_sys_log(dbsdev, DBS_LOG_INFO, "starting module register success");
	
	/* ѭ������ע���¼�*/
	ProcessRegist();

	/* ��Ҫ�����������Ӵ��룬ִ�в�����*/
	dbs_sys_log(dbsdev, DBS_LOG_INFO, "SignalProcessHandle : module register exit");
	msg_mmead_destroy();
	msg_alarm_destroy();
	msg_tm_destroy();
	reg_dbsClose();
	
	return 0;
}

