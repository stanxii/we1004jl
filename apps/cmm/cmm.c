/*****************************************************************************************
  �ļ����� : cmm.c
  �ļ����� : cmmģ������ں���
  				��Ҫ��������UI��ϵͳ�ڲ���ģ�������Ϊ�˼���
  				�ƣ�CMM�����Ϊ�����̵��̣߳���������ǰһ������
  				�����������ű�ִ�У�������Щ��Ҫ��ʱ�ϳ��Ĳ�
  				�������������󣩣�CMM��Ҫ�������أ�Ȼ������
  				����̨����ȥ�����ⲿģ��ͨ������Ĳ�ѯ������
  				��ȡ�����Ĵ���״̬��������־�ļ�¼��CMMģ���н��С�
  �޶���¼ :
           1 ���� : frank
             ���� : 2010-08-13
             ���� : �����ļ�

 *****************************************************************************************/
#include <assert.h>
#include <signal.h>
#include <net/route.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/mii.h>
#include <linux/sem.h>
#include <dirent.h>
#include <fcntl.h>
#include <dbsapi.h>
#include <boardapi.h>
//#include <mtd-abi.h>
#include "cmm.h"
#include "cmm_tm.h"
#include "cmm_mmead.h"
#include "cmm_reg.h"
#include "cmm_alarm.h"
#include "cmm_dsdt.h"
#include "upgrade.h"
#include "at30tk175stk/at30ts75.h"

int CMM_MODULE_DEBUG_ENABLE = 0;

T_UDP_SK_INFO SK_CMM;	/* �������ⲿ������׽���*/

static BBLOCK_QUEUE bblock;

/* ��DBS  ͨѶ���豸�ļ�*/
T_DBS_DEV_INFO *dbsdev = NULL;

int __findCnuEntry()
{
	int i = 0;
	DB_INTEGER_V iValue;

	for( i=1; i<=MAX_CNU_AMOUNT_LIMIT; i++ )
	{
		iValue.ci.tbl = DBS_SYS_TBL_ID_CNU;
		iValue.ci.row = i;
		iValue.ci.col = DBS_SYS_TBL_CNU_COL_ID_ROWSTS;
		iValue.ci.colType = DBS_INTEGER;
		if( CMM_SUCCESS != dbsGetInteger(dbsdev, &iValue))
		{
			return 0;
		}
		else if( 0 == iValue.integer )
		{
			return 1;
		}
	}
	return 0;
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
	}
	return 1;
}

/********************************************************************************************
*	��������:do_network_config
*	��������:���ñ���ip ,����,��������
*	����:may2250
*	ʱ��:2010-09-13
*********************************************************************************************/
int do_network_config(st_dbsNetwork networkinfo)
{
	int fd;
	int I_ip ,I_mask, I_gw;
	struct ifreq ifr; 
	struct sockaddr_in *sin;
	struct rtentry  rt;
	char *ifname="eth0";
    
	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if(fd < 0)
	{
		perror("do_network_config socket error !\n");     
		return CMM_FAILED;
	}
    	memset(&ifr,0,sizeof(ifr)); 
    	strcpy(ifr.ifr_name, "eth0"); 
	sin = (struct sockaddr_in*)&ifr.ifr_addr;     
	sin->sin_family = AF_INET;     
 
    	if(inet_aton(networkinfo.col_ip, &(sin->sin_addr)) < 0)   
	{     
		perror("do_network_config->inet_aton error !\n");
		close(fd);
		return CMM_FAILED;     
	}    
    
	if(ioctl(fd,SIOCSIFADDR,&ifr) < 0)   
	{     
		perror("do_network_config ioctl error !\n");
		close(fd);
		return CMM_FAILED;     
	}
   	 //netmask
	if(inet_aton(networkinfo.col_netmask, &(sin->sin_addr)) < 0)   
	{     
		perror("inet_pton   error");
		close(fd);
		return CMM_FAILED;     
	}    
	if(ioctl(fd, SIOCSIFNETMASK, &ifr) < 0)
	{
		perror("do_network_config ioctl error !\n");
		close(fd);
		return CMM_FAILED; 
	}
/*****************************************************/
/* ���IP�����ز���ͬһ�������ڣ��˶�
** ����ᵼ���豸�����쳣����ʱ����*/
/*****************************************************/
#if 1
   	 //gateway
    	memset(&rt, 0, sizeof(struct rtentry));
    	memset(sin, 0, sizeof(struct sockaddr_in));
    	sin->sin_family = AF_INET;
    	sin->sin_port = 0;
		
	if(strlen(networkinfo.col_gw) == 0)
	{
		close(fd);
		return CMM_SUCCESS;
	}		

	I_ip = inet_addr(networkinfo.col_ip);
	I_mask = inet_addr(networkinfo.col_netmask);
	I_gw = inet_addr(networkinfo.col_gw);
	
	if((I_ip & I_mask) == (I_gw & I_mask))
	{
		if(inet_aton(networkinfo.col_gw, &sin->sin_addr)<0)
		{
			printf ( "inet_aton error\n" );
			close(fd);
			return CMM_FAILED; 
		}
		memcpy ( &rt.rt_gateway, sin, sizeof(struct sockaddr_in));
		((struct sockaddr_in *)&rt.rt_dst)->sin_family=AF_INET;
		((struct sockaddr_in *)&rt.rt_genmask)->sin_family=AF_INET;
		rt.rt_dev = ifname;
		rt.rt_flags = RTF_UP|RTF_GATEWAY;
		if (ioctl(fd, SIOCADDRT, &rt)<0)
		{
			perror( "ioctl(SIOCADDRT) error in set_default_route\n");
			close(fd);
			return CMM_FAILED;
		}			
	}
	
	
#endif

	close(fd);
	return CMM_SUCCESS;
}

int do_dsdt_rgmii_delay_config(st_dsdtRgmiiTimingDelay* pDelay)
{
	int opt_sts=CMM_SUCCESS;

	opt_sts = cmm2dsdt_setRgmiiTimingDelay(pDelay);
	
	if( CMM_SUCCESS ==  opt_sts )
	{
		if( pDelay->rxdelay )
		{
			dbs_sys_log(dbsdev, DBS_LOG_DEBUG, "dsdt port6 rgmii rx delay enabled");
		}
		if( pDelay->txdelay )
		{
			dbs_sys_log(dbsdev, DBS_LOG_DEBUG, "dsdt port6 rgmii tx delay enabled");
		}
	}

	return opt_sts;
}

/********************************************************************************************
*	��������:init_network
*	��������:�����ݿ��ȡ����������Ϣ,��ʼ������
*	����:may2250
*	ʱ��:2010-09-13
*********************************************************************************************/
int init_network(void)
{
	//int opt_sts=CMM_SUCCESS;	
	st_dbsNetwork	szNetwork;
	st_dbsSysinfo		szSysinfo;
	st_dsdtRgmiiTimingDelay dsdtRgmiiDelay;

	/*�����ݿ��ж�ȡ������Ϣ*/
	if(CMM_SUCCESS == dbsGetSysinfo(dbsdev, 1, &szSysinfo))
	{
		/* dsdt rgmii delay config */
		dsdtRgmiiDelay.port = 6;
		dsdtRgmiiDelay.rxdelay = szSysinfo.col_p6rxdelay;
		dsdtRgmiiDelay.txdelay = szSysinfo.col_p6txdelay;
		if( CMM_SUCCESS != do_dsdt_rgmii_delay_config(&dsdtRgmiiDelay) )
		{
			perror("cmm init_network->do_dsdt_rgmii_delay_config failed !\n");
			return CMM_FAILED;
		}		
	}
	else
	{
		perror("CMM init_network->dbsGetSysinfo error !\n");
		return CMM_FAILED;
	}
	
	if(CMM_SUCCESS == dbsGetNetwork(dbsdev, 1, &szNetwork))
	{
		/* ����ip,��������,����*/
		if( do_network_config(szNetwork) != CMM_SUCCESS )
		{
			perror("CMM init_network->do_network_config error !\n");
			return CMM_FAILED;
		}
		/* ���ù���VLAN */
		if( cmm2dsdt_mgmtVlanInit() != CMM_SUCCESS )
		{
			perror("CMM init_network->cmm2dsdt_mgmtVlanInit error !\n");
			return CMM_FAILED;
		}
		/*��������*/
		return CMM_SUCCESS;
	}
	else
	{
		perror("CMM init_network->db_get_network_info error !\n");
		return CMM_FAILED;
	}
}

/********************************************************************************************
*	��������:cmm_dump_msg
*	��������:������API����ʮ�����Ƶķ�ʽ�������������������
*				   �ļ�ָ��fpָ����豸�ļ�
*	����:frank
*	ʱ��:2010-08-13
*********************************************************************************************/
void cmm_dump_msg(const unsigned char memory [], size_t length, FILE *fp)
{
	if(CMM_MODULE_DEBUG_ENABLE)
	{
		printf("----------------------------------------------------------------------\n");
		hexdump(memory, length, fp);
		printf("\n----------------------------------------------------------------------\n");
	}
}

/********************************************************************************************
*	��������:CMM_ProcessAck
*	��������:�ú���ΪCMM�ⲿӦ������CMM������
*				  ���֮��ͨ���ú��������������ظ�������
*	����:frank
*	ʱ��:2010-08-13
*********************************************************************************************/
void CMM_ProcessAck(uint16_t status, BBLOCK_QUEUE *this, uint8_t *b, uint32_t len)
{
	assert(NULL != this);
	
	T_Msg_Header_CMM rh;
	T_Msg_Header_CMM *h = (T_Msg_Header_CMM *)(this->b);
	uint8_t buf[MAX_UDP_SIZE];
	uint16_t result = status;
	int sendn = 0;
	int total_len = len + sizeof(uint16_t) + sizeof(rh);

	bzero((char *)buf, MAX_UDP_SIZE);

	rh.usSrcMID = MID_CMM;
	rh.usDstMID = h->usSrcMID;
	rh.usMsgType = h->usMsgType;
	rh.fragment = 0;
	rh.ulBodyLength = (len + sizeof(uint16_t));		

	if( total_len > MAX_UDP_SIZE )
	{
		result = CMM_BUFFER_OVERFLOW;
		rh.ulBodyLength = sizeof(uint16_t);
		memcpy(buf, &rh, sizeof(rh));
		memcpy(buf+sizeof(rh), &result, sizeof(uint16_t));
		dbs_sys_log(dbsdev, DBS_LOG_ALERT, "cmm->nCMM_ProcessAck msg length exceeded");	
		
		/* ��������Ϣ���͸������� */
		sendn = sendto(this->sk.sk, buf, sizeof(rh)+sizeof(uint16_t), 0, 
				(struct sockaddr *)&(this->sk.skaddr), sizeof(this->sk.skaddr));
		if( sendn > 0 )
		{
			/* ��ӡ���յ����ݱ���*/
			cmm_debug_printf("\n<==== CMM SEND MASSAGE:\n");
			cmm_dump_msg(buf, sizeof(rh)+sizeof(uint16_t)+len, stdout);
		}
		return;
	}
	
	memcpy(buf, &rh, sizeof(rh));
	memcpy(buf+sizeof(rh), &result, sizeof(uint16_t));

	if( 0 != len )
	{
		assert(NULL != b);
		memcpy(buf+sizeof(rh)+sizeof(uint16_t), b, len);
	}
	
	/* ��ӡ���յ����ݱ���*/
	cmm_debug_printf("\n<==== CMM SEND MASSAGE:\n");
	cmm_dump_msg(buf, sizeof(rh)+sizeof(uint16_t)+len, stderr);
		
	/* ��������Ϣ���͸������� */
	sendn = sendto(this->sk.sk, buf, total_len, 0, 
				(struct sockaddr *)&(this->sk.skaddr), sizeof(this->sk.skaddr));
	if( sendn != total_len )
	{
		dbs_sys_log(dbsdev, DBS_LOG_ALERT, "cmm->nCMM_ProcessAck call sendto error");
	}
}

/********************************************************************************************
*	��������:CMM_ProcessMTAck
*	��������:���������Թ��߷���Ӧ�����Ϣ�ӿ�
*	����:may2250
*	ʱ��:2011-4-26
*********************************************************************************************/
void CMM_ProcessMTAck(BBLOCK_QUEUE *this, uint8_t *b, uint32_t len)
{
	if(sendto(this->sk.sk, b, len, 0,(struct sockaddr *)&(this->sk.skaddr), sizeof(this->sk.skaddr) ) == -1)
	{
		dbs_sys_log(dbsdev, DBS_LOG_ALERT, "ERROR: CMM_ProcessMTAck sendto failed");
		perror("\nERROR: CMM_ProcessMTAck sendto failed\n");
	}
}

#if 0
/********************************************************************************************
*	��������:CMM_ProcessSequenceAck
*	��������:�ú���ΪCMM�ⲿӦ������CMM������
*				  ���֮��ͨ���ú��������������ظ�������
*				  �ú�����ɳ������ĵ�Ӧ��
*	����:frank
*	ʱ��:2010-08-13
*********************************************************************************************/
void CMM_ProcessSequenceAck(uint16_t status, BBLOCK_QUEUE *this, uint8_t *b, uint32_t len)
{
	assert(NULL != this);

	uint8_t *p = b;
	T_Msg_Header_CMM rh;
	T_Msg_Header_CMM *h = (T_Msg_Header_CMM *)(this->b);
	uint8_t buf[MAX_UDP_SIZE];
	uint16_t result = status;
	int sendn = 0;
	int i = 1;
	int lenth = 0;
	int remain_len = len;
	int step = (MAX_UDP_SIZE-sizeof(rh)-sizeof(uint16_t));

	rh.usSrcMID = MID_CMM;
	rh.usDstMID = h->usSrcMID;
	rh.usMsgType = h->usMsgType;	

	for( remain_len=len; remain_len>0; remain_len-=step,p+=step )
	{
		bzero((char *)buf, MAX_UDP_SIZE);
		if( remain_len > step )
		{
			/* ����������һ֡*/
			rh.fragment = 1;
			rh.ulBodyLength = (step + sizeof(uint16_t));
			memcpy(buf, &rh, sizeof(rh));
			memcpy(buf+sizeof(rh), &result, sizeof(uint16_t));
			memcpy(buf+sizeof(rh)+sizeof(uint16_t), p, step);
			lenth = MAX_UDP_SIZE;
		}
		else
		{
			/* ���һ֡*/
			rh.fragment = 0;
			rh.ulBodyLength = (remain_len + sizeof(uint16_t));
			memcpy(buf, &rh, sizeof(rh));
			memcpy(buf+sizeof(rh), &result, sizeof(uint16_t));
			memcpy(buf+sizeof(rh)+sizeof(uint16_t), p, remain_len);
			lenth = (sizeof(rh)+sizeof(uint16_t)+remain_len);
		}

		/* ��ӡ���յ����ݱ���*/
		cmm_debug_printf("\n<==== CMM SEND MASSAGE [ SYN = %d ]:\n", i++);
		cmm_dump_msg(buf, lenth, stderr);
		
		/* ��������Ϣ���͸������� */
		sendn = sendto(this->sk.sk, buf, lenth, 0, (struct sockaddr *)&(this->sk.skaddr), sizeof(this->sk.skaddr));
		if( sendn != lenth )
		{
			dbs_sys_log(DBS_LOG_ALERT, "cmm->nCMM_ProcessAck call sendto error");
		}
	}	
}
#endif

int CMM_WriteOptLog(BBLOCK_QUEUE *this, int result)
{
	time_t b_time;
	st_dbsOptlog log;
	T_Msg_CMM *req = (T_Msg_CMM *)(this->b);

	/* HTTP �Ĳ�����־��HTTPD���Ѿ���¼*/
	if( req->HEADER.usSrcMID == MID_HTTP )
	{
		return CMM_SUCCESS;
	}
	if( req->HEADER.usSrcMID == MID_SYSMONITOR )
	{
		return CMM_SUCCESS;
	}
	if( req->HEADER.usMsgType == CMM_GET_CBAT_TEMPERATURE )
	{
		return CMM_SUCCESS;
	}

	/* ��ȡϵͳ��ǰʱ��*/
	time(&b_time);

	log.who = req->HEADER.usSrcMID;
	log.time = b_time;
	log.level = DBS_LOG_INFO;
	log.result = result?CMM_FAILED:CMM_SUCCESS;

	switch(req->HEADER.usMsgType)
	{
		case CMM_MODULE_DEBUG_CONTROL:
		{
			strcpy(log.cmd, "CMM_MODULE_DEBUG_CONTROL");
			break;
		}
		case CMM_CLT_RESET:
		{
			strcpy(log.cmd, "CMM_RESET_CLT");
			break;
		}
		case CMM_CNU_RESET:
		{
			strcpy(log.cmd, "CMM_RESET_CNU");
			break;
		}
		case CMM_SEND_TOPOLOGY_HB_TRAP:
		{
			strcpy(log.cmd, "SNMP_CMM_SEND_HEARTBEAT");
			break;
		}
		case CMM_DO_HB_TRAP_CTRL:
		{
			strcpy(log.cmd, "CMM_DO_HB_TRAP_CTRL");
			break;
		}
		case CMM_UNDO_HB_TRAP_CTRL:
		{
			strcpy(log.cmd, "CMM_UNDO_HB_TRAP_CTRL");
			break;
		}
		case CMM_CBAT_RESET:
		case CMM_RESTORE_DEFAULT:
		{
			/* �������ϵͳ������������־��Ϣ��û������*/
			break;
		}
		case CMM_CNU_VLAN_CONFIG:
		{
			strcpy(log.cmd, "CMM_DO_CNU_VLAN_CONFIG");
			break;
		}
		case CMM_CLI_FLOWCONTROL:
		{
			strcpy(log.cmd, "CMM_DO_CNU_RATE_LIMITING");
			break;
		}
		case CMM_CLI_STROMCONTROL:
		{
			strcpy(log.cmd, "CMM_DO_CNU_STORM_FILTER");
			break;
		}
		case CMM_CLI_SHUTDOWN:
		{
			strcpy(log.cmd, "CMM_DO_PROT_STATUS_CONFIG");
			break;
		}
		case CMM_AR8236_PHY_REG_READ:
		{
			strcpy(log.cmd, "CMM_DEBUG_PHY_REG_READ");
			break;
		}
		case CMM_AR8236_PHY_REG_WRITE:
		{
			strcpy(log.cmd, "CMM_DEBUG_PHY_REG_WRITE");
			break;
		}
		case CMM_AR8236_SW_REG_READ:
		{
			strcpy(log.cmd, "CMM_DEBUG_SW_REG_READ");
			break;
		}
		case CMM_AR8236_SW_REG_WRITE:
		{
			strcpy(log.cmd, "CMM_DEBUG_SW_REG_WRITE");
			break;
		}
		case CMM_MME_MDIO_READ:
		{
			strcpy(log.cmd, "CMM_MME_MDIO_READ");
			break;
		}
		case CMM_MME_MDIO_WRITE:
		{
			strcpy(log.cmd, "CMM_MME_MDIO_WRITE");
			break;
		}
		case CMM_CLI_UPGRADE:
		{
			strcpy(log.cmd, "CMM_UPGRADE_SOFTWARE");
			break;
		}
		case CMM_CLI_SEND_CONFIG:
		{
			strcpy(log.cmd, "CMM_RELOAD_CNU_PROFILE");
			break;
		}
		case CMM_CLI_USER_NEW:
		{
			strcpy(log.cmd, "CMM_PERMIT_CNU");
			break;
		}
		case CMM_CLI_USER_DEL:
		{
			strcpy(log.cmd, "CMM_UNDO_PERMIT_CNU");
			break;
		}
		case CMM_CLI_DELETE_USER:
		{
			strcpy(log.cmd, "CMM_DELETE_CNU");
			break;
		}
		case CMM_DO_WLIST_CONTROL:
		{
			strcpy(log.cmd, "CMM_ENABLE_WLIST");
			break;
		}
		case CMM_UNDO_WLIST_CONTROL:
		{
			strcpy(log.cmd, "CMM_DISABLE_WLIST");
			break;
		}
		case CMM_CREATE_CNU:
		{
			strcpy(log.cmd, "CMM_CREATE_CNU");
			break;
		}
		case CMM_DUMP_CNU_REG:
		{
			strcpy(log.cmd, "CMM_DEBUG_DUMP_CNU_REG");
			break;
		}
		case CMM_DUMP_CNU_MOD:
		{
			strcpy(log.cmd, "CMM_DEBUG_DUMP_CNU_MOD");
			break;
		}
		case CMM_DUMP_CNU_PIB:
		{
			strcpy(log.cmd, "CMM_DEBUG_DUMP_CNU_PIB");
			break;
		}
		case CMM_DO_WDT_CONTROL:
		{
			strcpy(log.cmd, "CMM_DO_WDT_CONTROL");
			break;
		}
		case CMM_UNDO_WDT_CONTROL:
		{
			strcpy(log.cmd, "CMM_UNDO_WDT_CONTROL");
			break;
		}
		case CMM_GET_CBAT_TEMPERATURE:
		{
			strcpy(log.cmd, "CMM_GET_CBAT_TEMPERATURE");
			break;
		}
		case CMM_GET_DSDT_RGMII_DELAY:
		{
			strcpy(log.cmd, "CMM_GET_DSDT_RGMII_DELAY");
			break;
		}
		case CMM_SET_DSDT_RGMII_DELAY:
		{
			strcpy(log.cmd, "CMM_SET_DSDT_RGMII_DELAY");
			break;
		}
		default:
		{
			strcpy(log.cmd, "CMM_UNKONEN_CMD");
			break;
		}
	}

	/* ����־д�����ݿ�*/
	return dbs_opt_log(dbsdev, &log);

}

int CMM_ProcessModuleDebugCtl(BBLOCK_QUEUE *this)
{
	assert( NULL != this );
	
	int opt_sts = CMM_SUCCESS;
	T_Msg_CMM *msg = (T_Msg_CMM *)(this->b);
	st_ModuleDebugCtl *pDebugClt = (st_ModuleDebugCtl *)(msg->BUF);

	switch(pDebugClt->usMID)
	{
		case MID_CMM:
		{
			CMM_MODULE_DEBUG_ENABLE = pDebugClt->enable;
			break;
		}
		case MID_DBS:
		{
			opt_sts = dbsMsgDebug(dbsdev, pDebugClt->enable);
			break;
		}
		case MID_SQL:
		{
			opt_sts = dbsSQLDebug(dbsdev, pDebugClt->enable);
			break;
		}
		default:
		{
			opt_sts = CMM_FAILED;
			break;
		}
	}
	
	/* ��������Ϣ���͸�������*/
	CMM_ProcessAck(opt_sts, this, NULL, 0);
	return CMM_SUCCESS;
}

int CMM_ProcessRestoreDefault(BBLOCK_QUEUE *this)
{
	int opt_sts = CMM_SUCCESS;
	
	CMM_ProcessAck(opt_sts, this, NULL, 0);
	/* ����ϵͳָʾ��æ״̬*/
	cmm2sysmonitor_sysledCtrol(SYSLED_STS_BUSY);
	/* ��Ҫ���͸澯*/
	cmm2alarm_sendCbatResetNotification();
	/* ɾ���ļ�ϵͳ�е�����*/
	system("rm -f /usr/mnt/config/databases/*");	
	sleep(3);
	/* ����ϵͳָʾ��ϵͳֹͣ����״̬*/
	cmm2sysmonitor_sysledCtrol(SYSLED_STS_RESET);
	system("reboot");
	return opt_sts;
}

int CMM_ProcessCbatReset(BBLOCK_QUEUE *this)
{
	CMM_ProcessAck(CMM_SUCCESS, this, NULL, 0);
	cmm2alarm_sendCbatResetNotification();
	/* ����ϵͳָʾ��ϵͳֹͣ����״̬*/
	cmm2sysmonitor_sysledCtrol(SYSLED_STS_RESET);
	/* ��Ҫ���͸澯*/
	sleep(1);
    	system("reboot");	
	return CMM_SUCCESS;
}

int CMM_ProcessSendHeartbeat(BBLOCK_QUEUE *this)
{
	CMM_ProcessAck(CMM_SUCCESS, this, NULL, 0);
	cmm2alarm_sendCbatTopHbTrap();
	return CMM_SUCCESS;
}

int CMM_ProcessHeartbeatCtrl(BBLOCK_QUEUE *this)
{
	int opt_sts = CMM_SUCCESS;
	T_Msg_CMM *req = (T_Msg_CMM *)(this->b);

	if( CMM_DO_HB_TRAP_CTRL == req->HEADER.usMsgType )
	{
		cmm2alarm_heartbeatEnable(1);
	}
	else if( CMM_UNDO_HB_TRAP_CTRL == req->HEADER.usMsgType )
	{
		cmm2alarm_heartbeatEnable(0);
	}
	else
	{
		opt_sts = CMM_FAILED;
	}	
	
	CMM_ProcessAck(opt_sts, this, NULL, 0);
	return opt_sts;
}

int CMM_ProcessCltReset(BBLOCK_QUEUE *this)
{
	int opt_sts = CMM_SUCCESS;
	T_Msg_CMM *req = (T_Msg_CMM *)(this->b);	
	uint16_t *index = (uint16_t *)(req->BUF);
	st_dbsClt clt;
	stRegEvent registerEvent;

	registerEvent.clt = *index;
	registerEvent.cnu = 0;
	registerEvent.event = REG_CLT_RESET;

	//printf("\r\n\r\n  CMM_ProcessCtlReset(%d)\n", registerEvent.clt);
	
	if( (registerEvent.clt < 1) ||(registerEvent.clt > MAX_CLT_AMOUNT_LIMIT))
	{
		opt_sts = CMM_FAILED;
	}
	else if( CMM_SUCCESS != dbsGetClt(dbsdev, registerEvent.clt, &clt))
	{
		opt_sts = CMM_FAILED;
	}
	else if( (DEV_STS_ONLINE != clt.col_sts)||(BOOL_TRUE != clt.col_row_sts))
	{
		opt_sts = CMM_SUCCESS;
	}
	else
	{
		/* ����MME ��CLT ����*/
		send_notification_to_reg(&registerEvent);
		opt_sts = CMM_SUCCESS;
	}

	CMM_ProcessAck(opt_sts, this, NULL, 0);
	return opt_sts;
}

int CMM_ProcessCnuReset(BBLOCK_QUEUE *this)
{
	int opt_sts = CMM_SUCCESS;
	T_Msg_CMM *req = (T_Msg_CMM *)(this->b);	
	uint16_t *index = (uint16_t *)(req->BUF);
	st_dbsCnu cnu;
	stRegEvent registerEvent;

	registerEvent.clt = 1;
	registerEvent.cnu = *index;
	registerEvent.event = REG_CNURESET;

	//printf("\r\n\r\n  CMM_ProcessResetCnu(%d)\n", registerEvent.cnu);

	if( (registerEvent.cnu < 1)||(registerEvent.cnu > MAX_CNU_AMOUNT_LIMIT))
	{
		opt_sts = CMM_FAILED;
	}
	else if( CMM_SUCCESS != dbsGetCnu(dbsdev, registerEvent.cnu, &cnu) )
	{
		opt_sts = CMM_FAILED;
	}	
	else if( (DEV_STS_ONLINE != cnu.col_sts)||BOOL_TRUE != cnu.col_row_sts )
	{
		opt_sts = CMM_SUCCESS;
	}
	else 
	{
		opt_sts = CMM_SUCCESS;
		send_notification_to_reg(&registerEvent);		
	}

	CMM_ProcessAck(opt_sts, this, NULL, 0);
	return opt_sts;
}

int CMM_ProcessCnuVlanConfig(BBLOCK_QUEUE *this)
{
	int opt_sts = CMM_SUCCESS;
	T_Msg_CMM *req = (T_Msg_CMM *)(this->b);
	st_dbsProfile *req_data = (st_dbsProfile *)(req->BUF);

	//printf("\r\n\r\n  CMM_ProcessCnuVlanConfig(%d)\n", req_data->id);
	
	opt_sts = cmmTmWriteCnuProfile(1, req_data->id, req_data);
	CMM_ProcessAck(opt_sts, this, NULL, 0);
	return opt_sts;
}

int CMM_ProcessCnuRateLimit(BBLOCK_QUEUE *this)
{
	int opt_sts = CMM_SUCCESS;
	T_Msg_CMM *req = (T_Msg_CMM *)(this->b);
	st_dbsProfile *req_data = (st_dbsProfile *)(req->BUF);

	//printf("\r\n\r\n  CMM_ProcessCnuRateLimit(%d)\n", req_data->id);
	
	opt_sts = cmmTmWriteCnuProfile(1, req_data->id, req_data);
	CMM_ProcessAck(opt_sts, this, NULL, 0);
	return opt_sts;
}

int CMM_ProcessCnuStormFilter(BBLOCK_QUEUE *this)
{
	int opt_sts = CMM_SUCCESS;
	T_Msg_CMM *req = (T_Msg_CMM *)(this->b);
	st_dbsProfile *req_data = (st_dbsProfile *)(req->BUF);

	//printf("\r\n\r\n  CMM_ProcessCnuStormFilter(%d)\n", req_data->id);
	
	opt_sts = cmmTmWriteCnuProfile(1, req_data->id, req_data);
	CMM_ProcessAck(opt_sts, this, NULL, 0);
	return opt_sts;
}

int CMM_ProcessCnuPortStatusConfig(BBLOCK_QUEUE *this)
{
	int opt_sts = CMM_SUCCESS;
	T_Msg_CMM *req = (T_Msg_CMM *)(this->b);
	st_dbsProfile *req_data = (st_dbsProfile *)(req->BUF);

	//printf("\r\n\r\n  CMM_ProcessCnuPortStatusConfig(%d)\n", req_data->id);
	
	opt_sts = cmmTmWriteCnuProfile(1, req_data->id, req_data);
	CMM_ProcessAck(opt_sts, this, NULL, 0);
	return opt_sts;
}

int CMM_ProcessCnuMacLimitConfig(BBLOCK_QUEUE *this)
{
	int opt_sts = CMM_SUCCESS;
	T_Msg_CMM *req = (T_Msg_CMM *)(this->b);
	st_dbsProfile *req_data = (st_dbsProfile *)(req->BUF);

	opt_sts = cmmTmWriteCnuProfile(1, req_data->id, req_data);
	CMM_ProcessAck(opt_sts, this, NULL, 0);
	return opt_sts;
}

int CMM_ProcessMmeMdioPhyRead(BBLOCK_QUEUE *this)
{
	int opt_sts = CMM_SUCCESS;
	st_dbsClt clt;
	T_Msg_CMM *msg = (T_Msg_CMM *)(this->b);
	T_szAr8236Phy *smiPhy = (T_szAr8236Phy *)(msg->BUF);
	T_szAr8236Phy cli_ar8236_phy;
	T_szMdioPhy ar8236_phy;
	uint8_t bMac[6] = {0};

	cli_ar8236_phy.clt = smiPhy->clt;
	cli_ar8236_phy.cnu = smiPhy->cnu;
	cli_ar8236_phy.mdioPhy.phy = smiPhy->mdioPhy.phy;
	cli_ar8236_phy.mdioPhy.reg = smiPhy->mdioPhy.reg;
	cli_ar8236_phy.mdioPhy.value = 0x0000;

	if( 0 != cli_ar8236_phy.cnu )
	{
		opt_sts = CMM_FAILED;
	}
	else if( (smiPhy->clt<1) ||(smiPhy->clt>MAX_CLT_AMOUNT_LIMIT))
	{
		opt_sts = CMM_FAILED;
	}	
	else if( CMM_SUCCESS != dbsGetClt(dbsdev, smiPhy->clt, &clt) )
	{
		opt_sts = CMM_FAILED;
	}
	else if( (DEV_STS_ONLINE != clt.col_sts)||BOOL_TRUE != clt.col_row_sts )
	{
		opt_sts = CMM_FAILED;
	}
	else if( CMM_SUCCESS != boardapi_macs2b(clt.col_mac, bMac) )
	{
		opt_sts = CMM_FAILED;
	}	
	else
	{
		memcpy(&ar8236_phy, &(smiPhy->mdioPhy), sizeof(T_szMdioPhy));
		
		/* ��MMEAD��ȡ��Ϣ*/	
		if( CMM_SUCCESS == mmead_get_ar8236_phy(bMac, &ar8236_phy))
		{
			cli_ar8236_phy.mdioPhy.value = ar8236_phy.value;
			opt_sts = CMM_SUCCESS;
		}
		else
		{
			opt_sts = CMM_FAILED;
		}
	}

	/* ��������Ϣ���͸������� */
	CMM_ProcessAck(opt_sts, this, (uint8_t *)&cli_ar8236_phy, sizeof(T_szAr8236Phy));

	return opt_sts;
}

int CMM_ProcessMmeMdioPhyWrite(BBLOCK_QUEUE *this)
{
	int opt_sts = CMM_SUCCESS;
	st_dbsClt clt;
	T_Msg_CMM *msg = (T_Msg_CMM *)(this->b);
	T_szAr8236Phy *smiPhy = (T_szAr8236Phy *)(msg->BUF);
	T_szAr8236Phy cli_ar8236_phy;
	T_szMdioPhy ar8236_phy;
	uint8_t bMac[6] = {0};

	cli_ar8236_phy.clt = smiPhy->clt;
	cli_ar8236_phy.cnu = smiPhy->cnu;
	cli_ar8236_phy.mdioPhy.phy = smiPhy->mdioPhy.phy;
	cli_ar8236_phy.mdioPhy.reg = smiPhy->mdioPhy.reg;
	cli_ar8236_phy.mdioPhy.value = smiPhy->mdioPhy.value;

	if( 0 != cli_ar8236_phy.cnu )
	{
		opt_sts = CMM_FAILED;
	}
	else if( (smiPhy->clt<1) ||(smiPhy->clt>MAX_CLT_AMOUNT_LIMIT))
	{
		opt_sts = CMM_FAILED;
	}	
	else if( CMM_SUCCESS != dbsGetClt(dbsdev, smiPhy->clt, &clt) )
	{
		opt_sts = CMM_FAILED;
	}
	else if( (DEV_STS_ONLINE != clt.col_sts)||BOOL_TRUE != clt.col_row_sts )
	{
		opt_sts = CMM_FAILED;
	}
	else if( CMM_SUCCESS != boardapi_macs2b(clt.col_mac, bMac) )
	{
		opt_sts = CMM_FAILED;
	}	
	else
	{
		memcpy(&ar8236_phy, &(smiPhy->mdioPhy), sizeof(T_szMdioPhy));
		/* ��MMEAD��ȡ��Ϣ*/
		opt_sts = mmead_set_ar8236_phy(bMac, &ar8236_phy);		
	}

	/* ��������Ϣ���͸������� */
	CMM_ProcessAck(opt_sts, this, (uint8_t *)&cli_ar8236_phy, sizeof(T_szAr8236Phy));
	return opt_sts;
}


int CMM_ProcessReadAr8236Phy(BBLOCK_QUEUE *this)
{
	int opt_sts = CMM_SUCCESS;
	st_dbsCnu cnu;
	T_Msg_CMM *msg = (T_Msg_CMM *)(this->b);
	T_szAr8236Phy *smiPhy = (T_szAr8236Phy *)(msg->BUF);
	T_szAr8236Phy cli_ar8236_phy;
	T_szMdioPhy ar8236_phy;
	uint8_t bMac[6] = {0};

	cli_ar8236_phy.clt = smiPhy->clt;
	cli_ar8236_phy.cnu = smiPhy->cnu;
	cli_ar8236_phy.mdioPhy.phy = smiPhy->mdioPhy.phy;
	cli_ar8236_phy.mdioPhy.reg = smiPhy->mdioPhy.reg;
	cli_ar8236_phy.mdioPhy.value = 0x0000;

	if( (smiPhy->cnu<1)||(smiPhy->cnu > MAX_CNU_AMOUNT_LIMIT))
	{
		opt_sts = CMM_FAILED;
	}
	else if( (smiPhy->clt<1) ||(smiPhy->clt>MAX_CLT_AMOUNT_LIMIT))
	{
		opt_sts = CMM_FAILED;
	}
	else if( CMM_SUCCESS != dbsGetCnu(dbsdev, smiPhy->cnu, &cnu) )
	{
		opt_sts = CMM_FAILED;
	}
	else if( (DEV_STS_ONLINE != cnu.col_sts)||BOOL_TRUE != cnu.col_row_sts )
	{
		opt_sts = CMM_FAILED;
	}
	else if( CMM_SUCCESS != boardapi_macs2b(cnu.col_mac, bMac) )
	{
		opt_sts = CMM_FAILED;
	}
	else if( !boardapi_isCnuSupported(cnu.col_model) )
	{
		opt_sts = CMM_FAILED;
	}
	else
	{
		memcpy(&ar8236_phy, &(smiPhy->mdioPhy), sizeof(T_szMdioPhy));
		
		/* ��MMEAD��ȡ��Ϣ*/	
		if( CMM_SUCCESS == mmead_get_ar8236_phy(bMac, &ar8236_phy))
		{
			cli_ar8236_phy.mdioPhy.value = ar8236_phy.value;
			opt_sts = CMM_SUCCESS;
		}
		else
		{
			opt_sts = CMM_FAILED;
		}
	}

	/* ��������Ϣ���͸������� */
	CMM_ProcessAck(opt_sts, this, (uint8_t *)&cli_ar8236_phy, sizeof(T_szAr8236Phy));

	return opt_sts;
}

int CMM_ProcessWriteAr8236Phy(BBLOCK_QUEUE *this)
{
	int opt_sts = CMM_SUCCESS;
	st_dbsCnu cnu;
	T_Msg_CMM *msg = (T_Msg_CMM *)(this->b);
	T_szAr8236Phy *smiPhy = (T_szAr8236Phy *)(msg->BUF);
	T_szAr8236Phy cli_ar8236_phy;
	T_szMdioPhy ar8236_phy;
	uint8_t bMac[6] = {0};

	cli_ar8236_phy.clt = smiPhy->clt;
	cli_ar8236_phy.cnu = smiPhy->cnu;
	cli_ar8236_phy.mdioPhy.phy = smiPhy->mdioPhy.phy;
	cli_ar8236_phy.mdioPhy.reg = smiPhy->mdioPhy.reg;
	cli_ar8236_phy.mdioPhy.value = smiPhy->mdioPhy.value;

	if( CMM_SUCCESS != dbsGetCnu(dbsdev, smiPhy->cnu, &cnu) )
	{
		opt_sts = CMM_DB_GETCNU_ERROR;
	}
	else if( (DEV_STS_ONLINE != cnu.col_sts)||BOOL_TRUE != cnu.col_row_sts )
	{
		opt_sts = CMM_FAILED;
	}
	else if( CMM_SUCCESS != boardapi_macs2b(cnu.col_mac, bMac) )
	{
		opt_sts = CMM_FAILED;
	}
	else if( !boardapi_isCnuSupported(cnu.col_model) )
	{
		opt_sts = CMM_FAILED;
	}
	else
	{
		memcpy(&ar8236_phy, &(smiPhy->mdioPhy), sizeof(T_szMdioPhy));
		/* ��MMEAD��ȡ��Ϣ*/
		opt_sts = mmead_set_ar8236_phy(bMac, &ar8236_phy);		
	}

	/* ��������Ϣ���͸������� */
	CMM_ProcessAck(opt_sts, this, (uint8_t *)&cli_ar8236_phy, sizeof(T_szAr8236Phy));
	return opt_sts;
}

int CMM_ProcessReadAr8236Reg(BBLOCK_QUEUE *this)
{
	int opt_sts = CMM_SUCCESS;
	st_dbsCnu cnu;
	T_Msg_CMM *msg = (T_Msg_CMM *)(this->b);
	T_szAr8236Reg *smiReg = (T_szAr8236Reg *)(msg->BUF);	
	T_szAr8236Reg cli_ar8236_reg;
	T_szMdioSw ar8236_reg;
	uint8_t bMac[6] = {0};

	cli_ar8236_reg.clt = smiReg->clt;
	cli_ar8236_reg.cnu = smiReg->cnu;
	cli_ar8236_reg.mdioReg.reg = smiReg->mdioReg.reg;
	cli_ar8236_reg.mdioReg.value = 0x0000;

	if( CMM_SUCCESS != dbsGetCnu(dbsdev, smiReg->cnu, &cnu) )
	{
		opt_sts = CMM_DB_GETCNU_ERROR;
	}
	else if( (DEV_STS_ONLINE != cnu.col_sts)||BOOL_TRUE != cnu.col_row_sts )
	{
		opt_sts = CMM_FAILED;
	}
	else if( CMM_SUCCESS != boardapi_macs2b(cnu.col_mac, bMac) )
	{
		opt_sts = CMM_FAILED;
	}
	else if( !boardapi_isCnuSupported(cnu.col_model) )
	{
		opt_sts = CMM_FAILED;
	}
	else
	{
		memcpy(&ar8236_reg, &(smiReg->mdioReg), sizeof(T_szMdioSw));
		/* ��MMEAD��ȡ��Ϣ*/	
		if( CMM_SUCCESS == mmead_get_ar8236_reg(bMac, &ar8236_reg))
		{
			cli_ar8236_reg.mdioReg.value = ar8236_reg.value;
			opt_sts = CMM_SUCCESS;
		}
		else
		{
			opt_sts = CMM_FAILED;
		}
	}

	/* ��������Ϣ���͸������� */
	CMM_ProcessAck(opt_sts, this, (uint8_t *)&cli_ar8236_reg, sizeof(T_szAr8236Reg));
	return opt_sts;
}

int CMM_ProcessWriteAr8236Reg(BBLOCK_QUEUE *this)
{
	int opt_sts = CMM_SUCCESS;
	st_dbsCnu cnu;
	T_Msg_CMM *msg = (T_Msg_CMM *)(this->b);
	T_szAr8236Reg *smiReg = (T_szAr8236Reg *)(msg->BUF);	
	T_szAr8236Reg cli_ar8236_reg;
	T_szMdioSw ar8236_reg;
	uint8_t bMac[6] = {0};

	cli_ar8236_reg.clt = smiReg->clt;
	cli_ar8236_reg.cnu = smiReg->cnu;
	cli_ar8236_reg.mdioReg.reg = smiReg->mdioReg.reg;
	cli_ar8236_reg.mdioReg.value = smiReg->mdioReg.value;

	if( CMM_SUCCESS != dbsGetCnu(dbsdev, smiReg->cnu, &cnu) )
	{
		opt_sts = CMM_DB_GETCNU_ERROR;
	}
	else if( (DEV_STS_ONLINE != cnu.col_sts)||BOOL_TRUE != cnu.col_row_sts )
	{
		opt_sts = CMM_FAILED;
	}
	else if( CMM_SUCCESS != boardapi_macs2b(cnu.col_mac, bMac) )
	{
		opt_sts = CMM_FAILED;
	}
	else if( !boardapi_isCnuSupported(cnu.col_model) )
	{
		opt_sts = CMM_FAILED;
	}
	else
	{
		memcpy(&ar8236_reg, &(smiReg->mdioReg), sizeof(T_szMdioSw));
		/* ��MMEAD��ȡ��Ϣ*/
		opt_sts = mmead_set_ar8236_reg(bMac, &ar8236_reg);
	}

	/* ��������Ϣ���͸������� */
	CMM_ProcessAck(opt_sts, this, (uint8_t *)&cli_ar8236_reg, sizeof(T_szAr8236Reg));
	return opt_sts;
}

int CMM_ProcessLinkDiag(BBLOCK_QUEUE *this)
{
	int opt_sts = CMM_SUCCESS;
	st_dbsCnu cnu;
	st_dbsClt clt;
	uint8_t oda[6] = {0};
	
	T_Msg_CMM *msg = (T_Msg_CMM *)(this->b);
	T_CMM_LINK_DIAG_INFO *msg_data = (T_CMM_LINK_DIAG_INFO *)(msg->BUF);

	T_MMEAD_LINK_DIAG_INFO inputInfo;
	T_MMEAD_LINK_DIAG_RESULT outputInfo;	

	/* RX/TX */
	if( msg_data->dir > 1 )
	{
		opt_sts = CMM_FAILED;
		/* ��������Ϣ���͸������� */
		CMM_ProcessAck(opt_sts, this, NULL, 0);
		return opt_sts;
	}

	/* Get CCo MAC address */
	if( CMM_SUCCESS == dbsGetClt(dbsdev, 1, &clt) )
	{
		inputInfo.dir = msg_data->dir;
		if( CMM_SUCCESS != boardapi_macs2b(clt.col_mac, inputInfo.ccoMac) )
		{
			opt_sts = CMM_FAILED;
			/* ��������Ϣ���͸������� */
			CMM_ProcessAck(opt_sts, this, NULL, 0);
			return opt_sts;
		}		
		memcpy(inputInfo.peerNodeMac, inputInfo.ccoMac, 6);
	}
	else
	{
		opt_sts = CMM_FAILED;
		/* ��������Ϣ���͸������� */
		CMM_ProcessAck(opt_sts, this, NULL, 0);
		return opt_sts;
	}

	/* CNU �����߽�ֹ���*/
	if( CMM_SUCCESS == dbsGetCnu(dbsdev, msg_data->cnu, &cnu) )
	{
		if( DEV_STS_ONLINE != cnu.col_sts )
		{
			opt_sts = CMM_FAILED;
			/* ��������Ϣ���͸������� */
			CMM_ProcessAck(opt_sts, this, NULL, 0);
			return opt_sts;
		}
	}
	else
	{
		opt_sts = CMM_FAILED;
		/* ��������Ϣ���͸������� */
		CMM_ProcessAck(opt_sts, this, NULL, 0);
		return opt_sts;
	}

	if( CMM_SUCCESS != boardapi_macs2b(cnu.col_mac, oda) )
	{
		opt_sts = CMM_FAILED;
		/* ��������Ϣ���͸������� */
		CMM_ProcessAck(opt_sts, this, NULL, 0);
		return opt_sts;
	}
	
	if( CMM_SUCCESS != mmead_do_link_diag(oda, &inputInfo, &outputInfo) )
	{
		opt_sts = CMM_FAILED;
		/* ��������Ϣ���͸������� */
		CMM_ProcessAck(opt_sts, this, NULL, 0);
		return opt_sts;
	}
	else
	{
		opt_sts = CMM_SUCCESS;
		outputInfo.model = cnu.col_model;
		/* ��������Ϣ���͸������� */
		CMM_ProcessAck(opt_sts, this, (uint8_t *)&outputInfo, sizeof(T_MMEAD_LINK_DIAG_RESULT));
		return opt_sts;
	}
}

int CMM_ProcessGetPortPropety(BBLOCK_QUEUE *this)
{
	int opt_sts = CMM_SUCCESS;
	
	T_Msg_CMM *msg = (T_Msg_CMM *)(this->b);
	uint32_t *msg_data = (uint32_t *)(msg->BUF);

	T_CMM_PORT_PROPETY_INFO ack_data;
	int portid = *msg_data;

	if( ( portid < 0 )||(portid > 6) )
	{
		opt_sts = CMM_FAILED;
		/* ��������Ϣ���͸������� */
		CMM_ProcessAck(opt_sts, this, NULL, 0);
		return opt_sts;
	}

	ack_data.linkStatus = cmm2dsdt_getPortLinkStatus(portid);
	ack_data.speed = cmm2dsdt_getPortLinkSpeed(portid);
	ack_data.duplex = cmm2dsdt_getPortLinkDuplex(portid);
	ack_data.pri = cmm2dsdt_getPortPri(portid);
	ack_data.flowControl = cmm2dsdt_getPortFlowControl(portid);
	ack_data.portStatus = cmm2dsdt_getPortState(portid);	

	/* ��������Ϣ���͸������� */
	CMM_ProcessAck(opt_sts, this, (uint8_t *)&ack_data, sizeof(T_CMM_PORT_PROPETY_INFO));
	return opt_sts;
}

int CMM_ProcessGetDsdtRgmiiDelay(BBLOCK_QUEUE *this)
{
	int opt_sts = CMM_SUCCESS;	

	T_Msg_CMM *msg = (T_Msg_CMM *)(this->b);
	st_dsdtRgmiiTimingDelay *msg_data = (st_dsdtRgmiiTimingDelay *)(msg->BUF);
	
	st_dsdtRgmiiTimingDelay ack_data;
	ack_data.port = msg_data->port;

	opt_sts = cmm2dsdt_getRgmiiTimingDelay(&ack_data);	

	/* ��������Ϣ���͸������� */
	CMM_ProcessAck(opt_sts, this, (uint8_t *)&ack_data, sizeof(st_dsdtRgmiiTimingDelay));
	return opt_sts;
}

int CMM_ProcessSetDsdtRgmiiDelay(BBLOCK_QUEUE *this)
{
	int opt_sts = CMM_SUCCESS;
	
	T_Msg_CMM *msg = (T_Msg_CMM *)(this->b);
	st_dsdtRgmiiTimingDelay *msg_data = (st_dsdtRgmiiTimingDelay *)(msg->BUF);

	/* save dsdt rgmii delay config in dbs */
	if( CMM_SUCCESS != dbsSetDsdtRgmiiDelay(dbsdev, msg_data) )
	{
		fprintf(stderr, "\r\n  WARNING: Cannot save dsdt rgmii delay config into databases.");
		dbs_sys_log(dbsdev, DBS_LOG_WARNING, "cannot save dsdt rgmii delay config into databases");
	}

	opt_sts = cmm2dsdt_setRgmiiTimingDelay(msg_data);

	/* ��������Ϣ���͸������� */
	CMM_ProcessAck(opt_sts, this, NULL, 0);
	return opt_sts;
}

int CMM_ProcessSetDsdtPortMirroring(BBLOCK_QUEUE *this)
{
	int opt_sts = CMM_SUCCESS;
	
	T_Msg_CMM *msg = (T_Msg_CMM *)(this->b);
	st_dsdtPortMirroring *msg_data = (st_dsdtPortMirroring *)(msg->BUF);


	opt_sts = cmm2dsdt_setPortMirroring(msg_data);	

	/* ��������Ϣ���͸������� */
	CMM_ProcessAck(opt_sts, this, NULL, 0);
	return opt_sts;
}


int CMM_ProcessGetCbatTemperature(BBLOCK_QUEUE *this)
{
	int opt_sts = CMM_SUCCESS;
	st_temperature ack_data;

	opt_sts = get_at30ts75_init_status();
	if( CMM_SUCCESS == opt_sts )
	{
		opt_sts = at30ts75_read_temperature(&ack_data);
	}
	
	/* ��������Ϣ���͸������� */
	CMM_ProcessAck(opt_sts, this, (uint8_t *)&ack_data, sizeof(st_temperature));
	return opt_sts;
}

int CMM_ProcessGetPortStats(BBLOCK_QUEUE *this)
{
	int opt_sts = CMM_SUCCESS;
	
	T_Msg_CMM *msg = (T_Msg_CMM *)(this->b);
	uint32_t *msg_data = (uint32_t *)(msg->BUF);

	T_CMM_PORT_STATS_INFO ack_data;
	int portid = *msg_data;

	if( ( portid < 0 )||(portid > 6) )
	{
		opt_sts = CMM_FAILED;
		/* ��������Ϣ���͸������� */
		CMM_ProcessAck(opt_sts, this, NULL, 0);
		return opt_sts;
	}

	//cmm2dsdt_getPortCtr(portid, &ack_data);
	cmm2dsdt_getPortAllCounters(portid, &ack_data);

	/* ��������Ϣ���͸������� */
	CMM_ProcessAck(opt_sts, this, (uint8_t *)&ack_data, sizeof(T_CMM_PORT_STATS_INFO));
	return opt_sts;
}

int CMM_ProcessDebugPrintPortStats(BBLOCK_QUEUE *this)
{
	int opt_sts = CMM_SUCCESS;
	
	T_Msg_CMM *msg = (T_Msg_CMM *)(this->b);
	uint32_t *msg_data = (uint32_t *)(msg->BUF);

	int portid = *msg_data;

	if( ( portid < 0 )||(portid > 6) )
	{
		opt_sts = CMM_FAILED;
		/* ��������Ϣ���͸������� */
		CMM_ProcessAck(opt_sts, this, NULL, 0);
		return opt_sts;
	}

	/* ��������Ϣ���͸������� */
	CMM_ProcessAck(cmm2dsdt_debugPrintPortAllCounters(portid), this, NULL, 0);
	return opt_sts;
}

int CMM_ProcessClearPortStats(BBLOCK_QUEUE *this)
{
	/* ��������Ϣ���͸������� */
	CMM_ProcessAck(cmm2dsdt_clearPortCounters(), this, NULL, 0);
	return CMM_SUCCESS;
}

int CMM_ProcessUpgradeFtp(BBLOCK_QUEUE *this)
{
	int opt_sts = CMM_SUCCESS;
	
	T_Msg_CMM *request = (T_Msg_CMM *)(this->b);

	if( request->HEADER.usSrcMID == MID_CLI )
	{
		CMM_ProcessAck(processCliUpgrade(), this, NULL, 0);
		return CMM_SUCCESS;
	}
	else
	{
		CMM_ProcessAck(CMM_SUCCESS, this, NULL, 0);
		opt_sts = processSnmpUpgrade();
	}
	return opt_sts;
}

int CMM_ProcessReloadCnuProfile(BBLOCK_QUEUE *this)
{
	stRegEvent registerEvent;
	T_Msg_CMM *msg = (T_Msg_CMM *)(this->b);
	stTmUserInfo *inode = (stTmUserInfo *)(msg->BUF);

	registerEvent.clt = inode->clt;
	registerEvent.cnu = inode->cnu;
	registerEvent.event = REG_CNU_FORCE_REGISTRATION;

	send_notification_to_reg(&registerEvent);

	/* ��������Ϣ���͸������� */
	CMM_ProcessAck(CMM_SUCCESS, this, NULL, 0);
	
	return CMM_SUCCESS;	
}

int CMM_ProcessUserPermit(BBLOCK_QUEUE *this)
{
	int opt_sts = CMM_SUCCESS;
	T_Msg_CMM *req = (T_Msg_CMM *)(this->b);
	stTmUserInfo *req_data = (stTmUserInfo *)(req->BUF);

	//printf("\r\n\r\n  CMM_ProcessUserPermit(%d, %d)\n", req_data->clt, req_data->cnu);
	
	opt_sts = cmmTmDoCnuPermit(req_data->clt, req_data->cnu);
	CMM_ProcessAck(opt_sts, this, NULL, 0);
	return opt_sts;
}

int CMM_ProcessUndoUserPermit(BBLOCK_QUEUE *this)
{
	int opt_sts = CMM_SUCCESS;
	T_Msg_CMM *req = (T_Msg_CMM *)(this->b);
	stTmUserInfo *req_data = (stTmUserInfo *)(req->BUF);

	//printf("\r\n\r\n  CMM_ProcessUndoUserPermit(%d, %d)\n", req_data->clt, req_data->cnu);
	
	opt_sts = cmmTmUndoCnuPermit(req_data->clt, req_data->cnu);
	CMM_ProcessAck(opt_sts, this, NULL, 0);
	return opt_sts;
}

int CMM_ProcessDeleteCnu(BBLOCK_QUEUE *this)
{
	int opt_sts = CMM_SUCCESS;
	T_Msg_CMM *req = (T_Msg_CMM *)(this->b);
	stTmUserInfo *req_data = (stTmUserInfo *)(req->BUF);

	//printf("\r\n\r\n  CMM_ProcessDeleteCnu(%d, %d)\n", req_data->clt, req_data->cnu);
	
	opt_sts = cmmTmDeleteCnu(req_data->clt, req_data->cnu);
	CMM_ProcessAck(opt_sts, this, NULL, 0);
	return opt_sts;
}

int CMM_ProcessCreateCnu(BBLOCK_QUEUE *this)
{
	int opt_sts = CMM_SUCCESS;
	T_Msg_CMM *req = (T_Msg_CMM *)(this->b);

	//printf("\r\n\r\n  CMM_ProcessCreateCnu()\n");

	/* �ж�CNU �����Ƿ�ﵽ����*/
	if( !__findCnuEntry() )
	{
		dbs_sys_log(dbsdev, DBS_LOG_ERR, "cmm create cnu error: cnu entry is full !");
		opt_sts = TM_DB_MAX_ROW;
	}	
	/* �ж�����ӵ�MAC ��ַ������Ч��û���ظ�*/
	else if( !__isNewCnuMacaddr(req->BUF) )
	{
		dbs_sys_log(dbsdev, DBS_LOG_ERR, "cmm create cnu error: cnu mac conflict !");
		opt_sts = CMM_CONFIG_REPEAT;
	}
	else
	{
		/* ����������ע��ģ��*/
		send_notification2_to_reg(req->BUF);
		opt_sts = CMM_SUCCESS;
	}
	
	CMM_ProcessAck(opt_sts, this, NULL, 0);
	return opt_sts;
}

int CMM_ProcessDoWlistControl(BBLOCK_QUEUE *this)
{
	int opt_sts = CMM_SUCCESS;
	T_Msg_CMM *req = (T_Msg_CMM *)(this->b);

	if( CMM_DO_WLIST_CONTROL == req->HEADER.usMsgType )
	{
		//printf("\r\n\r\n  CMM_ProcessDoWlistControl(1)\n");
		opt_sts = cmmTmDoWlistControl(1);
	}
	else if( CMM_UNDO_WLIST_CONTROL == req->HEADER.usMsgType )
	{
		//printf("\r\n\r\n  CMM_ProcessDoWlistControl(0)\n");
		opt_sts = cmmTmDoWlistControl(0);
	}
	else
	{
		opt_sts = CMM_FAILED;
	}	
	
	CMM_ProcessAck(opt_sts, this, NULL, 0);
	return opt_sts;
}

int CMM_ProcessDoWdtControl(BBLOCK_QUEUE *this)
{
	int opt_sts = CMM_SUCCESS;
	T_Msg_CMM *req = (T_Msg_CMM *)(this->b);
	DB_INTEGER_V iValue;

	iValue.ci.tbl = DBS_SYS_TBL_ID_SYSINFO;
	iValue.ci.row = 1;
	iValue.ci.col = DBS_SYS_TBL_SYSINFO_COL_ID_WDT;
	iValue.ci.colType = DBS_INTEGER;
	iValue.len = sizeof(uint32_t);

	if( CMM_DO_WDT_CONTROL == req->HEADER.usMsgType )
	{
		if( CMM_SUCCESS == nvm_set_parameter("watchdog", "on") )
		{
			iValue.integer = 1;
			if( CMM_SUCCESS == dbsUpdateInteger(dbsdev, &iValue) )
			{
				opt_sts = CMM_SUCCESS;
			}
			else
			{
				opt_sts = CMM_FAILED;
			}
		}
		else
		{
			opt_sts = CMM_FAILED;
		}		
	}
	else if( CMM_UNDO_WDT_CONTROL == req->HEADER.usMsgType )
	{
		if( CMM_SUCCESS == nvm_set_parameter("watchdog", "off") )
		{
			iValue.integer = 0;
			if( CMM_SUCCESS == dbsUpdateInteger(dbsdev, &iValue) )
			{
				opt_sts = CMM_SUCCESS;
			}
			else
			{
				opt_sts = CMM_FAILED;
			}
		}
		else
		{
			opt_sts = CMM_FAILED;
		}
	}
	else
	{
		opt_sts = CMM_FAILED;
	}	
	
	CMM_ProcessAck(opt_sts, this, NULL, 0);
	return opt_sts;
}

int CMM_ProcessDumpCnuReg(BBLOCK_QUEUE *this)
{
	int opt_sts = CMM_SUCCESS;
	T_Msg_CMM *req = (T_Msg_CMM *)(this->b);
	stTmUserInfo *req_data = (stTmUserInfo *)(req->BUF);

	//printf("\r\n\r\n  CMM_ProcessDumpCnuReg(%d, %d)\n", req_data->clt, req_data->cnu);
	
	opt_sts = cmmTmDumpCnuReg(req_data->clt, req_data->cnu);
	CMM_ProcessAck(opt_sts, this, NULL, 0);
	return opt_sts;
}

int CMM_ProcessDumpCnuMod(BBLOCK_QUEUE *this)
{
	int opt_sts = CMM_SUCCESS;
	T_Msg_CMM *req = (T_Msg_CMM *)(this->b);
	stTmUserInfo *req_data = (stTmUserInfo *)(req->BUF);

	//printf("\r\n\r\n  CMM_ProcessDumpCnuMod(%d, %d)\n", req_data->clt, req_data->cnu);
	
	opt_sts = cmmTmDumpCnuMod(req_data->clt, req_data->cnu);
	CMM_ProcessAck(opt_sts, this, NULL, 0);
	return opt_sts;
}

int CMM_ProcessDumpCnuPib(BBLOCK_QUEUE *this)
{
	int opt_sts = CMM_SUCCESS;
	T_Msg_CMM *req = (T_Msg_CMM *)(this->b);
	stTmUserInfo *req_data = (stTmUserInfo *)(req->BUF);

	//printf("\r\n\r\n  CMM_ProcessDumpCnuPib(%d, %d)\n", req_data->clt, req_data->cnu);
	
	opt_sts = cmmTmDumpCnuPib(req_data->clt, req_data->cnu);
	CMM_ProcessAck(opt_sts, this, NULL, 0);
	return opt_sts;
}

/********************************************************************************************
*	��������:CMM_ProcessConnect
*	��������:����MACTOOL����������֤
*	����:may2250
*	ʱ��:2011-4-26
*********************************************************************************************/
static int CMM_ProcessMTConnect(BBLOCK_QUEUE *this)
{
	char *ack_data = "OK";
	CMM_ProcessMTAck(this, ack_data, strlen(ack_data));
	return CMM_SUCCESS;
}

/********************************************************************************************
*	��������:CMM_ProcessToolMac
*	��������:����MACTOOL���ߵ�MAC��Ϣ������CBAT
*	����:may2250
*	ʱ��:2011-4-26
*********************************************************************************************/
static int CMM_ProcessMTProgram(BBLOCK_QUEUE *this)
{
	int opt_sts = CMM_SUCCESS;
	stMTmsgInfo recvpack;
	//char strDevModel[32] = {0};

	T_Msg_CMM *req = (T_Msg_CMM *)(this->b);
	stMTmsgInfo *req_data = (stMTmsgInfo *)(req->BUF);

	bzero(&recvpack, sizeof(recvpack));
	
	memcpy(recvpack.mac, req_data->mac, 17);
	
	/* change big-endian to little-endian */
	recvpack.model = boardapi_umapDevModel(req_data->model);
	
	/*��д���ذ�MAC*/
	opt_sts = boardapi_setMTParameters(&recvpack);
	
	if( CMM_SUCCESS ==  opt_sts )
	{
		CMM_ProcessMTAck(this, "1", 1);
		return CMM_SUCCESS;
	}
	else
	{
		CMM_ProcessMTAck(this, "0", 1);
		perror("\nERROR: CMM_ProcessToolMac write mac address failed\n");
		dbs_sys_log(dbsdev, DBS_LOG_ALERT, "ERROR: CMM_ProcessToolMac write mac address failed");
		return CMM_FAILED;
	}	
}

void cmmProcessManager(void)
{
	int opt_sts = CMM_SUCCESS;
	T_Msg_Header_CMM *h = NULL;
	BBLOCK_QUEUE *this = &bblock;
	int FromAddrSize = 0;	
	memcpy((char *)&(this->sk), (const char *)&SK_CMM, sizeof(T_UDP_SK_INFO));
	FromAddrSize = sizeof(SK_CMM.skaddr);

	assert( NULL != this );

	while(1)
	{
		bzero(this->b, MAX_UDP_SIZE);
		
		/*���ﶼ����fragment�ֶ�Ϊ0������������Ϣ��Ƭ�����*/
		this->blen = recvfrom(this->sk.sk, this->b, MAX_UDP_SIZE, 0, 
			(struct sockaddr *)&(this->sk.skaddr), &FromAddrSize);

		/* ��ӡ���յ��ı���*/
		cmm_debug_printf("\n====> CMM RECIEVED MASSAGE:\n");
		cmm_dump_msg(this->b, this->blen, stderr);
		
		h = (T_Msg_Header_CMM *)(this->b);

		/* WEC9720EK cpu is little-endian but WEC-3501I cpu is big-endian, Modifications to production test software compatible interface */
		if( h->usDstMID != MID_CMM )
		{
			fprintf(stderr, "cmm received error msg: usDstMID = 0x%02X, continue", h->usDstMID);
			continue;
		}
		
		/* �÷�֧����е�����һ��case�Ĵ���������Ӧ������ */
		switch(h->usMsgType)
		{
			case CMM_MODULE_DEBUG_CONTROL:
			{
				opt_sts = CMM_ProcessModuleDebugCtl(this);
				break;
			}
			case CMM_CLT_RESET:
			{
				opt_sts = CMM_ProcessCltReset(this);
				break;
			}
			case CMM_CNU_RESET:
			{
				opt_sts = CMM_ProcessCnuReset(this);
				break;
			}
			case CMM_SEND_TOPOLOGY_HB_TRAP:
			{
				opt_sts = CMM_ProcessSendHeartbeat(this);
				break;
			}
			case CMM_DO_HB_TRAP_CTRL:
			case CMM_UNDO_HB_TRAP_CTRL:
			{
				opt_sts = CMM_ProcessHeartbeatCtrl(this);
				break;
			}
			case CMM_CBAT_RESET:
			{
				opt_sts = CMM_ProcessCbatReset(this);
				break;
			}
			case CMM_RESTORE_DEFAULT:
			{
				opt_sts = CMM_ProcessRestoreDefault(this);
				break;
			}
			case CMM_CNU_VLAN_CONFIG:
			{
				opt_sts = CMM_ProcessCnuVlanConfig(this);
				break;
			}
			case CMM_CLI_FLOWCONTROL:
			{
				opt_sts = CMM_ProcessCnuRateLimit(this);
				break;
			}
			case CMM_CLI_STROMCONTROL:
			{
				opt_sts = CMM_ProcessCnuStormFilter(this);
				break;
			}
			case CMM_CLI_SHUTDOWN:
			{
				opt_sts = CMM_ProcessCnuPortStatusConfig(this);
				break;
			}
			case CMM_CLI_DO_AGING_TIME_CONFIG:
			case CMM_DO_MAC_LIMIT_CONFIG:
			{
				opt_sts = CMM_ProcessCnuMacLimitConfig(this);
				break;
			}
			case CMM_AR8236_PHY_REG_READ:
			{
				opt_sts = CMM_ProcessReadAr8236Phy(this);
				break;
			}
			case CMM_AR8236_PHY_REG_WRITE:
			{
				opt_sts = CMM_ProcessWriteAr8236Phy(this);
				break;
			}
			case CMM_AR8236_SW_REG_READ:
			{
				opt_sts = CMM_ProcessReadAr8236Reg(this);
				break;
			}
			case CMM_AR8236_SW_REG_WRITE:
			{
				opt_sts = CMM_ProcessWriteAr8236Reg(this);
				break;
			}
			case CMM_MME_MDIO_READ:
			{
				opt_sts = CMM_ProcessMmeMdioPhyRead(this);
				break;
			}
			case CMM_MME_MDIO_WRITE:
			{
				opt_sts = CMM_ProcessMmeMdioPhyWrite(this);
				break;
			}
			case CMM_CLI_UPGRADE:
			{
				opt_sts=CMM_ProcessUpgradeFtp(this);
				break;
			}
			case CMM_CLI_SEND_CONFIG:
			{
				opt_sts=CMM_ProcessReloadCnuProfile(this);
				break;
			}
			case CMM_CLI_USER_NEW:
			{
				opt_sts=CMM_ProcessUserPermit(this);
				break;
			}
			case CMM_CLI_USER_DEL:
			{
				opt_sts=CMM_ProcessUndoUserPermit(this);
				break;
			}
			case CMM_CLI_DELETE_USER:
			{
				opt_sts=CMM_ProcessDeleteCnu(this);
				break;
			}
			case CMM_DO_WLIST_CONTROL:
			case CMM_UNDO_WLIST_CONTROL:
			{
				opt_sts=CMM_ProcessDoWlistControl(this);
				break;
			}
			case CMM_DO_WDT_CONTROL:
			case CMM_UNDO_WDT_CONTROL:
			{
				opt_sts=CMM_ProcessDoWdtControl(this);
				break;
			}
			case CMM_CREATE_CNU:
			{
				opt_sts=CMM_ProcessCreateCnu(this);
				break;
			}
			case CMM_DUMP_CNU_REG:
			{
				opt_sts=CMM_ProcessDumpCnuReg(this);
				break;
			}
			case CMM_DUMP_CNU_MOD:
			{
				opt_sts=CMM_ProcessDumpCnuMod(this);
				break;
			}
			case CMM_DUMP_CNU_PIB:
			{
				opt_sts=CMM_ProcessDumpCnuPib(this);
				break;
			}
			case CMM_DO_LINK_DIAG:
			{
				opt_sts=CMM_ProcessLinkDiag(this);
				break;
			}
			case CMM_GET_IP175D_PORT_PROPETY:
			{
				opt_sts=CMM_ProcessGetPortPropety(this);
				break;
			}
			case CMM_GET_PORT_STAT:
			{
				opt_sts=CMM_ProcessGetPortStats(this);
				break;
			}
			case CMM_CLEAR_PORT_STAT:
			{
				opt_sts = CMM_ProcessClearPortStats(this);
				break;
			}
			case CMM_DEBUG_PRINT_PORT_STAT:
			{
				opt_sts=CMM_ProcessDebugPrintPortStats(this);
				break;
			}
			case CMM_GET_DSDT_RGMII_DELAY:
			{
				opt_sts=CMM_ProcessGetDsdtRgmiiDelay(this);
				break;
			}
			case CMM_SET_DSDT_RGMII_DELAY:
			{
				opt_sts=CMM_ProcessSetDsdtRgmiiDelay(this);
				break;
			}
#ifdef __AT30TK175STK__
			case CMM_GET_CBAT_TEMPERATURE:
			{
				opt_sts=CMM_ProcessGetCbatTemperature(this);
				break;
			}
#endif
			case CMM_SET_DSDT_PORT_MIRRORING:
			{
				opt_sts=CMM_ProcessSetDsdtPortMirroring(this);
				break;
			}
			case CMM_CONNET:
			{
				opt_sts = CMM_ProcessMTConnect(this);
				break;
			}
			case CMM_TOOL_MAC:
			{
				opt_sts = CMM_ProcessMTProgram(this);
				break;
			}
			default:
			{
				/* ���ڲ�֧�ֵ���Ϣ����Ӧ�ø���Ӧ���Ա���������֪�� */
				CMM_ProcessAck(CMM_UNKNOWN_MMTYPE, this, NULL, 0);
				opt_sts = CMM_UNKNOWN_MMTYPE;
				break;
			}
		}
		CMM_WriteOptLog(this, opt_sts);
	}
}

int init_socket_cmm(void)
{
	struct sockaddr_in server_addr;
	T_UDP_SK_INFO *sk = &SK_CMM;

	/*����UDP SOCKET�ӿ�*/
	if( ( sk->sk = socket(PF_INET, SOCK_DGRAM, 0) ) < 0 )
	{
		return CMM_CREATE_SOCKET_ERROR;
	}

	/* CMMģ���ػ������׽���*/
	bzero((char *)&(sk->skaddr), sizeof(sk->skaddr));
   	server_addr.sin_family = PF_INET;
   	server_addr.sin_port = htons(CMM_LISTEN_PORT);		/* Ŀ�Ķ˿ں�*/
   	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);	/* Ŀ�ĵ�ַ*/
	if (bind(sk->sk, (struct sockaddr*)&server_addr, sizeof(server_addr))<0)
	{
		return CMM_CREATE_SOCKET_ERROR;
	}

	return CMM_SUCCESS;
}

int close_socket_cmm(void)
{
	T_UDP_SK_INFO *sk = &SK_CMM;
	if( sk->sk != 0)
	{
		close(sk->sk);
		sk->sk = 0;
	}
	return CMM_SUCCESS;
}

/********************************************************************************************
*	��������:cmmSignalProcessHandle
*	��������:�쳣�������������������˳�ʱ�ú���������
*	����:frank
*	ʱ��:2010-08-13
*********************************************************************************************/
void cmmSignalProcessHandle(int n)
{
	dbs_sys_log(dbsdev, DBS_LOG_INFO, "cmmSignalProcessHandle : module cmm exit");
	
	/* �ر�socket�ӿ� */
	cmm2alarm_destroy();
	destroy_cmm_reg();
	destroy_cmm_tm();
	destroy_cmm_mmead();
	close_socket_cmm();
	dbsClose(dbsdev);
#ifdef __AT30TK175STK__
	uninit_at30ts75();
#endif
	exit(0);
}

/********************************************************************************************
*	��������:main
*	��������:CMM����ں��������������ⲿģ�������
*				   ���ҵ���߼��Ĵ���
*	����:frank
*	ʱ��:2010-08-13
*********************************************************************************************/
int main(void)
{
	/*���������ݿ�ģ��ͨѶ���ⲿSOCKET�ӿ�*/
	dbsdev = dbsOpen(MID_CMM);
	if( NULL == dbsdev )
	{
		fprintf(stderr,"ERROR: cmm->dbsOpen error, exited !\n");
		return CMM_CREATE_SOCKET_ERROR;
	}
	
	/* �����ⲿͨѶUDP SOCKET */
	if( init_socket_cmm() != CMM_SUCCESS )
	{
		fprintf(stderr, "cmm->init_socket_cmm error, exited !\n");
		dbs_sys_log(dbsdev, DBS_LOG_EMERG, "module cmm init_socket_cmm error, exited !");
		dbsClose(dbsdev);
		return CMM_CREATE_SOCKET_ERROR;
	}
	
	/*������MMEADģ��ͨѶ���ⲿSOCKET�ӿ�*/
	if( init_cmm_mmead() != CMM_SUCCESS )
	{
		fprintf(stderr, "cmm->init_cmm_mmead error, exited !\n");
		dbs_sys_log(dbsdev, DBS_LOG_EMERG, "module cmm init_cmm_mmead error, exited !");
		close_socket_cmm();
		dbsClose(dbsdev);
		return CMM_CREATE_SOCKET_ERROR;
	}
	
	/*������TEMPLATEģ��ͨѶ���ⲿSOCKET�ӿ�*/
	if( init_cmm_tm() != CMM_SUCCESS )
	{
		fprintf(stderr, "cmm->init_cmm_tm error, exited !\n");
		dbs_sys_log(dbsdev, DBS_LOG_EMERG, "module cmm init_cmm_tm error, exited !");
		destroy_cmm_mmead();
		close_socket_cmm();
		dbsClose(dbsdev);
		return CMM_CREATE_SOCKET_ERROR;
	}
	
	/*������registerģ��ͨѶ���ⲿSOCKET�ӿ�*/
	if( init_cmm_reg() != CMM_SUCCESS )
	{
		fprintf(stderr, "cmm->init_cmm_reg error, exited !\n");
		dbs_sys_log(dbsdev, DBS_LOG_EMERG, "module cmm init_cmm_reg error, exited !");
		destroy_cmm_tm();
		destroy_cmm_mmead();
		close_socket_cmm();
		dbsClose(dbsdev);
		return CMM_CREATE_SOCKET_ERROR;
	}

	/*������alarmģ��ͨѶ���ⲿSOCKET�ӿ�*/
	if( cmm2alarm_init() != CMM_SUCCESS )
	{
		fprintf(stderr, "cmm->cmm2alarm_init error, exited !\n");
		dbs_sys_log(dbsdev, DBS_LOG_EMERG, "module cmm cmm2alarm_init error, exited !");
		destroy_cmm_reg();
		destroy_cmm_tm();
		destroy_cmm_mmead();
		close_socket_cmm();
		dbsClose(dbsdev);
		return CMM_CREATE_SOCKET_ERROR;
	}

	/*������sysMOnitorģ��ͨѶ���ⲿSOCKET�ӿ�*/
	if( cmm2sysmonitor_init() != CMM_SUCCESS )
	{
		fprintf(stderr, "cmm->cmm2sysmonitor_init error, exited !\n");
		dbs_sys_log(dbsdev, DBS_LOG_EMERG, "module cmm cmm2sysmonitor_init error, exited !");
		cmm2alarm_destroy();
		destroy_cmm_reg();
		destroy_cmm_tm();
		destroy_cmm_mmead();
		close_socket_cmm();
		dbsClose(dbsdev);
		return CMM_CREATE_SOCKET_ERROR;
	}

	/*��ʼ��DSDT*/
	if( cmm2dsdt_init() != CMM_SUCCESS )
	{
		fprintf(stderr, "cmm->cmm2dsdt_init error, exited !\n");
		dbs_sys_log(dbsdev, DBS_LOG_EMERG, "module cmm cmm2dsdt_init error, exited !");
		cmm2sysmonitor_destroy();
		cmm2alarm_destroy();
		destroy_cmm_reg();
		destroy_cmm_tm();
		destroy_cmm_mmead();
		close_socket_cmm();
		dbsClose(dbsdev);
		return CMM_FAILED;
	}
	if( cmm2dsdt_addAtherosMulticastAddressToAllCablePort() != CMM_SUCCESS )
	{
		fprintf(stderr, "cmm->init_network error, exited !\n");
		dbs_sys_log(dbsdev, DBS_LOG_EMERG, "module cmm init_network error, exited !");
		cmm2sysmonitor_destroy();
		cmm2alarm_destroy();
		destroy_cmm_reg();
		destroy_cmm_tm();
		destroy_cmm_mmead();
		close_socket_cmm();
		dbsClose(dbsdev);
		return CMM_FAILED;
	}
	else
	{
		fprintf(stderr, "cmm binding atheros multicast address\n");
		dbs_sys_log(dbsdev, DBS_LOG_INFO, "cmm add atheros multicast address to all cable port");
	}
	
	/*��ʼ��������Ϣ����*/
	if( init_network() != CMM_SUCCESS )
	{
		fprintf(stderr, "cmm->init_network error, exited !\n");
		dbs_sys_log(dbsdev, DBS_LOG_EMERG, "module cmm init_network error, exited !");
		cmm2sysmonitor_destroy();
		cmm2alarm_destroy();
		destroy_cmm_reg();
		destroy_cmm_tm();
		destroy_cmm_mmead();
		close_socket_cmm();
		dbsClose(dbsdev);
		return CMM_FAILED;
	}
	
#ifdef __AT30TK175STK__
	/* ��ʼ���¸�оƬ*/
	if( init_at30ts75() != CMM_SUCCESS )
	{
		fprintf(stderr, "cmm->init_at30ts75() failed\n");
		dbs_sys_log(dbsdev, DBS_LOG_EMERG, "module cmm init_at30ts75 failed");
	}
#endif

	/* ע���쳣�˳��������*/
	signal(SIGTERM, cmmSignalProcessHandle);	

	fprintf(stderr, "Starting module CMM		......		[OK]\n");
	dbs_sys_log(dbsdev, DBS_LOG_INFO, "starting module cmm success");
	
	/* ѭ�������ⲿ����*/
	cmmProcessManager();

	/* ��Ҫ�����������Ӵ��룬ִ�в�����*/
	/* �ر�socket�ӿ� */
	dbs_sys_log(dbsdev, DBS_LOG_INFO, "module cmm exit");	
	cmm2sysmonitor_destroy();
	cmm2alarm_destroy();
	destroy_cmm_reg();
	destroy_cmm_tm();
	destroy_cmm_mmead();
	close_socket_cmm();
	dbsClose(dbsdev);
#ifdef __AT30TK175STK__
	uninit_at30ts75();
#endif
	return 0;
}

