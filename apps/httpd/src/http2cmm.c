#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <wecplatform.h>
#include "syscall.h"
#include "http2cmm.h"
#include <http2dbs.h>
#include <dbsapi.h>
#include <boardapi.h>

static T_UDP_SK_INFO SK_HTTP2CMM;

int __mapSelectIndex2Rate(int index)
{
	switch(index)
	{
		case 0:
		{
			return 0;
		}
		case 1:
		{
			return 128;
		}
		case 2:
		{
			return 256;
		}
		case 3:
		{
			return 512;
		}
		case 4:
		{
			return 1024;
		}
		case 5:
		{
			return 1024+512;
		}
		case 6:
		{
			return 2048;
		}
		case 7:
		{
			return 3*1024;
		}
		case 8:
		{
			return 4*1024;
		}
		case 9:
		{
			return 6*1024;
		}
		case 10:
		{
			return 8*1024;
		}
		default:
		{
			return 0;
		}
	}
}

int __getCnuPortStatus(uint16_t id, uint16_t port)
{
	st_dbsCnu cnu;
	st_dbsProfile profile;

	if( (id < 1)||(id > MAX_CNU_AMOUNT_LIMIT) )
	{
		return 0;
	}

	if( port > 4 )
	{
		return 0;
	}

	if( CMM_SUCCESS != dbsGetCnu(dbsdev, id, &cnu) )
	{
		return 0;
	}	
	else if( 0 == cnu.col_row_sts )
	{
		return 0;
	}

	if( CMM_SUCCESS != dbsGetProfile(dbsdev, id,  &profile) )
	{
		return 0;
	}	
	else if( 0 == profile.col_row_sts )
	{
		return 0;
	}

	if( 0 == profile.col_psctlSts )
	{
		return 1;
	}
	else
	{
		switch(port)
		{
			case 0:
			{
				return profile.col_cpuPortSts?1:0;
			}
			case 1:
			{
				return profile.col_eth1sts?1:0;
			}
			case 2:
			{
				return profile.col_eth2sts?1:0;
			}
			case 3:
			{
				return profile.col_eth3sts?1:0;
			}
			case 4:
			{
				return profile.col_eth4sts?1:0;
			}
			default:
			{
				return 0;
			}
		}
	}
}

int __http2cmm_comm(uint8_t *buf, uint32_t len)
{
	T_UDP_SK_INFO *sk = &SK_HTTP2CMM;
	T_Msg_CMM *req = (T_Msg_CMM *)buf;
	uint16_t msgType = req->HEADER.usMsgType;
	T_REQ_Msg_CMM *ack = NULL;

	fd_set fdsr;
	//int maxsock;
	struct timeval tv;	
	int ret = 0;
	int sendn = 0;	
	struct sockaddr_in from;	
	int FromAddrSize = 0;
	int rev_len = 0;

	sendn = sendto(sk->sk, buf, len, 0, (struct sockaddr *)&(sk->skaddr), sizeof(struct sockaddr));
	if ( -1 == sendn )
	{
		dbs_sys_log(dbsdev, DBS_LOG_CRIT, "httpd call __http2cmm_comm sendto failed");
		return CMM_FAILED;
	}

	while(1)
	{
		// initialize file descriptor set
		FD_ZERO(&fdsr);
		FD_SET(sk->sk, &fdsr);

		// timeout setting
		tv.tv_sec = 18;
		tv.tv_usec = 0;

		//检测socket
		ret = select(sk->sk + 1, &fdsr, NULL, NULL, &tv);
		if( ret <= 0 )
		{
			dbs_sys_log(dbsdev, DBS_LOG_CRIT, "httpd call __http2cmm_comm select failed");
			return CMM_FAILED;
		}
		// check whether a new connection comes
		if (FD_ISSET(sk->sk, &fdsr))
		{
			FromAddrSize = sizeof(from);
			rev_len = recvfrom(sk->sk, buf, MAX_UDP_SIZE, 0, (struct sockaddr *)&from, &FromAddrSize);
			if ( -1 == rev_len )
			{
				dbs_sys_log(dbsdev, DBS_LOG_CRIT, "httpd call __http2cmm_comm recvfrom failed");
				return CMM_FAILED;
			}
			else
			{
				ack = (T_REQ_Msg_CMM *)buf;
				if( msgType != ack->HEADER.usMsgType )
				{
					fprintf(stderr, "WARNNING: __http2cmm_comm: msgType[%d!=%d], [continue] !\n", 
						ack->HEADER.usMsgType, msgType);
					dbs_sys_log(dbsdev, DBS_LOG_WARNING, "__http2cmm_comm received non-mached msg type");
					continue;
				}
				else if( ack->result != CMM_SUCCESS )
				{	
					dbs_sys_log(dbsdev, DBS_LOG_CRIT, "httpd call __http2cmm_comm recvfrom result error");
					return  ack->result ;
				}
				else
				{
					return ack->result;
				}
			}
		}
		else
		{
			dbs_sys_log(dbsdev, DBS_LOG_CRIT, "httpd call __http2cmm_comm FD_ISSET failed");
			return CMM_FAILED;
		}
	}
}

int http2cmm_rebootClt(int id)
{
	uint8_t buf[MAX_UDP_SIZE] = {0};
	uint32_t len = 0;
	T_Msg_CMM *req = (T_Msg_CMM *)buf;
	uint16_t *index = (uint16_t *)(req->BUF);

	req->HEADER.usSrcMID = MID_HTTP;
	req->HEADER.usDstMID = MID_CMM;
	req->HEADER.usMsgType = CMM_CLT_RESET;
	req->HEADER.ulBodyLength = sizeof(uint16_t);
	req->HEADER.fragment = 0;

	*index = id;

	len = sizeof(req->HEADER) + req->HEADER.ulBodyLength;
	return __http2cmm_comm(buf, len);
}

int http2cmm_reloadClt(int id)
{
	/* 暂时没有实现*/
	return CMM_SUCCESS;
}


int http2cmm_rebootCnu(int id)
{
	uint8_t buf[MAX_UDP_SIZE] = {0};
	uint32_t len = 0;
	T_Msg_CMM *req = (T_Msg_CMM *)buf;
	uint16_t *index = (uint16_t *)(req->BUF);

	req->HEADER.usSrcMID = MID_HTTP;
	req->HEADER.usDstMID = MID_CMM;
	req->HEADER.usMsgType = CMM_CNU_RESET;
	req->HEADER.ulBodyLength = sizeof(uint16_t);
	req->HEADER.fragment = 0;

	*index = id;

	len = sizeof(req->HEADER) + req->HEADER.ulBodyLength;
	return __http2cmm_comm(buf, len);
}

int http2cmm_reloadCnu(int id)
{
	uint8_t buf[MAX_UDP_SIZE] = {0};
	uint32_t len = 0;	
	T_Msg_CMM *req = (T_Msg_CMM *)buf;
	stTmUserInfo szNode;

	szNode.clt = (id-1)/MAX_CNUS_PER_CLT + 1;
	szNode.cnu = (id-1)%MAX_CNUS_PER_CLT +1;

	req->HEADER.usSrcMID = MID_HTTP;
	req->HEADER.usDstMID = MID_CMM;
	req->HEADER.usMsgType = CMM_CLI_SEND_CONFIG;
	req->HEADER.ulBodyLength = sizeof(stTmUserInfo);
	req->HEADER.fragment = 0;

	len = sizeof(req->HEADER) + req->HEADER.ulBodyLength;
	if( len > MAX_UDP_SIZE )
	{
		return CMM_FAILED;
	}

	memcpy(req->BUF, &szNode, req->HEADER.ulBodyLength);

	return __http2cmm_comm(buf, len);
}

int http2cmm_deleteCnu(int id)
{
	uint8_t buf[MAX_UDP_SIZE] = {0};
	uint32_t len = 0;	
	T_Msg_CMM *req = (T_Msg_CMM *)buf;
	stTmUserInfo szNode;

	szNode.clt = (id-1)/MAX_CNUS_PER_CLT + 1;
	szNode.cnu = (id-1)%MAX_CNUS_PER_CLT +1;
	
	req->HEADER.usSrcMID = MID_HTTP;
	req->HEADER.usDstMID = MID_CMM;
	req->HEADER.usMsgType = CMM_CLI_DELETE_USER;
	req->HEADER.ulBodyLength = sizeof(stTmUserInfo);
	req->HEADER.fragment = 0;

	len = sizeof(req->HEADER) + req->HEADER.ulBodyLength;
	if( len > MAX_UDP_SIZE )
	{
		return CMM_FAILED;
	}

	memcpy(req->BUF, &szNode, req->HEADER.ulBodyLength);

	return __http2cmm_comm(buf, len);
}

int http2cmm_permitCnu(int id)
{
	uint8_t buf[MAX_UDP_SIZE] = {0};
	uint32_t len = 0;	
	T_Msg_CMM *req = (T_Msg_CMM *)buf;
	stTmUserInfo szNode;

	szNode.clt = (id-1)/MAX_CNUS_PER_CLT + 1;
	szNode.cnu = (id-1)%MAX_CNUS_PER_CLT +1;

	req->HEADER.usSrcMID = MID_HTTP;
	req->HEADER.usDstMID = MID_CMM;
	req->HEADER.usMsgType = CMM_CLI_USER_NEW;
	req->HEADER.ulBodyLength = sizeof(stTmUserInfo);
	req->HEADER.fragment = 0;

	len = sizeof(req->HEADER) + req->HEADER.ulBodyLength;
	if( len > MAX_UDP_SIZE )
	{
		return CMM_FAILED;
	}

	memcpy(req->BUF, &szNode, req->HEADER.ulBodyLength);

	return __http2cmm_comm(buf, len);
}

int http2cmm_undoPermitCnu(int id)
{
	uint8_t buf[MAX_UDP_SIZE] = {0};
	uint32_t len = 0;	
	T_Msg_CMM *req = (T_Msg_CMM *)buf;
	stTmUserInfo szNode;

	szNode.clt = (id-1)/MAX_CNUS_PER_CLT + 1;
	szNode.cnu = (id-1)%MAX_CNUS_PER_CLT +1;

	req->HEADER.usSrcMID = MID_HTTP;
	req->HEADER.usDstMID = MID_CMM;
	req->HEADER.usMsgType = CMM_CLI_USER_DEL;
	req->HEADER.ulBodyLength = sizeof(stTmUserInfo);
	req->HEADER.fragment = 0;

	len = sizeof(req->HEADER) + req->HEADER.ulBodyLength;
	if( len > MAX_UDP_SIZE )
	{
		return CMM_FAILED;
	}

	memcpy(req->BUF, &szNode, req->HEADER.ulBodyLength);

	return __http2cmm_comm(buf, len);
}

#if 0
int http2cmm_getEth1Stat(PWEB_NTWK_VAR pWebVar)
{
	uint8_t buf[MAX_UDP_SIZE] = {0};
	uint32_t len = 0;
	
	T_Msg_CMM *req = (T_Msg_CMM *)buf;
	uint32_t *req_data = (uint32_t *)(req->BUF);
	
	T_REQ_Msg_CMM *ack = (T_REQ_Msg_CMM *)buf;
	T_CMM_PORT_STATS_INFO *ack_data = (T_CMM_PORT_STATS_INFO *)(ack->BUF);

	req->HEADER.usSrcMID = MID_HTTP;
	req->HEADER.usDstMID = MID_CMM;
	req->HEADER.usMsgType = CMM_GET_PORT_STAT;
	req->HEADER.ulBodyLength = sizeof(uint32_t);
	req->HEADER.fragment = 0;

	*req_data = PORT_ETH1_PORT_ID;

	len = sizeof(req->HEADER) + req->HEADER.ulBodyLength;

	if( CMM_SUCCESS == __http2cmm_comm(buf, len) )
	{
		pWebVar->eth1rxbc = ack_data->InBroadcasts;
		pWebVar->eth1rxm = ack_data->InMulticasts;
		pWebVar->eth1rxu = ack_data->InUnicasts;
		pWebVar->eth1rxp = ack_data->rxCtr;
		pWebVar->eth1rxb = ack_data->InGoodOctets;
		pWebVar->eth1txbc = ack_data->OutBroadcasts;
		pWebVar->eth1txm = ack_data->OutMulticasts;
		pWebVar->eth1txu = ack_data->OutUnicasts;
		pWebVar->eth1txp = ack_data->txCtr;
		pWebVar->eth1txb = ack_data->OutGoodOctets;
	}	
	return CMM_SUCCESS;
}

int http2cmm_getEth2Stat(PWEB_NTWK_VAR pWebVar)
{
	uint8_t buf[MAX_UDP_SIZE] = {0};
	uint32_t len = 0;
	
	T_Msg_CMM *req = (T_Msg_CMM *)buf;
	uint32_t *req_data = (uint32_t *)(req->BUF);
	
	T_REQ_Msg_CMM *ack = (T_REQ_Msg_CMM *)buf;
	T_CMM_PORT_STATS_INFO *ack_data = (T_CMM_PORT_STATS_INFO *)(ack->BUF);

	req->HEADER.usSrcMID = MID_HTTP;
	req->HEADER.usDstMID = MID_CMM;
	req->HEADER.usMsgType = CMM_GET_PORT_STAT;
	req->HEADER.ulBodyLength = sizeof(uint32_t);
	req->HEADER.fragment = 0;

	*req_data = PORT_ETH2_PORT_ID;

	len = sizeof(req->HEADER) + req->HEADER.ulBodyLength;

	if( CMM_SUCCESS == __http2cmm_comm(buf, len) )
	{
		pWebVar->eth2rxbc = ack_data->InBroadcasts;
		pWebVar->eth2rxm = ack_data->InMulticasts;
		pWebVar->eth2rxu = ack_data->InUnicasts;
		pWebVar->eth2rxp = ack_data->rxCtr;
		pWebVar->eth2rxb = ack_data->InGoodOctets;
		pWebVar->eth2txbc = ack_data->OutBroadcasts;
		pWebVar->eth2txm = ack_data->OutMulticasts;
		pWebVar->eth2txu = ack_data->OutUnicasts;
		pWebVar->eth2txp = ack_data->txCtr;
		pWebVar->eth2txb = ack_data->OutGoodOctets;
	}	
	return CMM_SUCCESS;
}

int http2cmm_getCable1Stat(PWEB_NTWK_VAR pWebVar)
{
	uint8_t buf[MAX_UDP_SIZE] = {0};
	uint32_t len = 0;
	
	T_Msg_CMM *req = (T_Msg_CMM *)buf;
	uint32_t *req_data = (uint32_t *)(req->BUF);
	
	T_REQ_Msg_CMM *ack = (T_REQ_Msg_CMM *)buf;
	T_CMM_PORT_STATS_INFO *ack_data = (T_CMM_PORT_STATS_INFO *)(ack->BUF);

	req->HEADER.usSrcMID = MID_HTTP;
	req->HEADER.usDstMID = MID_CMM;
	req->HEADER.usMsgType = CMM_GET_PORT_STAT;
	req->HEADER.ulBodyLength = sizeof(uint32_t);
	req->HEADER.fragment = 0;

	*req_data = PORT_CABLE1_PORT_ID;

	len = sizeof(req->HEADER) + req->HEADER.ulBodyLength;

	if( CMM_SUCCESS == __http2cmm_comm(buf, len) )
	{
		pWebVar->eth3rxbc = ack_data->InBroadcasts;
		pWebVar->eth3rxm = ack_data->InMulticasts;
		pWebVar->eth3rxu = ack_data->InUnicasts;
		pWebVar->eth3rxp = ack_data->rxCtr;
		pWebVar->eth3rxb = ack_data->InGoodOctets;
		pWebVar->eth3txbc = ack_data->OutBroadcasts;
		pWebVar->eth3txm = ack_data->OutMulticasts;
		pWebVar->eth3txu = ack_data->OutUnicasts;
		pWebVar->eth3txp = ack_data->txCtr;
		pWebVar->eth3txb = ack_data->OutGoodOctets;
	}	
	return CMM_SUCCESS;
}

int http2cmm_doPortStas(PWEB_NTWK_VAR pWebVar)
{	
	http2cmm_getEth1Stat(pWebVar);
	http2cmm_getEth2Stat(pWebVar);
	http2cmm_getCable1Stat(pWebVar);
	//http2cmm_getCable2Stat(pWebVar);
	return CMM_SUCCESS;
}



int http2cmm_clearPortStas(PWEB_NTWK_VAR pWebVar)
{
	pWebVar->eth1rxbc = 0;
	pWebVar->eth1rxm = 0;
	pWebVar->eth1rxu = 0;
	pWebVar->eth1rxp = 0;
	pWebVar->eth1rxb = 0;
	pWebVar->eth1txbc = 0;
	pWebVar->eth1txm = 0;
	pWebVar->eth1txu = 0;
	pWebVar->eth1txp = 0;
	pWebVar->eth1txb = 0;

	pWebVar->eth2rxbc = 0;
	pWebVar->eth2rxm = 0;
	pWebVar->eth2rxu = 0;
	pWebVar->eth2rxp = 0;
	pWebVar->eth2rxb = 0;
	pWebVar->eth2txbc = 0;
	pWebVar->eth2txm = 0;
	pWebVar->eth2txu = 0;
	pWebVar->eth2txp = 0;
	pWebVar->eth2txb = 0;

	pWebVar->eth3rxbc = 0;
	pWebVar->eth3rxm = 0;
	pWebVar->eth3rxu = 0;
	pWebVar->eth3rxp = 0;
	pWebVar->eth3rxb = 0;
	pWebVar->eth3txbc = 0;
	pWebVar->eth3txm = 0;
	pWebVar->eth3txu = 0;
	pWebVar->eth3txp = 0;
	pWebVar->eth3txb = 0;

	pWebVar->eth4rxbc = 0;
	pWebVar->eth4rxm = 0;
	pWebVar->eth4rxu = 0;
	pWebVar->eth4rxp = 0;
	pWebVar->eth4rxb = 0;
	pWebVar->eth4txbc = 0;
	pWebVar->eth4txm = 0;
	pWebVar->eth4txu = 0;
	pWebVar->eth4txp = 0;
	pWebVar->eth4txb = 0;
	
	return http2cmm_clearPortStats();
}

#endif

int http2cmm_readSwitchSettings(PWEB_NTWK_VAR pWebVar)
{
	uint8_t buf[MAX_UDP_SIZE] = {0};
	uint32_t len = 0;

	stTmUserInfo szNode;	
	
	T_Msg_CMM *req = (T_Msg_CMM *)buf;
	stTmUserInfo *req_data = (stTmUserInfo *)(req->BUF);
	
	T_REQ_Msg_CMM *ack = (T_REQ_Msg_CMM *)buf;
	st_rtl8306eSettings *ack_data = (st_rtl8306eSettings *)(ack->BUF);

	req->HEADER.usSrcMID = MID_HTTP;
	req->HEADER.usDstMID = MID_CMM;
	req->HEADER.usMsgType = CMM_CNU_SWITCH_CONFIG_READ;
	req->HEADER.ulBodyLength = sizeof(stTmUserInfo);
	req->HEADER.fragment = 0;

	req_data->clt = pWebVar->cltid;
	req_data->cnu = pWebVar->cnuid;

	len = sizeof(req->HEADER) + req->HEADER.ulBodyLength;

	if( CMM_SUCCESS == __http2cmm_comm(buf, len) )
	{
		//802.1q Vlan Status
		if( (1==ack_data->vlanConfig.vlan_enable)&&(1==ack_data->vlanConfig.vlan_tag_aware))
		{
			pWebVar->swVlanEnable = 1; //enable
		}
		else
		{
			pWebVar->swVlanEnable = 0; //disable
		}
		//port egress mode
		switch(ack_data->vlanConfig.vlan_port[0].egress_mode)
		{
			case 0:	/* add new tag */
			{
				pWebVar->swEth1PortVMode = 0;	/* Transparent */
				break;
			}
			case 1:
			{
				pWebVar->swEth1PortVMode = 1;	/* untag */
				break;
			}
			case 2:
			{
				pWebVar->swEth1PortVMode = 2;	/* tag */
				break;
			}
			default:
			{
				pWebVar->swEth1PortVMode = 0;	/* Transparent */
				break;
			}
		}
		switch(ack_data->vlanConfig.vlan_port[1].egress_mode)
		{
			case 0:
			{
				pWebVar->swEth2PortVMode = 0;
				break;
			}
			case 1:
			{
				pWebVar->swEth2PortVMode = 1;
				break;
			}
			case 2:
			{
				pWebVar->swEth2PortVMode = 2;
				break;
			}
			default:
			{
				pWebVar->swEth2PortVMode = 0;
				break;
			}
		}
		switch(ack_data->vlanConfig.vlan_port[2].egress_mode)
		{
			case 0:
			{
				pWebVar->swEth3PortVMode = 0;
				break;
			}
			case 1:
			{
				pWebVar->swEth3PortVMode = 1;
				break;
			}
			case 2:
			{
				pWebVar->swEth3PortVMode = 2;
				break;
			}
			default:
			{
				pWebVar->swEth3PortVMode = 0;
				break;
			}
		}
		switch(ack_data->vlanConfig.vlan_port[3].egress_mode)
		{
			case 0:
			{
				pWebVar->swEth4PortVMode = 0;
				break;
			}
			case 1:
			{
				pWebVar->swEth4PortVMode = 1;
				break;
			}
			case 2:
			{
				pWebVar->swEth4PortVMode = 2;
				break;
			}
			default:
			{
				pWebVar->swEth4PortVMode = 0;
				break;
			}
		}
		switch(ack_data->vlanConfig.vlan_port[4].egress_mode)
		{
			case 0:
			{
				pWebVar->swUplinkPortVMode = 0;
				break;
			}
			case 1:
			{
				pWebVar->swUplinkPortVMode = 1;
				break;
			}
			case 2:
			{
				pWebVar->swUplinkPortVMode = 2;
				break;
			}
			default:
			{
				pWebVar->swUplinkPortVMode = 0;
				break;
			}
		}
		//pvid
		pWebVar->swEth1PortVid = ack_data->vlanConfig.vlan_port[0].pvid;
		pWebVar->swEth2PortVid = ack_data->vlanConfig.vlan_port[1].pvid;
		pWebVar->swEth3PortVid = ack_data->vlanConfig.vlan_port[2].pvid;
		pWebVar->swEth4PortVid = ack_data->vlanConfig.vlan_port[3].pvid;
		pWebVar->swUplinkPortVid = ack_data->vlanConfig.vlan_port[4].pvid;

		//bandwidth config
		pWebVar->swRxRateLimitEnable = ack_data->bandwidthConfig.g_rx_bandwidth_control_enable;
		pWebVar->swTxRateLimitEnable = ack_data->bandwidthConfig.g_tx_bandwidth_control_enable;
		pWebVar->swEth1RxRate = ack_data->bandwidthConfig.rxPort[0].bandwidth_value;
		pWebVar->swEth2RxRate = ack_data->bandwidthConfig.rxPort[1].bandwidth_value;
		pWebVar->swEth3RxRate = ack_data->bandwidthConfig.rxPort[2].bandwidth_value;
		pWebVar->swEth4RxRate = ack_data->bandwidthConfig.rxPort[3].bandwidth_value;
		pWebVar->swUplinkRxRate = ack_data->bandwidthConfig.rxPort[4].bandwidth_value;
		pWebVar->swEth1TxRate = ack_data->bandwidthConfig.txPort[0].bandwidth_value;
		pWebVar->swEth2TxRate = ack_data->bandwidthConfig.txPort[1].bandwidth_value;
		pWebVar->swEth3TxRate = ack_data->bandwidthConfig.txPort[2].bandwidth_value;
		pWebVar->swEth4TxRate = ack_data->bandwidthConfig.txPort[3].bandwidth_value;
		pWebVar->swUplinkTxRate = ack_data->bandwidthConfig.txPort[4].bandwidth_value;

		//loopdetect
		pWebVar->swLoopDetect = ack_data->loopDetect.status;
		sprintf(pWebVar->swSwitchSid, "%02X:%02X:%02X:%02X:%02X:%02X", 
			ack_data->loopDetect.sid[0], ack_data->loopDetect.sid[1], 
			ack_data->loopDetect.sid[2], ack_data->loopDetect.sid[3],
			ack_data->loopDetect.sid[4], ack_data->loopDetect.sid[5]
		);
		pWebVar->swldmethod = ack_data->loopDetect.ldmethod;
		pWebVar->swldtime = ack_data->loopDetect.ldtime;
		pWebVar->swldbckfrq = ack_data->loopDetect.ldbckfrq;
		pWebVar->swldsclr = ack_data->loopDetect.ldsclr;
		pWebVar->swpabuzzer = ack_data->loopDetect.pabuzzer;
		pWebVar->swentaglf = ack_data->loopDetect.entaglf;
		pWebVar->swlpttlinit = ack_data->loopDetect.lpttlinit;
		pWebVar->swlpfpri = ack_data->loopDetect.lpfpri;
		pWebVar->swenlpfpri = ack_data->loopDetect.enlpfpri;
		pWebVar->swdisfltlf = ack_data->loopDetect.disfltlf;
		pWebVar->swenlpttl = ack_data->loopDetect.enlpttl;
		pWebVar->swEth1LoopStatus = ack_data->loopDetect.port_loop_status[0];
		pWebVar->swEth2LoopStatus = ack_data->loopDetect.port_loop_status[1];
		pWebVar->swEth3LoopStatus = ack_data->loopDetect.port_loop_status[2];
		pWebVar->swEth4LoopStatus = ack_data->loopDetect.port_loop_status[3];

		//storm filter
		pWebVar->swSfDisBroadcast = ack_data->stormFilter.disable_broadcast;
		pWebVar->swSfDisMulticast = ack_data->stormFilter.disable_multicast;
		pWebVar->swSfDisUnknown = ack_data->stormFilter.disable_unknown;
		pWebVar->swSfRule = ack_data->stormFilter.rule;
		pWebVar->swSfResetSrc = ack_data->stormFilter.reset_source;
		pWebVar->swSfIteration = ack_data->stormFilter.iteration;
		pWebVar->swSfThresholt = ack_data->stormFilter.thresholt;

		//mac limit
		pWebVar->swMlSysEnable = ack_data->macLimit.system.enable;
		pWebVar->swMlSysThresholt = ack_data->macLimit.system.thresholt;
		pWebVar->swMlEth1Enable = ack_data->macLimit.port[0].enable;
		pWebVar->swMlEth1Thresholt = ack_data->macLimit.port[0].thresholt;
		pWebVar->swMlEth2Enable = ack_data->macLimit.port[1].enable;
		pWebVar->swMlEth2Thresholt = ack_data->macLimit.port[1].thresholt;
		pWebVar->swMlEth3Enable = ack_data->macLimit.port[2].enable;
		pWebVar->swMlEth3Thresholt = ack_data->macLimit.port[2].thresholt;
		pWebVar->swMlEth4Enable = ack_data->macLimit.port[3].enable;
		pWebVar->swMlEth4Thresholt = ack_data->macLimit.port[3].thresholt;

		//port control
		pWebVar->cnuPermition = ack_data->portControl.port[4].enable;
		pWebVar->col_eth1sts = ack_data->portControl.port[0].enable;
		pWebVar->col_eth2sts = ack_data->portControl.port[1].enable;
		pWebVar->col_eth3sts = ack_data->portControl.port[2].enable;
		pWebVar->col_eth4sts = ack_data->portControl.port[3].enable;

		return CMM_SUCCESS;
	}	
	return CMM_FAILED;
}

int http2cmm_writeSwitchSettings(PWEB_NTWK_VAR pWebVar)
{	
	uint8_t buf[MAX_UDP_SIZE] = {0};
	uint32_t len = 0;
	int i = 0;
	uint8_t tmp;
	
	T_Msg_CMM *req = (T_Msg_CMM *)buf;
	rtl8306eWriteInfo *req_data = (rtl8306eWriteInfo *)(req->BUF);
	
	//T_REQ_Msg_CMM *ack = (T_REQ_Msg_CMM *)buf;

	/* add by stan for save dbs  begin */
	st_dbsProfile profile;
	

	/* 如果该PROFILE 槽位无效则禁止配置*/
	if( CMM_SUCCESS != dbsGetProfile(dbsdev, pWebVar->cnuid,  &profile) )
	{
		return CMM_FAILED;
	}	
	else if( 0 == profile.col_row_sts )
	{
		return CMM_FAILED;
	}

	profile.col_vlanSts = (uint32_t)pWebVar->swVlanEnable;
	if( profile.col_vlanSts == 0 )
	{
		profile.col_eth1vid = 1;
		profile.col_eth2vid = 1;
		profile.col_eth3vid = 1;
		profile.col_eth4vid = 1;
		pWebVar->swEth1PortVid = 1;
		pWebVar->swEth2PortVid = 1;
		pWebVar->swEth3PortVid = 1;
		pWebVar->swEth4PortVid = 1;
	}
	else
	{
		profile.col_eth1vid = pWebVar->swEth1PortVid;
		profile.col_eth2vid = pWebVar->swEth2PortVid;
		profile.col_eth3vid = pWebVar->swEth3PortVid;
		profile.col_eth4vid = pWebVar->swEth4PortVid;
	}

	/* 防止端口PVID 被设置为0 */
	if( 0 == profile.col_eth1vid )
	{
		profile.col_eth1vid = 1;
		pWebVar->col_eth1vid = 1;
	}
	if( 0 == profile.col_eth2vid )
	{
		profile.col_eth2vid = 1;
		pWebVar->col_eth2vid = 1;
	}
	if( 0 == profile.col_eth3vid )
	{
		profile.col_eth3vid = 1;
		pWebVar->col_eth3vid = 1;
	}
	if( 0 == profile.col_eth4vid )
	{
		profile.col_eth4vid = 1;
		pWebVar->col_eth4vid = 1;
	}

	/* CHECK VID */
	if( (profile.col_eth1vid < 1) || (profile.col_eth1vid > 4094) )
	{
		return CMM_FAILED;
	}
	if( (profile.col_eth2vid < 1) || (profile.col_eth2vid > 4094) )
	{
		return CMM_FAILED;
	}
	if( (profile.col_eth3vid < 1) || (profile.col_eth3vid > 4094) )
	{
		return CMM_FAILED;
	}
	if( (profile.col_eth4vid < 1) || (profile.col_eth4vid > 4094) )
	{
		return CMM_FAILED;
	}	
	profile.col_rxLimitSts = pWebVar->swRxRateLimitEnable;
	profile.col_txLimitSts = pWebVar->swTxRateLimitEnable;
	profile.col_eth1rx = pWebVar->swEth1RxRate;
	profile.col_eth2rx = pWebVar->swEth2RxRate;
	profile.col_eth3rx = pWebVar->swEth3RxRate;
	profile.col_eth4rx = pWebVar->swEth4RxRate;
	profile.col_cpuPortRxRate = pWebVar->swUplinkRxRate;
	profile.col_eth1tx = pWebVar->swEth1TxRate;
	profile.col_eth2tx = pWebVar->swEth2TxRate;
	profile.col_eth3tx = pWebVar->swEth3TxRate;
	profile.col_eth4tx = pWebVar->swEth4TxRate;
	profile.col_cpuPortTxRate = pWebVar->swUplinkTxRate;
	profile.col_cpuPortSts = pWebVar->cnuPermition;
	profile.col_eth1sts = pWebVar->col_eth1sts;
	profile.col_eth2sts = pWebVar->col_eth2sts;
	profile.col_eth3sts = pWebVar->col_eth3sts;
	profile.col_eth4sts = pWebVar->col_eth4sts;
	profile.col_sfbSts = !(pWebVar->swSfDisBroadcast);
	profile.col_sfuSts = !(pWebVar->swSfDisUnknown);
	profile.col_sfmSts = !(pWebVar->swSfDisMulticast);
	profile.col_macLimit = pWebVar->swMlSysThresholt;
	profile.col_uplinkvid = pWebVar->swUplinkPortVid;
	profile.col_eth1VMode = pWebVar->swEth1PortVMode;
	profile.col_eth2VMode = pWebVar->swEth2PortVMode;
	profile.col_eth3VMode = pWebVar->swEth3PortVMode;
	profile.col_eth4VMode = pWebVar->swEth4PortVMode;
	profile.col_uplinkVMode = pWebVar->swUplinkPortVMode;
	/* add by stan for save dbs  end */

	req->HEADER.usSrcMID = MID_HTTP;
	req->HEADER.usDstMID = MID_CMM;
	req->HEADER.usMsgType = CMM_CNU_SWITCH_CONFIG_WRITE;
	req->HEADER.ulBodyLength = sizeof(rtl8306eWriteInfo) + sizeof(st_dbsProfile);
	req->HEADER.fragment = 0;

	/*add by stan */
	len = sizeof(req->HEADER) + req->HEADER.ulBodyLength;
	if( len > MAX_UDP_SIZE )
	{
		printf("add by stan too len rtl8306 body + profile len\n");
		return CMM_FAILED;
	}

	memcpy(req->BUF + sizeof(rtl8306eWriteInfo), &profile, sizeof(st_dbsProfile));

	/*add by stan end  */

	req_data->node.clt = pWebVar->cltid;
	req_data->node.cnu = pWebVar->cnuid;
	//port control
	req_data->rtl8306eConfig.portControl.port[4].enable = pWebVar->cnuPermition;
	req_data->rtl8306eConfig.portControl.port[0].enable = pWebVar->col_eth1sts;
	req_data->rtl8306eConfig.portControl.port[1].enable = pWebVar->col_eth2sts;
	req_data->rtl8306eConfig.portControl.port[2].enable = pWebVar->col_eth3sts;
	req_data->rtl8306eConfig.portControl.port[3].enable = pWebVar->col_eth4sts;

	//vlan enable
	req_data->rtl8306eConfig.vlanConfig.vlan_enable = pWebVar->swVlanEnable;
	//vlan tag aware enable
	req_data->rtl8306eConfig.vlanConfig.vlan_tag_aware = pWebVar->swVlanEnable;
	//VLAN member set ingress filtering
	//The switch will not drop the received frame if the ingress port of this packet is not included in the matched VLAN
	//member set. It will still forward the packet to the VLAN members specified in the matched member set
	req_data->rtl8306eConfig.vlanConfig.ingress_filter = 0;
	//The switch accepts all frames it receives whether tagged or untagged
	req_data->rtl8306eConfig.vlanConfig.g_admit_control = 0;
	//PVID
	req_data->rtl8306eConfig.vlanConfig.vlan_port[0].pvid = pWebVar->swEth1PortVid;
	req_data->rtl8306eConfig.vlanConfig.vlan_port[1].pvid = pWebVar->swEth2PortVid;
	req_data->rtl8306eConfig.vlanConfig.vlan_port[2].pvid = pWebVar->swEth3PortVid;
	req_data->rtl8306eConfig.vlanConfig.vlan_port[3].pvid = pWebVar->swEth4PortVid;
	req_data->rtl8306eConfig.vlanConfig.vlan_port[4].pvid = pWebVar->swUplinkPortVid;
	//port egress mode	
	req_data->rtl8306eConfig.vlanConfig.vlan_port[0].egress_mode = pWebVar->swEth1PortVMode;
	req_data->rtl8306eConfig.vlanConfig.vlan_port[1].egress_mode = pWebVar->swEth2PortVMode;
	req_data->rtl8306eConfig.vlanConfig.vlan_port[2].egress_mode = pWebVar->swEth3PortVMode;
	req_data->rtl8306eConfig.vlanConfig.vlan_port[3].egress_mode = pWebVar->swEth4PortVMode;
	req_data->rtl8306eConfig.vlanConfig.vlan_port[4].egress_mode = pWebVar->swUplinkPortVMode;
	//change
	for(i=0;i<=4;i++)
	{
		switch(req_data->rtl8306eConfig.vlanConfig.vlan_port[i].egress_mode)
		{
			case 0:		/* transparent */
			{
				tmp = 3;	/* transparent*/
				break;
			}
			case 1:		/* remove tag */
			{
				tmp = 1;	/* remove tag */
				break;
			}
			case 2:		/* tag */
			{
				tmp = 2;	/* tag */
				break;
			}
			default:
			{
				tmp = 3;	/* transparent */
				break;
			}
		}
		req_data->rtl8306eConfig.vlanConfig.vlan_port[i].egress_mode = tmp;
	}
	//port ingress filtering, Accept all frames whatever the frames is untagged or tagged
	req_data->rtl8306eConfig.vlanConfig.vlan_port[0].admit_control = 0;
	req_data->rtl8306eConfig.vlanConfig.vlan_port[1].admit_control = 0;
	req_data->rtl8306eConfig.vlanConfig.vlan_port[2].admit_control = 0;
	req_data->rtl8306eConfig.vlanConfig.vlan_port[3].admit_control = 0;
	req_data->rtl8306eConfig.vlanConfig.vlan_port[4].admit_control = 0;

	//bandwidth control settings
	req_data->rtl8306eConfig.bandwidthConfig.g_rx_bandwidth_control_enable = pWebVar->swRxRateLimitEnable;
	req_data->rtl8306eConfig.bandwidthConfig.rxPort[0].bandwidth_control_enable = pWebVar->swRxRateLimitEnable;
	req_data->rtl8306eConfig.bandwidthConfig.rxPort[1].bandwidth_control_enable = pWebVar->swRxRateLimitEnable;
	req_data->rtl8306eConfig.bandwidthConfig.rxPort[2].bandwidth_control_enable = pWebVar->swRxRateLimitEnable;
	req_data->rtl8306eConfig.bandwidthConfig.rxPort[3].bandwidth_control_enable = pWebVar->swRxRateLimitEnable;
	req_data->rtl8306eConfig.bandwidthConfig.rxPort[4].bandwidth_control_enable = pWebVar->swRxRateLimitEnable;
	req_data->rtl8306eConfig.bandwidthConfig.rxPort[0].bandwidth_value = pWebVar->swEth1RxRate;
	req_data->rtl8306eConfig.bandwidthConfig.rxPort[1].bandwidth_value = pWebVar->swEth2RxRate;
	req_data->rtl8306eConfig.bandwidthConfig.rxPort[2].bandwidth_value = pWebVar->swEth3RxRate;
	req_data->rtl8306eConfig.bandwidthConfig.rxPort[3].bandwidth_value = pWebVar->swEth4RxRate;
	req_data->rtl8306eConfig.bandwidthConfig.rxPort[4].bandwidth_value = pWebVar->swUplinkRxRate;

	req_data->rtl8306eConfig.bandwidthConfig.g_tx_bandwidth_control_enable = pWebVar->swTxRateLimitEnable;
	req_data->rtl8306eConfig.bandwidthConfig.txPort[0].bandwidth_control_enable = pWebVar->swTxRateLimitEnable;
	req_data->rtl8306eConfig.bandwidthConfig.txPort[1].bandwidth_control_enable = pWebVar->swTxRateLimitEnable;
	req_data->rtl8306eConfig.bandwidthConfig.txPort[2].bandwidth_control_enable = pWebVar->swTxRateLimitEnable;
	req_data->rtl8306eConfig.bandwidthConfig.txPort[3].bandwidth_control_enable = pWebVar->swTxRateLimitEnable;
	req_data->rtl8306eConfig.bandwidthConfig.txPort[4].bandwidth_control_enable = pWebVar->swTxRateLimitEnable;
	req_data->rtl8306eConfig.bandwidthConfig.txPort[0].bandwidth_value = pWebVar->swEth1TxRate;
	req_data->rtl8306eConfig.bandwidthConfig.txPort[1].bandwidth_value = pWebVar->swEth2TxRate;
	req_data->rtl8306eConfig.bandwidthConfig.txPort[2].bandwidth_value = pWebVar->swEth3TxRate;
	req_data->rtl8306eConfig.bandwidthConfig.txPort[3].bandwidth_value = pWebVar->swEth4TxRate;
	req_data->rtl8306eConfig.bandwidthConfig.txPort[4].bandwidth_value = pWebVar->swUplinkTxRate;

	//storm filter
	req_data->rtl8306eConfig.stormFilter.disable_broadcast = pWebVar->swSfDisBroadcast;
	req_data->rtl8306eConfig.stormFilter.disable_multicast = pWebVar->swSfDisMulticast;
	req_data->rtl8306eConfig.stormFilter.disable_unknown = pWebVar->swSfDisUnknown;
	req_data->rtl8306eConfig.stormFilter.iteration = pWebVar->swSfIteration;
	req_data->rtl8306eConfig.stormFilter.reset_source = pWebVar->swSfResetSrc;
	req_data->rtl8306eConfig.stormFilter.rule = pWebVar->swSfRule;
	req_data->rtl8306eConfig.stormFilter.thresholt = pWebVar->swSfThresholt;

	//mac limit
	req_data->rtl8306eConfig.macLimit.action = 0;			/* drop */
	req_data->rtl8306eConfig.macLimit.system.enable = pWebVar->swMlSysEnable;
	req_data->rtl8306eConfig.macLimit.system.thresholt = pWebVar->swMlSysThresholt;
	req_data->rtl8306eConfig.macLimit.system.mport = 0xf;	/* p0~p3 */
	req_data->rtl8306eConfig.macLimit.port[0].enable = pWebVar->swMlEth1Enable;
	req_data->rtl8306eConfig.macLimit.port[0].thresholt = pWebVar->swMlEth1Thresholt;
	req_data->rtl8306eConfig.macLimit.port[1].enable = pWebVar->swMlEth2Enable;
	req_data->rtl8306eConfig.macLimit.port[1].thresholt = pWebVar->swMlEth2Thresholt;
	req_data->rtl8306eConfig.macLimit.port[2].enable = pWebVar->swMlEth3Enable;
	req_data->rtl8306eConfig.macLimit.port[2].thresholt = pWebVar->swMlEth3Thresholt;
	req_data->rtl8306eConfig.macLimit.port[3].enable = pWebVar->swMlEth4Enable;
	req_data->rtl8306eConfig.macLimit.port[3].thresholt = pWebVar->swMlEth4Thresholt;

	//loop deection
	req_data->rtl8306eConfig.loopDetect.status = pWebVar->swLoopDetect;
	if( 0 == req_data->rtl8306eConfig.loopDetect.status )
	{
		req_data->rtl8306eConfig.loopDetect.ldmethod = 0;
		req_data->rtl8306eConfig.loopDetect.ldtime = 0;
		req_data->rtl8306eConfig.loopDetect.disfltlf = 0;
		req_data->rtl8306eConfig.loopDetect.enlpttl = 0;
	}
	else
	{
		req_data->rtl8306eConfig.loopDetect.ldmethod = 1;
		req_data->rtl8306eConfig.loopDetect.ldtime = 3;
		req_data->rtl8306eConfig.loopDetect.disfltlf = 1;
		req_data->rtl8306eConfig.loopDetect.enlpttl = 1;
	}
	//set sid here
	//printf("-sid: %s\n", pWebVar->swSwitchSid);
	if( CMM_SUCCESS == boardapi_macs2b(pWebVar->swSwitchSid, req_data->rtl8306eConfig.loopDetect.sid) )
	{
		len = sizeof(req->HEADER) + req->HEADER.ulBodyLength;
		return __http2cmm_comm(buf, len);
	}
	else
	{
		return CMM_FAILED;
	}
	
}

int http2cmm_eraseSwitchSettings(PWEB_NTWK_VAR pWebVar)
{	
	uint8_t buf[MAX_UDP_SIZE] = {0};
	uint32_t len = 0;
	uint8_t tmp;
	uint32_t cltindex = 0;
	uint32_t cnuindex = 0;
	st_dbsCnu cnu;
	
	T_Msg_CMM *req = (T_Msg_CMM *)buf;
	stTmUserInfo *req_data = (stTmUserInfo *)(req->BUF);

	dbsGetCnu(dbsdev, pWebVar->cnuid, &cnu);
	if( cnu.col_auth == 0 )
	{
		cnu.col_auth = 1;
		dbsUpdateCnu(dbsdev, pWebVar->cnuid, &cnu);
	}

	pWebVar->cnuPermition = 1;
	pWebVar->col_eth1sts = 1;
	pWebVar->col_eth2sts = 1;
	pWebVar->col_eth3sts = 1;
	pWebVar->col_eth4sts = 1;
	
	pWebVar->swVlanEnable = 0;
	pWebVar->swUplinkPortVMode = 0;
	pWebVar->swEth1PortVMode = 0;
	pWebVar->swEth2PortVMode = 0;
	pWebVar->swEth3PortVMode = 0;
	pWebVar->swEth4PortVMode = 0;

	pWebVar->swUplinkPortVid = 1;
	pWebVar->swEth1PortVid = 1;
	pWebVar->swEth2PortVid = 1;
	pWebVar->swEth3PortVid = 1;
	pWebVar->swEth4PortVid = 1;

	pWebVar->swRxRateLimitEnable = 0;
	pWebVar->swTxRateLimitEnable = 0;

	pWebVar->swUplinkRxRate = 0x7ff;
	pWebVar->swEth1RxRate = 0x7ff;
	pWebVar->swEth2RxRate = 0x7ff;
	pWebVar->swEth3RxRate = 0x7ff;
	pWebVar->swEth4RxRate = 0x7ff;
	pWebVar->swUplinkTxRate = 0x7ff;
	pWebVar->swEth1TxRate = 0x7ff;
	pWebVar->swEth2TxRate = 0x7ff;
	pWebVar->swEth3TxRate = 0x7ff;
	pWebVar->swEth4TxRate = 0x7ff;

	pWebVar->swLoopDetect = 0;
	pWebVar->swldmethod = 0;
	pWebVar->swldtime = 0;
	pWebVar->swldbckfrq = 0;
	pWebVar->swldsclr = 0;
	pWebVar->swpabuzzer = 0;
	pWebVar->swentaglf = 0;
	pWebVar->swlpttlinit = 0;
	pWebVar->swlpfpri = 0;
	pWebVar->swenlpfpri = 0;
	pWebVar->swdisfltlf = 0;
	pWebVar->swenlpttl = 0;
	pWebVar->swEth1LoopStatus = 0;
	pWebVar->swEth2LoopStatus = 0;
	pWebVar->swEth3LoopStatus = 0;
	pWebVar->swEth4LoopStatus = 0;
	strcpy(pWebVar->swSwitchSid, "52:54:4C:83:05:C0");

	pWebVar->swSfDisBroadcast = 1;
	pWebVar->swSfDisMulticast = 1;
	pWebVar->swSfDisUnknown = 1;
	pWebVar->swSfRule = 0;
	pWebVar->swSfResetSrc = 0;
	pWebVar->swSfIteration = 0;
	pWebVar->swSfThresholt = 0;

	//mac limit
	pWebVar->swMlSysEnable = 0;
	pWebVar->swMlSysThresholt = 0;
	pWebVar->swMlEth1Enable = 0;
	pWebVar->swMlEth1Thresholt = 0;
	pWebVar->swMlEth2Enable = 0;
	pWebVar->swMlEth2Thresholt = 0;
	pWebVar->swMlEth3Enable = 0;
	pWebVar->swMlEth3Thresholt = 0;
	pWebVar->swMlEth4Enable = 0;
	pWebVar->swMlEth4Thresholt = 0;

	cltindex = (pWebVar->cnuid - 1)/MAX_CNUS_PER_CLT + 1;
	cnuindex = (pWebVar->cnuid - 1)%MAX_CNUS_PER_CLT + 1;
	
	//T_REQ_Msg_CMM *ack = (T_REQ_Msg_CMM *)buf;

	req->HEADER.usSrcMID = MID_HTTP;
	req->HEADER.usDstMID = MID_CMM;
	req->HEADER.usMsgType = CMM_ERASE_MOD_A;
	req->HEADER.ulBodyLength = sizeof(stTmUserInfo);
	req->HEADER.fragment = 0;

	req_data->clt = cltindex;
	req_data->cnu = cnuindex;

	len = sizeof(req->HEADER) + req->HEADER.ulBodyLength;
	
	return __http2cmm_comm(buf, len);
	
}

int http2cmm_getSwitchSettings(stCnuNode *node, st_rtl8306eSettings * rtl8306e)
{
	uint8_t buf[MAX_UDP_SIZE] = {0};
	uint32_t len = 0;

	stTmUserInfo szNode;	
	
	T_Msg_CMM *req = (T_Msg_CMM *)buf;
	stTmUserInfo *req_data = (stTmUserInfo *)(req->BUF);
	
	T_REQ_Msg_CMM *ack = (T_REQ_Msg_CMM *)buf;
	st_rtl8306eSettings *ack_data = (st_rtl8306eSettings *)(ack->BUF);

	req->HEADER.usSrcMID = MID_HTTP;
	req->HEADER.usDstMID = MID_CMM;
	req->HEADER.usMsgType = CMM_CNU_SWITCH_CONFIG_READ;
	req->HEADER.ulBodyLength = sizeof(stTmUserInfo);
	req->HEADER.fragment = 0;

	req_data->clt = node->clt;
	req_data->cnu = node->cnu;

	len = sizeof(req->HEADER) + req->HEADER.ulBodyLength;

	if( CMM_SUCCESS == __http2cmm_comm(buf, len) )
	{
		memcpy(rtl8306e, ack_data, sizeof(st_rtl8306eSettings));
		return CMM_SUCCESS;
	}
	else
	{
		return CMM_FAILED;
	}
}

int http2cmm_setSwitchSettings(stCnuNode *node, st_rtl8306eSettings * rtl8306e)
{	
	uint8_t buf[MAX_UDP_SIZE] = {0};
	uint32_t len = 0;
	int i = 0;
	uint8_t tmp;
	
	T_Msg_CMM *req = (T_Msg_CMM *)buf;
	rtl8306eWriteInfo *req_data = (rtl8306eWriteInfo *)(req->BUF);
	
	//T_REQ_Msg_CMM *ack = (T_REQ_Msg_CMM *)buf;

	/* add by stan for save dbs  begin */
	st_dbsProfile profile;
	

	/* 如果该PROFILE 槽位无效则禁止配置*/
	
	if( CMM_SUCCESS != http2dbs_getProfile(node->cnu, &profile) )
	{
		return CMM_FAILED;
	}	
	else if( 0 == profile.col_row_sts )
	{
		return CMM_FAILED;
	}


	profile.col_vlanSts = (uint32_t) rtl8306e->vlanConfig.vlan_enable;
	if( profile.col_vlanSts == 0 )
	{
		profile.col_eth1vid = 1;
		profile.col_eth2vid = 1;
		profile.col_eth3vid = 1;
		profile.col_eth4vid = 1;
		rtl8306e->vlanConfig.vlan_port[0].pvid= 1;
		rtl8306e->vlanConfig.vlan_port[1].pvid= 1;						
		rtl8306e->vlanConfig.vlan_port[2].pvid= 1;
		rtl8306e->vlanConfig.vlan_port[3].pvid= 1;	
		//port egress mode	
		rtl8306e->vlanConfig.vlan_port[0].egress_mode = 3;
		rtl8306e->vlanConfig.vlan_port[1].egress_mode = 3;
		rtl8306e->vlanConfig.vlan_port[2].egress_mode = 3;
		rtl8306e->vlanConfig.vlan_port[3].egress_mode = 3;
		rtl8306e->vlanConfig.vlan_port[4].egress_mode = 3;
	}
	else
	{
		profile.col_eth1vid = rtl8306e->vlanConfig.vlan_port[0].pvid;
		profile.col_eth2vid = rtl8306e->vlanConfig.vlan_port[1].pvid;
		profile.col_eth3vid = rtl8306e->vlanConfig.vlan_port[2].pvid;
		profile.col_eth4vid = rtl8306e->vlanConfig.vlan_port[3].pvid;
		rtl8306e->vlanConfig.vlan_port[4].egress_mode = 2;
	}

	/* 防止端口PVID 被设置为0 */
	if( 0 == profile.col_eth1vid )
	{
		profile.col_eth1vid = 1;
		rtl8306e->vlanConfig.vlan_port[0].pvid = 1;
		rtl8306e->vlanConfig.vlan_port[0].egress_mode = 3;
	}
	if( 0 == profile.col_eth2vid )
	{
		profile.col_eth2vid = 1;
		rtl8306e->vlanConfig.vlan_port[1].pvid = 1;
		rtl8306e->vlanConfig.vlan_port[1].egress_mode = 3;
	}
	if( 0 == profile.col_eth3vid )
	{
		profile.col_eth3vid = 1;
		rtl8306e->vlanConfig.vlan_port[2].pvid = 1;
		rtl8306e->vlanConfig.vlan_port[2].egress_mode = 3;
	}
	if( 0 == profile.col_eth4vid )
	{
		profile.col_eth4vid = 1;
		rtl8306e->vlanConfig.vlan_port[3].pvid = 1;
		rtl8306e->vlanConfig.vlan_port[3].egress_mode = 3;
	}

	/* CHECK VID */
	if( (profile.col_eth1vid < 1) || (profile.col_eth1vid > 4094) )
	{
		return CMM_FAILED;
	}
	if( (profile.col_eth2vid < 1) || (profile.col_eth2vid > 4094) )
	{
		return CMM_FAILED;
	}
	if( (profile.col_eth3vid < 1) || (profile.col_eth3vid > 4094) )
	{
		return CMM_FAILED;
	}
	if( (profile.col_eth4vid < 1) || (profile.col_eth4vid > 4094) )
	{
		return CMM_FAILED;
	}
	
	/*  add by may2250 for port egress mode change */
	if( (profile.col_eth1vid > 1) && (profile.col_eth1vid < 4094) )
	{
		rtl8306e->vlanConfig.vlan_port[0].egress_mode = 1;
	}else{
		rtl8306e->vlanConfig.vlan_port[0].egress_mode = 3;
	}
	if( (profile.col_eth2vid > 1) && (profile.col_eth2vid < 4094) )
	{
		rtl8306e->vlanConfig.vlan_port[1].egress_mode = 1;
	}else{
		rtl8306e->vlanConfig.vlan_port[1].egress_mode = 3;
	}
	if( (profile.col_eth3vid > 1) && (profile.col_eth3vid < 4094) )
	{
		rtl8306e->vlanConfig.vlan_port[2].egress_mode = 1;
	}else{
		rtl8306e->vlanConfig.vlan_port[2].egress_mode = 3;
	}
	if( (profile.col_eth4vid > 1) && (profile.col_eth4vid < 4094) )
	{
		rtl8306e->vlanConfig.vlan_port[3].egress_mode = 1;
	}else{
		rtl8306e->vlanConfig.vlan_port[3].egress_mode = 3;
	}	

	/* add by stan for save dbs  end */

	req->HEADER.usSrcMID = MID_HTTP;
	req->HEADER.usDstMID = MID_CMM;
	req->HEADER.usMsgType = CMM_CNU_SWITCH_CONFIG_WRITE;
	req->HEADER.ulBodyLength = sizeof(rtl8306eWriteInfo) + sizeof(st_dbsProfile);
	req->HEADER.fragment = 0;

	/*add by stan */
	

	memcpy(req->BUF + sizeof(rtl8306eWriteInfo), &profile, sizeof(st_dbsProfile));

	/*add by stan end  */
	

	req_data->node.clt = node->clt;
	req_data->node.cnu = node->cnu;
	memcpy(&(req_data->rtl8306eConfig), rtl8306e, sizeof(st_rtl8306eSettings));

	len = sizeof(req->HEADER) + req->HEADER.ulBodyLength;
	if( len > MAX_UDP_SIZE )
	{
		printf("add by stan too len rtl8306 body + profile len\n");
		return CMM_FAILED;
	}
	
	return __http2cmm_comm(buf, len);	
}

int http2cmm_doLinkDiag( PWEB_NTWK_VAR pWebVar )
{
	uint8_t buf[MAX_UDP_SIZE] = {0};
	uint32_t len = 0;	
	
	T_Msg_CMM *req = (T_Msg_CMM *)buf;
	T_CMM_LINK_DIAG_INFO *req_data = (T_CMM_LINK_DIAG_INFO *)(req->BUF);

	T_REQ_Msg_CMM *ack = (T_REQ_Msg_CMM *)buf;
	T_MMEAD_LINK_DIAG_RESULT *ack_data = (T_MMEAD_LINK_DIAG_RESULT *)(ack->BUF);

	strcpy(pWebVar->diagCnuMac, "00:00:00:00:00:00");
	pWebVar->diagCnuModel = 0;
	pWebVar->diagCnuTei = 0;
	strcpy(pWebVar->ccoMac, "00:00:00:00:00:00");
	strcpy(pWebVar->ccoNid, "0");
	pWebVar->ccoSnid = 0;
	pWebVar->ccoTei = 0;
	pWebVar->diagCnuRxRate = 0;
	pWebVar->diagCnuTxRate = 0;
	sprintf(pWebVar->bitCarrier, "0.00");
	pWebVar->diagCnuAtten = 0;
	strcpy(pWebVar->bridgedMac, "00:00:00:00:00:00");
	sprintf(pWebVar->MPDU_ACKD, "0");
	sprintf(pWebVar->MPDU_COLL, "0");
	sprintf(pWebVar->MPDU_FAIL, "0");
	sprintf(pWebVar->PBS_PASS, "0");
	sprintf(pWebVar->PBS_FAIL, "0");

	req->HEADER.usSrcMID = MID_HTTP;
	req->HEADER.usDstMID = MID_CMM;
	req->HEADER.usMsgType = CMM_DO_LINK_DIAG;	
	req->HEADER.fragment = 0;
	req->HEADER.ulBodyLength = sizeof(T_CMM_LINK_DIAG_INFO);

	if( pWebVar->diagDir == 1 )
	{
		req_data->dir = 0;
	}
	else if( 2 == pWebVar->diagDir )
	{
		req_data->dir = 1;
	}
	else
	{
		pWebVar->diagResult = CMM_FAILED;
		return CMM_FAILED;
	}	
	req_data->clt = (pWebVar->cnuid - 1)/MAX_CNUS_PER_CLT + 1;
	req_data->cnu =  (pWebVar->cnuid - 1)%MAX_CNUS_PER_CLT + 1;

	len = sizeof(req->HEADER) + req->HEADER.ulBodyLength;
	
	if( len > MAX_UDP_SIZE )
	{
		pWebVar->diagResult = CMM_FAILED;
		return CMM_FAILED;
	}
	
	pWebVar->diagResult = __http2cmm_comm(buf, len);
	
	if( CMM_SUCCESS == pWebVar->diagResult )
	{
		sprintf(pWebVar->diagCnuMac, "%02X:%02X:%02X:%02X:%02X:%02X", 
			ack_data->mac[0], ack_data->mac[1], ack_data->mac[2], 
			ack_data->mac[3], ack_data->mac[4], ack_data->mac[5]
		);
		pWebVar->diagCnuModel = ack_data->model;
		pWebVar->diagCnuTei = ack_data->tei;
		sprintf(pWebVar->ccoMac, "%02X:%02X:%02X:%02X:%02X:%02X", 
			ack_data->ccoMac[0], ack_data->ccoMac[1], ack_data->ccoMac[2], 
			ack_data->ccoMac[3], ack_data->ccoMac[4], ack_data->ccoMac[5]
		);
		sprintf(pWebVar->ccoNid, "%02X%02X%02X%02X%02X%02X%02X", 
			ack_data->ccoNid[0], ack_data->ccoNid[1], ack_data->ccoNid[2], 
			ack_data->ccoNid[3], ack_data->ccoNid[4], ack_data->ccoNid[5], ack_data->ccoNid[6]
		);
		pWebVar->ccoSnid = ack_data->ccoSnid;
		pWebVar->ccoTei = ack_data->ccoTei;
		pWebVar->diagCnuRxRate = ack_data->rx;
		pWebVar->diagCnuTxRate = ack_data->tx;
		sprintf(pWebVar->bitCarrier, "%.2f", ack_data->bitRate);
		pWebVar->diagCnuAtten = ack_data->attenuation;
		sprintf(pWebVar->bridgedMac, "%02X:%02X:%02X:%02X:%02X:%02X", 
			ack_data->bridgedMac[0], ack_data->bridgedMac[1], ack_data->bridgedMac[2], 
			ack_data->bridgedMac[3], ack_data->bridgedMac[4], ack_data->bridgedMac[5]
		);
		sprintf(pWebVar->MPDU_ACKD, "%lld", ack_data->MPDU_ACKD);
		sprintf(pWebVar->MPDU_COLL, "%lld", ack_data->MPDU_COLL);
		sprintf(pWebVar->MPDU_FAIL, "%lld", ack_data->MPDU_FAIL);
		sprintf(pWebVar->PBS_PASS, "%lld", ack_data->PBS_PASS);
		sprintf(pWebVar->PBS_FAIL, "%lld", ack_data->PBS_FAIL);
	}
		
	return pWebVar->diagResult;
}

int http2cmm_doWListCtrlSettings( PWEB_NTWK_VAR pWebVar )
{
	uint8_t buf[MAX_UDP_SIZE] = {0};
	uint32_t len = 0;	
	T_Msg_CMM *req = (T_Msg_CMM *)buf;

	req->HEADER.usSrcMID = MID_HTTP;
	req->HEADER.usDstMID = MID_CMM;
	req->HEADER.usMsgType = pWebVar->wecWlistStatus?CMM_DO_WLIST_CONTROL:CMM_UNDO_WLIST_CONTROL;
	req->HEADER.ulBodyLength = 0;
	req->HEADER.fragment = 0;

	len = sizeof(req->HEADER) + req->HEADER.ulBodyLength;
	if( len > MAX_UDP_SIZE )
	{
		return CMM_FAILED;
	}
	return __http2cmm_comm(buf, len);
}

int http2cmm_doSpeedLimitSettings( PWEB_NTWK_VAR pWebVar )
{
	
	st_dbsProfile profile;
	uint8_t buf[MAX_UDP_SIZE] = {0};
	uint32_t len = 0;
	T_Msg_CMM *req = (T_Msg_CMM *)buf;
	

	/* 如果该PROFILE 槽位无效则禁止配置*/
	if( CMM_SUCCESS != dbsGetProfile(dbsdev, pWebVar->cnuid,  &profile) )
	{
		return CMM_FAILED;
	}	
	else if( 0 == profile.col_row_sts )
	{
		return CMM_FAILED;
	}

	profile.col_rxLimitSts = pWebVar->col_rxLimitSts;
	profile.col_txLimitSts = pWebVar->col_txLimitSts;
	
	if( 0 == profile.col_rxLimitSts )
	{
		profile.col_cpuPortRxRate = 0;
		profile.col_eth1rx = 0;
		profile.col_eth2rx = 0;
		profile.col_eth3rx = 0;
		profile.col_eth4rx = 0;
	}
	else
	{
		profile.col_cpuPortRxRate = __mapSelectIndex2Rate(pWebVar->col_cpuPortRxRate);
		profile.col_eth1rx = __mapSelectIndex2Rate(pWebVar->col_eth1rx);
		profile.col_eth2rx = __mapSelectIndex2Rate(pWebVar->col_eth2rx);
		profile.col_eth3rx = __mapSelectIndex2Rate(pWebVar->col_eth3rx);
		profile.col_eth4rx = __mapSelectIndex2Rate(pWebVar->col_eth4rx);
	}

	if( 0 == profile.col_txLimitSts )
	{
		profile.col_cpuPortTxRate = 0;
		profile.col_eth1tx = 0;
		profile.col_eth2tx = 0;
		profile.col_eth3tx = 0;
		profile.col_eth4tx = 0;
	}
	else
	{
		profile.col_cpuPortTxRate = __mapSelectIndex2Rate(pWebVar->col_cpuPortTxRate);
		profile.col_eth1tx = __mapSelectIndex2Rate(pWebVar->col_eth1tx);
		profile.col_eth2tx = __mapSelectIndex2Rate(pWebVar->col_eth2tx);
		profile.col_eth3tx = __mapSelectIndex2Rate(pWebVar->col_eth3tx);
		profile.col_eth4tx = __mapSelectIndex2Rate(pWebVar->col_eth4tx);
	}
	
	req->HEADER.usSrcMID = MID_HTTP;
	req->HEADER.usDstMID = MID_CMM;
	req->HEADER.usMsgType = CMM_CLI_FLOWCONTROL;
	req->HEADER.ulBodyLength = sizeof(st_dbsProfile);
	req->HEADER.fragment = 0;

	len = sizeof(req->HEADER) + req->HEADER.ulBodyLength;
	if( len > MAX_UDP_SIZE )
	{
		return CMM_FAILED;
	}

	memcpy(req->BUF, &profile, req->HEADER.ulBodyLength);
	
	return __http2cmm_comm(buf, len);
}

int http2cmm_doShutdownSettings( PWEB_NTWK_VAR pWebVar )
{
	st_dbsProfile profile;
	uint8_t buf[MAX_UDP_SIZE] = {0};
	uint32_t len = 0;
	T_Msg_CMM *req = (T_Msg_CMM *)buf;	

	/* 如果该PROFILE 槽位无效则禁止配置*/
	if( CMM_SUCCESS != dbsGetProfile(dbsdev, pWebVar->cnuid,  &profile) )
	{
		return CMM_FAILED;
	}	
	else if( 0 == profile.col_row_sts )
	{
		return CMM_FAILED;
	}
	
	profile.col_psctlSts = 1;

	profile.col_cpuPortSts = __getCnuPortStatus(profile.id, 0);
	profile.col_eth1sts = pWebVar->col_eth1sts;
	profile.col_eth2sts = pWebVar->col_eth2sts;
	profile.col_eth3sts = pWebVar->col_eth3sts;
	profile.col_eth4sts = pWebVar->col_eth4sts;	
	
	req->HEADER.usSrcMID = MID_HTTP;
	req->HEADER.usDstMID = MID_CMM;
	req->HEADER.usMsgType = CMM_CLI_SHUTDOWN;
	req->HEADER.ulBodyLength = sizeof(st_dbsProfile);
	req->HEADER.fragment = 0;

	len = sizeof(req->HEADER) + req->HEADER.ulBodyLength;
	if( len > MAX_UDP_SIZE )
	{
		return CMM_FAILED;
	}

	memcpy(req->BUF, &profile, req->HEADER.ulBodyLength);
	
	return __http2cmm_comm(buf, len);
}

int http2cmm_doCnuVlanSettings( PWEB_NTWK_VAR pWebVar )
{
	st_dbsProfile profile;
	uint8_t buf[MAX_UDP_SIZE] = {0};
	uint32_t len = 0;
	T_Msg_CMM *req = (T_Msg_CMM *)buf;
	

	/* 如果该PROFILE 槽位无效则禁止配置*/
	if( CMM_SUCCESS != dbsGetProfile(dbsdev, pWebVar->cnuid,  &profile) )
	{
		return CMM_FAILED;
	}	
	else if( 0 == profile.col_row_sts )
	{
		return CMM_FAILED;
	}

	profile.col_vlanSts = pWebVar->col_vlanSts;
	if( profile.col_vlanSts == 0 )
	{
		profile.col_eth1vid = 1;
		profile.col_eth2vid = 1;
		profile.col_eth3vid = 1;
		profile.col_eth4vid = 1;
		pWebVar->col_eth1vid = 1;
		pWebVar->col_eth2vid = 1;
		pWebVar->col_eth3vid = 1;
		pWebVar->col_eth4vid = 1;
	}
	else
	{
		profile.col_eth1vid = pWebVar->col_eth1vid;
		profile.col_eth2vid = pWebVar->col_eth2vid;
		profile.col_eth3vid = pWebVar->col_eth3vid;
		profile.col_eth4vid = pWebVar->col_eth4vid;
	}

	/* 防止端口PVID 被设置为0 */
	if( 0 == profile.col_eth1vid )
	{
		profile.col_eth1vid = 1;
		pWebVar->col_eth1vid = 1;
	}
	if( 0 == profile.col_eth2vid )
	{
		profile.col_eth2vid = 1;
		pWebVar->col_eth2vid = 1;
	}
	if( 0 == profile.col_eth3vid )
	{
		profile.col_eth3vid = 1;
		pWebVar->col_eth3vid = 1;
	}
	if( 0 == profile.col_eth4vid )
	{
		profile.col_eth4vid = 1;
		pWebVar->col_eth4vid = 1;
	}

	/* CHECK VID */
	if( (profile.col_eth1vid < 1) || (profile.col_eth1vid > 4094) )
	{
		return CMM_FAILED;
	}
	if( (profile.col_eth2vid < 1) || (profile.col_eth2vid > 4094) )
	{
		return CMM_FAILED;
	}
	if( (profile.col_eth3vid < 1) || (profile.col_eth3vid > 4094) )
	{
		return CMM_FAILED;
	}
	if( (profile.col_eth4vid < 1) || (profile.col_eth4vid > 4094) )
	{
		return CMM_FAILED;
	}	
	
	req->HEADER.usSrcMID = MID_HTTP;
	req->HEADER.usDstMID = MID_CMM;
	req->HEADER.usMsgType = CMM_CNU_VLAN_CONFIG;
	req->HEADER.ulBodyLength = sizeof(st_dbsProfile);
	req->HEADER.fragment = 0;

	len = sizeof(req->HEADER) + req->HEADER.ulBodyLength;
	if( len > MAX_UDP_SIZE )
	{
		return CMM_FAILED;
	}

	memcpy(req->BUF, &profile, req->HEADER.ulBodyLength);
	
	return __http2cmm_comm(buf, len);
}

int http2cmm_doSFilterSettings( PWEB_NTWK_VAR pWebVar )
{
	st_dbsProfile profile;
	uint8_t buf[MAX_UDP_SIZE] = {0};
	uint32_t len = 0;
	T_Msg_CMM *req = (T_Msg_CMM *)buf;
	

	/* 如果该PROFILE 槽位无效则禁止配置*/
	if( CMM_SUCCESS != dbsGetProfile(dbsdev, pWebVar->cnuid,  &profile) )
	{
		return CMM_FAILED;
	}	
	else if( 0 == profile.col_row_sts )
	{
		return CMM_FAILED;
	}
	
	profile.col_sfbSts = pWebVar->col_sfbSts;
	profile.col_sfuSts = pWebVar->col_sfuSts;
	profile.col_sfmSts = pWebVar->col_sfmSts;
	if(profile.col_sfbSts|profile.col_sfuSts|profile.col_sfmSts)
	{
		profile.col_sfRate = 1;
	}
	else
	{
		profile.col_sfRate = 0;
	}
	
	req->HEADER.usSrcMID = MID_HTTP;
	req->HEADER.usDstMID = MID_CMM;
	req->HEADER.usMsgType = CMM_CLI_STROMCONTROL;
	req->HEADER.ulBodyLength = sizeof(st_dbsProfile);
	req->HEADER.fragment = 0;

	len = sizeof(req->HEADER) + req->HEADER.ulBodyLength;
	if( len > MAX_UDP_SIZE )
	{
		return CMM_FAILED;
	}

	memcpy(req->BUF, &profile, req->HEADER.ulBodyLength);
	
	return __http2cmm_comm(buf, len);
}

int http2cmm_doAgTimeSettings( PWEB_NTWK_VAR pWebVar )
{
	st_dbsProfile profile;
	uint8_t buf[MAX_UDP_SIZE] = {0};
	uint32_t len = 0;
	T_Msg_CMM *req = (T_Msg_CMM *)buf;
	

	/* 如果该PROFILE 槽位无效则禁止配置*/
	if( CMM_SUCCESS != dbsGetProfile(dbsdev, pWebVar->cnuid,  &profile) )
	{
		return CMM_FAILED;
	}	
	else if( 0 == profile.col_row_sts )
	{
		return CMM_FAILED;
	}
	
	profile.col_loagTime = pWebVar->col_loagTime;
	profile.col_reagTime = pWebVar->col_reagTime;
	
	req->HEADER.usSrcMID = MID_HTTP;
	req->HEADER.usDstMID = MID_CMM;
	req->HEADER.usMsgType = CMM_CLI_DO_AGING_TIME_CONFIG;
	req->HEADER.ulBodyLength = sizeof(st_dbsProfile);
	req->HEADER.fragment = 0;

	len = sizeof(req->HEADER) + req->HEADER.ulBodyLength;
	if( len > MAX_UDP_SIZE )
	{
		return CMM_FAILED;
	}

	memcpy(req->BUF, &profile, req->HEADER.ulBodyLength);
	
	return __http2cmm_comm(buf, len);
}

int http2cmm_doMacLimiting( PWEB_NTWK_VAR pWebVar )
{
	st_dbsProfile profile;
	uint8_t buf[MAX_UDP_SIZE] = {0};
	uint32_t len = 0;
	T_Msg_CMM *req = (T_Msg_CMM *)buf;
	

	/* 如果该PROFILE 槽位无效则禁止配置*/
	if( CMM_SUCCESS != dbsGetProfile(dbsdev, pWebVar->cnuid,  &profile) )
	{
		return CMM_FAILED;
	}	
	else if( 0 == profile.col_row_sts )
	{
		return CMM_FAILED;
	}
	
	profile.col_macLimit = pWebVar->col_macLimit;
	
	req->HEADER.usSrcMID = MID_HTTP;
	req->HEADER.usDstMID = MID_CMM;
	req->HEADER.usMsgType = CMM_DO_MAC_LIMIT_CONFIG;
	req->HEADER.ulBodyLength = sizeof(st_dbsProfile);
	req->HEADER.fragment = 0;

	len = sizeof(req->HEADER) + req->HEADER.ulBodyLength;
	if( len > MAX_UDP_SIZE )
	{
		return CMM_FAILED;
	}

	memcpy(req->BUF, &profile, req->HEADER.ulBodyLength);
	
	return __http2cmm_comm(buf, len);
}

int http2cmm_createCnu( PWEB_NTWK_VAR pWebVar )
{
	uint8_t bMac[6] = {0};	
	uint8_t buf[MAX_UDP_SIZE] = {0};
	uint32_t len = 0;	
	T_Msg_CMM *req = (T_Msg_CMM *)buf;

	req->HEADER.usSrcMID = MID_HTTP;
	req->HEADER.usDstMID = MID_CMM;
	req->HEADER.usMsgType = CMM_CREATE_CNU;
	req->HEADER.ulBodyLength = 6;
	req->HEADER.fragment = 0;

	len = sizeof(req->HEADER) + req->HEADER.ulBodyLength;
	if( len > MAX_UDP_SIZE )
	{
		return CMM_FAILED;
	}
	
	if( 0 != boardapi_macs2b(pWebVar->newCnuMac, bMac) )
	{
		return CMM_FAILED;
	}

	memcpy(req->BUF, bMac, req->HEADER.ulBodyLength);

	return __http2cmm_comm(buf, len);
}

int http2cmm_sysReboot(void)
{
	uint8_t buf[MAX_UDP_SIZE] = {0};
	uint32_t len = 0;
	T_Msg_CMM *req = (T_Msg_CMM *)buf;

	req->HEADER.usSrcMID = MID_HTTP;
	req->HEADER.usDstMID = MID_CMM;
	req->HEADER.usMsgType = CMM_CBAT_RESET;
	req->HEADER.ulBodyLength = 0;
	req->HEADER.fragment = 0;

	len = sizeof(req->HEADER) + req->HEADER.ulBodyLength;
	return __http2cmm_comm(buf, len);
}

int http2cmm_restoreDefault(void)
{
	uint8_t buf[MAX_UDP_SIZE] = {0};
	uint32_t len = 0;
	T_Msg_CMM *req = (T_Msg_CMM *)buf;

	req->HEADER.usSrcMID = MID_HTTP;
	req->HEADER.usDstMID = MID_CMM;
	req->HEADER.usMsgType = CMM_RESTORE_DEFAULT;
	req->HEADER.ulBodyLength = 0;
	req->HEADER.fragment = 0;

	len = sizeof(req->HEADER) + req->HEADER.ulBodyLength;
	return __http2cmm_comm(buf, len);
}

int http2cmm_clearPortStats(void)
{
	uint8_t buf[MAX_UDP_SIZE] = {0};
	uint32_t len = 0;
	T_Msg_CMM *req = (T_Msg_CMM *)buf;

	req->HEADER.usSrcMID = MID_HTTP;
	req->HEADER.usDstMID = MID_CMM;
	req->HEADER.usMsgType = CMM_CLEAR_PORT_STAT;
	req->HEADER.ulBodyLength = 0;
	req->HEADER.fragment = 0;

	len = sizeof(req->HEADER) + req->HEADER.ulBodyLength;
	return __http2cmm_comm(buf, len);
}

int http2cmm_getPortStats(int port, T_CMM_PORT_STATS_INFO *stats)
{
	uint8_t buf[MAX_UDP_SIZE] = {0};
	uint32_t len = 0;

	assert( NULL != stats );
	
	T_Msg_CMM *req = (T_Msg_CMM *)buf;
	uint32_t *req_data = (uint32_t *)(req->BUF);
	
	T_REQ_Msg_CMM *ack = (T_REQ_Msg_CMM *)buf;
	T_CMM_PORT_STATS_INFO *ack_data = (T_CMM_PORT_STATS_INFO *)(ack->BUF);

	req->HEADER.usSrcMID = MID_HTTP;
	req->HEADER.usDstMID = MID_CMM;
	req->HEADER.usMsgType = CMM_GET_PORT_STAT;
	req->HEADER.ulBodyLength = sizeof(uint32_t);
	req->HEADER.fragment = 0;

	*req_data = port;

	len = sizeof(req->HEADER) + req->HEADER.ulBodyLength;

	if( CMM_SUCCESS == __http2cmm_comm(buf, len) )
	{
		memcpy(stats, ack_data, sizeof(T_CMM_PORT_STATS_INFO));
	}
	else
	{
		bzero(stats, sizeof(T_CMM_PORT_STATS_INFO));
	}
	return CMM_SUCCESS;
}

int http2cmm_getPortStatsAll(T_CMM_PORT_STATS_INFO *stats)
{
	uint8_t buf[MAX_UDP_SIZE] = {0};
	uint32_t len = 0;
	
	assert( NULL != stats );
	
	T_Msg_CMM *req = (T_Msg_CMM *)buf;
	
	T_REQ_Msg_CMM *ack = (T_REQ_Msg_CMM *)buf;
	T_CMM_PORT_STATS_INFO *ack_data = (T_CMM_PORT_STATS_INFO *)(ack->BUF);

	req->HEADER.usSrcMID = MID_HTTP;
	req->HEADER.usDstMID = MID_CMM;
	req->HEADER.usMsgType = CMM_GET_88E6171R_PORT_STATS_ALL;
	req->HEADER.ulBodyLength =0;
	req->HEADER.fragment = 0;

	len = sizeof(req->HEADER) + req->HEADER.ulBodyLength;

	if( CMM_SUCCESS == __http2cmm_comm(buf, len) )
	{
		len = ack->HEADER.ulBodyLength - sizeof(uint16_t);
		memcpy(stats, ack_data, len);	
	}	
	return CMM_SUCCESS;
}

int http2cmm_getPortPropety(int port, T_CMM_PORT_PROPETY_INFO *propety)
{
	uint8_t buf[MAX_UDP_SIZE] = {0};
	uint32_t len = 0;

	assert( NULL != propety );
	
	T_Msg_CMM *req = (T_Msg_CMM *)buf;
	uint32_t *req_data = (uint32_t *)(req->BUF);
	
	T_REQ_Msg_CMM *ack = (T_REQ_Msg_CMM *)buf;
	T_CMM_PORT_PROPETY_INFO *ack_data = (T_CMM_PORT_PROPETY_INFO *)(ack->BUF);

	req->HEADER.usSrcMID = MID_HTTP;
	req->HEADER.usDstMID = MID_CMM;
	req->HEADER.usMsgType = CMM_GET_IP175D_PORT_PROPETY;
	req->HEADER.ulBodyLength = sizeof(uint32_t);
	req->HEADER.fragment = 0;

	*req_data = port;

	len = sizeof(req->HEADER) + req->HEADER.ulBodyLength;

	if( CMM_SUCCESS == __http2cmm_comm(buf, len) )
	{
		memcpy(propety, ack_data, sizeof(T_CMM_PORT_PROPETY_INFO));
	}
	else
	{
		bzero(propety, sizeof(T_CMM_PORT_PROPETY_INFO));
	}
	return CMM_SUCCESS;
}

int http2cmm_getPortPropetyAll(T_CMM_PORT_PROPETY_INFO *propety)
{
	uint8_t buf[MAX_UDP_SIZE] = {0};
	uint32_t len = 0;
	
	assert( NULL != propety );
	
	T_Msg_CMM *req = (T_Msg_CMM *)buf;
	
	T_REQ_Msg_CMM *ack = (T_REQ_Msg_CMM *)buf;
	T_CMM_PORT_PROPETY_INFO *ack_data = (T_CMM_PORT_PROPETY_INFO *)(ack->BUF);

	req->HEADER.usSrcMID = MID_HTTP;
	req->HEADER.usDstMID = MID_CMM;
	req->HEADER.usMsgType = CMM_GET_88E6171R_PORT_PROPETY_ALL;
	req->HEADER.ulBodyLength =0;
	req->HEADER.fragment = 0;

	len = sizeof(req->HEADER) + req->HEADER.ulBodyLength;

	if( CMM_SUCCESS == __http2cmm_comm(buf, len) )
	{
		len = ack->HEADER.ulBodyLength - sizeof(uint16_t);
		memcpy(propety, ack_data, len);	
	}	
	return CMM_SUCCESS;
}

int http2cmm_getCbatTemperature(st_temperature *temp_data)
{
	uint8_t buf[MAX_UDP_SIZE] = {0};
	uint32_t len = 0;
	
	T_Msg_CMM *req = (T_Msg_CMM *)buf;
	uint32_t *req_data = (uint32_t *)(req->BUF);
	
	T_REQ_Msg_CMM *ack = (T_REQ_Msg_CMM *)buf;
	T_CMM_PORT_PROPETY_INFO *ack_data = (T_CMM_PORT_PROPETY_INFO *)(ack->BUF);

	req->HEADER.usSrcMID = MID_HTTP;
	req->HEADER.usDstMID = MID_CMM;
	req->HEADER.usMsgType = CMM_GET_CBAT_TEMPERATURE;
	req->HEADER.ulBodyLength = 0;
	req->HEADER.fragment = 0;

	len = sizeof(req->HEADER) + req->HEADER.ulBodyLength;
	if( len > MAX_UDP_SIZE )
	{
		IO_Print("\r\n\r\n	Memery Error !");
		return CMM_FAILED;
	}

	if( CMM_SUCCESS == __http2cmm_comm(buf, len) )
	{
		memcpy(temp_data, ack_data, sizeof(st_temperature));
		return CMM_SUCCESS;
	}	
	return CMM_FAILED;
}

int http2cmm_getCnuUserHFID(T_szCnuUserHFID *cnuuserhfid )
{
	uint8_t buf[MAX_UDP_SIZE] = {0};
	uint32_t len = 0;
	
	T_Msg_CMM *req = (T_Msg_CMM *)buf;
//	uint32_t *req_data = (uint32_t *)(req->BUF);
	
	T_REQ_Msg_CMM *ack = (T_REQ_Msg_CMM *)buf;
	T_szCnuUserHFID *ack_data = (T_szCnuUserHFID *)(ack->BUF);

	req->HEADER.usSrcMID = MID_HTTP;
	req->HEADER.usDstMID = MID_CMM;
	req->HEADER.usMsgType = CMM_USER_HFID_READ;
	req->HEADER.ulBodyLength = sizeof(T_szCnuUserHFID);
	req->HEADER.fragment = 0;

	len = sizeof(req->HEADER) + req->HEADER.ulBodyLength;
	if( len > MAX_UDP_SIZE )
	{
		IO_Print("\r\n\r\n	Memery Error !");
		return CMM_FAILED;
	}
	memcpy(req->BUF,cnuuserhfid, req->HEADER.ulBodyLength);

	if( CMM_SUCCESS == __http2cmm_comm(buf, len) )
	{
		memcpy(cnuuserhfid->pdata,ack_data->pdata,64);
		return CMM_SUCCESS;
	}	
	return CMM_FAILED;
}

int http2cmm_upgrade(void)
{
	return CMM_FAILED;
}

int http2cmm_destroy(void)
{
	T_UDP_SK_INFO *sk = &SK_HTTP2CMM;
	if( sk->sk != 0)
	{
		close(sk->sk);
		sk->sk = 0;
	}
	return CMM_SUCCESS;
}

int http2cmm_init(void)
{
	if( ( SK_HTTP2CMM.sk = socket(PF_INET, SOCK_DGRAM, 0) ) < 0 )
	{
		return CMM_FAILED;
	}	

	SK_HTTP2CMM.skaddr.sin_family = PF_INET;
	SK_HTTP2CMM.skaddr.sin_port = htons(CMM_LISTEN_PORT);		/* 目的端口号*/
	SK_HTTP2CMM.skaddr.sin_addr.s_addr = inet_addr("127.0.0.1");

	return CMM_SUCCESS;
} 

