#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
//#include <sys/time.h>
#include <time.h>
#include "json.h"
//#include "parse_flags.h"
#include "cgimain.h"
#include "jsonmain.h"
#include "http2dbs.h"
#include "http2cmm.h"
#include <public.h>
#include <boardapi.h>

#define JSON_STRING_SIZE	1024
#define JSON_DEBUG_ENABLE	0

typedef struct
{
	int devType;				/* devType defined by NMS */
	char macAddr[32];		/* mac address string */
	int cnuPermit;			/* snmp true(1) or false(2) value */
	int cnuVlanSts;			/* snmp true(1) or false(2) value */
	int cnuEth1Vid;			/* 1~4094 */
	int cnuEth2Vid;			/* 1~4094 */
	int cnuEth3Vid;			/* 1~4094 */
	int cnuEth4Vid;			/* 1~4094 */
	int cnuTxRateLimitSts;	/* snmp true(1) or false(2) value */
	int cnuCpuPortTxRate;	/* times of 32Kb(0Kb~100*1024Kb). No rate limiting if rate is 0, 1 means 32Kb, and so on */
	int cnuEth1TxRate;		/* times of 32Kb(0Kb~100*1024Kb). No rate limiting if rate is 0 */
	int cnuEth2TxRate;		/* times of 32Kb(0Kb~100*1024Kb). No rate limiting if rate is 0 */
	int cnuEth3TxRate;		/* times of 32Kb(0Kb~100*1024Kb). No rate limiting if rate is 0 */
	int cnuEth4TxRate;		/* times of 32Kb(0Kb~100*1024Kb). No rate limiting if rate is 0 */
	int cnuRxRateLimitSts;	/* snmp true(1) or false(2) value */
	int cnuCpuPortRxRate;	/* times of 32Kb(0Kb~100*1024Kb). No rate limiting if rate is 0 */
	int cnuEth1RxRate;		/* times of 32Kb(0Kb~100*1024Kb). No rate limiting if rate is 0 */
	int cnuEth2RxRate;		/* times of 32Kb(0Kb~100*1024Kb). No rate limiting if rate is 0 */
	int cnuEth3RxRate;		/* times of 32Kb(0Kb~100*1024Kb). No rate limiting if rate is 0 */
	int cnuEth4RxRate;		/* times of 32Kb(0Kb~100*1024Kb). No rate limiting if rate is 0 */
	
}
JSON_VAR, *PJSON_VAR;

JSON_VAR glbJsonVar;

CGI_ITEM jsonSetTable[] = 
{
	//{ "type",			(void *)(&(glbJsonVar.devType)),			CGI_TYPE_NUM },
	{ "mac",			(void *)(  (glbJsonVar.macAddr)),			CGI_TYPE_STR },
	{ "permit",		(void *)(&(glbJsonVar.cnuPermit)),			CGI_TYPE_NUM },
	{ "vlanen",		(void *)(&(glbJsonVar.cnuVlanSts)),			CGI_TYPE_NUM },
	{ "vlan0id",		(void *)(&(glbJsonVar.cnuEth1Vid)),			CGI_TYPE_NUM },
	{ "vlan1id",		(void *)(&(glbJsonVar.cnuEth2Vid)),			CGI_TYPE_NUM },
	{ "vlan2id",		(void *)(&(glbJsonVar.cnuEth3Vid)),			CGI_TYPE_NUM },
	{ "vlan3id",		(void *)(&(glbJsonVar.cnuEth4Vid)),			CGI_TYPE_NUM },
	{ "txlimitsts",		(void *)(&(glbJsonVar.cnuTxRateLimitSts)),	CGI_TYPE_NUM },
	{ "cpuporttxrate",	(void *)(&(glbJsonVar.cnuCpuPortTxRate)),	CGI_TYPE_NUM },
	{ "port0txrate",	(void *)(&(glbJsonVar.cnuEth1TxRate)),		CGI_TYPE_NUM },
	{ "port1txrate",	(void *)(&(glbJsonVar.cnuEth2TxRate)),		CGI_TYPE_NUM },
	{ "port2txrate",	(void *)(&(glbJsonVar.cnuEth3TxRate)),		CGI_TYPE_NUM },
	{ "port3txrate",	(void *)(&(glbJsonVar.cnuEth4TxRate)),		CGI_TYPE_NUM },

	{ "rxlimitsts",		(void *)(&(glbJsonVar.cnuRxRateLimitSts)),	CGI_TYPE_NUM },
	{ "cpuportrxrate",	(void *)(&(glbJsonVar.cnuCpuPortRxRate)),	CGI_TYPE_NUM },
	{ "port0rxrate",	(void *)(&(glbJsonVar.cnuEth1RxRate)),		CGI_TYPE_NUM },
	{ "port1rxrate",	(void *)(&(glbJsonVar.cnuEth2RxRate)),		CGI_TYPE_NUM },
	{ "port2rxrate",	(void *)(&(glbJsonVar.cnuEth3RxRate)),		CGI_TYPE_NUM },
	{ "port3rxrate",	(void *)(&(glbJsonVar.cnuEth4RxRate)),		CGI_TYPE_NUM },

	{ NULL, NULL, CGI_TYPE_NONE }
};

void json_debug(char *jsonString, size_t jstrLen)
{
	if(JSON_DEBUG_ENABLE)
	{
		printf("jstrLen	= %d\n", jstrLen);
		printf("jsonString	= %s\n", jsonString);
	}
}

void jsonSetVar(char *varName, char *varValue)
{
	int i = 0;

	for ( ; jsonSetTable[i].variable != NULL; i++ )
	{
		if ( strcmp(varName, jsonSetTable[i].variable) == 0 )
		{
			break;
		}
	}			

	if ( jsonSetTable[i].variable != NULL )
	{
		switch ( jsonSetTable[i].type )
		{
			case CGI_TYPE_STR:
			{
				strcpy((char *)jsonSetTable[i].value, varValue);
				break;
			}
			case CGI_TYPE_NUM:
			{
				*((int *)jsonSetTable[i].value) = atoi(varValue);
				break;
			}
			default:
			{
				break;
			}
		}
	}
}

void jsonParseSet(char *jsonString)
{
	char name[32] = {0};
	char value[64] = {0};
	json_object *my_object;
	my_object = json_tokener_parse(jsonString);
	
	json_object_object_foreach(my_object, key, val)
	{
		strcpy(name, key);
		strcpy(value, json_object_get_string(val));
		//printf("\t%s: %s\n", name, value);
		cgiUrlDecode(value);
		jsonSetVar(name, value);		
	}	
	//printf("my_object.to_string()=%s\n", json_object_to_json_string(my_object));
	json_object_put(my_object);
}

static void jsonSendAck( FILE * fs, int status, const char* jsonString )
{
	time_t now;
	char timebuf[100];

	if( status == 0 )
	{
		(void) fprintf( fs, "%s %d %s\r\n", "HTTP/1.1", 200, "Ok" );
	}
	else
	{
		(void) fprintf( fs, "%s %d %s\r\n", "HTTP/1.1", 400, "Bad Request" );
	}
	
	(void) fprintf( fs, "Server: %s\r\n", "micro_httpd" );
	(void) fprintf( fs,"Cache-Control: no-cache\r\n") ;
	now = time( (time_t*) 0 );
	(void) strftime( timebuf, sizeof(timebuf), "%a, %d %b %Y %H:%M:%S GMT", gmtime( &now ) );
	(void) fprintf( fs, "Date: %s\r\n", timebuf );
	(void) fprintf( fs, "Content-Type: %s\r\n", "text/json" );
	(void) fprintf( fs, "Connection: close\r\n" );
	(void) fprintf( fs, "\r\n" );
	if( NULL != jsonString )
	{
		(void) fprintf( fs, jsonString );
	}
}

/* check input parameters , return 0: success; else failed */
int jsonSetCnuCheckInput(void)
{
	/* check port vlan id if vlan enable */
	if( 1 == glbJsonVar.cnuVlanSts )
	{
		if( (glbJsonVar.cnuEth1Vid < 1) || (glbJsonVar.cnuEth1Vid > 4094 ))
			return CMM_FAILED;
		if( (glbJsonVar.cnuEth2Vid < 1) || (glbJsonVar.cnuEth2Vid > 4094 ))
			return CMM_FAILED;
		if( (glbJsonVar.cnuEth3Vid < 1) || (glbJsonVar.cnuEth3Vid > 4094 ))
			return CMM_FAILED;
		if( (glbJsonVar.cnuEth4Vid < 1) || (glbJsonVar.cnuEth4Vid > 4094 ))
			return CMM_FAILED;
	}

	/* check tx rate limiting parameters if cnuTxRateLimitSts enable */
	if( 1 == glbJsonVar.cnuTxRateLimitSts )
	{
		if( (glbJsonVar.cnuCpuPortTxRate < 0) || (glbJsonVar.cnuCpuPortTxRate > 3200 ))
			return CMM_FAILED;
		if( (glbJsonVar.cnuEth1TxRate < 0) || (glbJsonVar.cnuEth1TxRate > 3200 ))
			return CMM_FAILED;
		if( (glbJsonVar.cnuEth2TxRate < 0) || (glbJsonVar.cnuEth2TxRate > 3200 ))
			return CMM_FAILED;
		if( (glbJsonVar.cnuEth3TxRate < 0) || (glbJsonVar.cnuEth3TxRate > 3200 ))
			return CMM_FAILED;
		if( (glbJsonVar.cnuEth4TxRate < 0) || (glbJsonVar.cnuEth4TxRate > 3200 ))
			return CMM_FAILED;
	}

	/* check rx rate limiting parameters if cnuRxRateLimitSts enable */
	if( 1 == glbJsonVar.cnuRxRateLimitSts )
	{
		if( (glbJsonVar.cnuCpuPortRxRate < 0) || (glbJsonVar.cnuCpuPortRxRate > 3200 ))
			return CMM_FAILED;
		if( (glbJsonVar.cnuEth1RxRate < 0) || (glbJsonVar.cnuEth1RxRate > 3200 ))
			return CMM_FAILED;
		if( (glbJsonVar.cnuEth2RxRate < 0) || (glbJsonVar.cnuEth2RxRate > 3200 ))
			return CMM_FAILED;
		if( (glbJsonVar.cnuEth3RxRate < 0) || (glbJsonVar.cnuEth3RxRate > 3200 ))
			return CMM_FAILED;
		if( (glbJsonVar.cnuEth4RxRate < 0) || (glbJsonVar.cnuEth4RxRate > 3200 ))
			return CMM_FAILED;
	}
	
	return CMM_SUCCESS;
}

/* prepare setting parameters to profile, return 0: success; else failed */
int jsonSetCnuPrepare(st_dbsProfile * profile)
{
	/* over write vlan settings */
	profile->col_vlanSts = (1==glbJsonVar.cnuVlanSts)?1:0;
	if(profile->col_vlanSts)
	{
		profile->col_eth1vid = glbJsonVar.cnuEth1Vid;
		profile->col_eth2vid = glbJsonVar.cnuEth2Vid;
		profile->col_eth3vid = glbJsonVar.cnuEth3Vid;
		profile->col_eth4vid = glbJsonVar.cnuEth4Vid;
	}
	else
	{
		profile->col_eth1vid = 1;
		profile->col_eth2vid = 1;
		profile->col_eth3vid = 1;
		profile->col_eth4vid = 1;
	}

	/* over write rate limit settings*/
	profile->col_rxLimitSts = (1==glbJsonVar.cnuRxRateLimitSts)?1:0;
	if(profile->col_rxLimitSts)
	{
		profile->col_cpuPortRxRate = glbJsonVar.cnuCpuPortRxRate*32;
		profile->col_eth1rx = glbJsonVar.cnuEth1RxRate*32;
		profile->col_eth2rx = glbJsonVar.cnuEth2RxRate*32;
		profile->col_eth3rx = glbJsonVar.cnuEth3RxRate*32;
		profile->col_eth4rx = glbJsonVar.cnuEth4RxRate*32;
	}
	else
	{
		profile->col_cpuPortRxRate = 0;
		profile->col_eth1rx = 0;
		profile->col_eth2rx = 0;
		profile->col_eth3rx = 0;
		profile->col_eth4rx = 0;
	}
	profile->col_txLimitSts = (1==glbJsonVar.cnuTxRateLimitSts)?1:0;
	if(profile->col_txLimitSts)
	{
		profile->col_cpuPortTxRate = glbJsonVar.cnuCpuPortTxRate*32;
		profile->col_eth1tx = glbJsonVar.cnuEth1TxRate*32;
		profile->col_eth2tx = glbJsonVar.cnuEth2TxRate*32;
		profile->col_eth3tx = glbJsonVar.cnuEth3TxRate*32;
		profile->col_eth4tx = glbJsonVar.cnuEth4TxRate*32;
	}
	else
	{
		profile->col_cpuPortTxRate = 0;
		profile->col_eth1tx = 0;
		profile->col_eth2tx = 0;
		profile->col_eth3tx = 0;
		profile->col_eth4tx = 0;
	}
	
	return CMM_SUCCESS;
}

int jsonSetCnuProfile(FILE * fs)
{
	int ret = CMM_SUCCESS;
	char strlog[128] = {0};
	stCnuNode iNode;
	st_dbsCnu cnu;
	st_dbsProfile myProfile;
	st_rtl8306eSettings rtl8306e;
	json_object *my_object;
	int clt_index = 0;
	int cnu_index = 0;
	int i = 0;

	/* for debug */
	//printf("\n-->call jsonSetCnuProfile()\n");
	
	/* process json set request here */
	ret = boardapi_mac2Uppercase(glbJsonVar.macAddr);
	if( CMM_SUCCESS != ret )
	{
		printf("ERROR: jsonConvertMac2Uppercase\n");
		goto json_ack;
	}
	
	/* 1. get cnu index by input mac address */
	ret = http2dbs_getCnuIndexByMacaddress(glbJsonVar.macAddr, &iNode);
	if( CMM_SUCCESS != ret )
	{	
		/* system error */
		printf("ERROR: selectCnuIndex(%s)\n", glbJsonVar.macAddr);
		goto json_ack;
	}
	else if( 0 == iNode.cnu )
	{
		/* can not select this cnu */
		printf("ERROR: selectCnuIndex\n");
		ret = CMM_FAILED;
		goto json_ack;
	}
	
	/* 2. check input */
	ret = jsonSetCnuCheckInput();
	if( CMM_SUCCESS != ret )
	{
		printf("ERROR: jsonSetCnuCheckInput\n");
		goto json_ack;
	}

	ret = http2dbs_getCnu(iNode.cnu, &cnu);
	if( CMM_SUCCESS != ret )
	{
		printf("ERROR: http2dbs_getCnu\n");
		goto json_ack;
	}	

	if( CNU_SWITCH_TYPE_AR8236 == boardapi_getCnuSwitchType(cnu.col_model ))
	{
		/* 3. get profile by id */
		ret = http2dbs_getProfile(iNode.cnu, &myProfile);
		if( CMM_SUCCESS != ret )
		{
			printf("ERROR: http2dbs_getProfile\n");
			goto json_ack;
		}

		/* 4. set parameters from glbJsonVar to myProfile */
		ret = jsonSetCnuPrepare(&myProfile);
		if( CMM_SUCCESS != ret )
		{
			printf("ERROR: jsonSetCnuPrepare\n");
			goto json_ack;
		}

		/* 5. set profile to databases */
		ret = http2dbs_setProfile(iNode.cnu, &myProfile);
		if( CMM_SUCCESS != ret )
		{
			printf("ERROR: http2dbs_setProfile\n");
			goto json_ack;
		}

		/* 6. permit/undo-permit cnu */
		if( 1 == glbJsonVar.cnuPermit )
		{
			ret = http2cmm_permitCnu(iNode.cnu);
			if( CMM_SUCCESS != ret )
			{
				printf("ERROR: http2cmm_permitCnu\n");
				goto json_ack;
			}
		}
		else
		{
			ret = http2cmm_undoPermitCnu(iNode.cnu);
			if( CMM_SUCCESS != ret )
			{
				printf("ERROR: http2cmm_undoPermitCnu\n");
				goto json_ack;
			}
		}

		/* 7. reload profile for cnu */
		ret = http2cmm_reloadCnu(iNode.cnu);
		if( CMM_SUCCESS != ret )
		{
			printf("ERROR: http2cmm_reloadCnu\n");
			goto json_ack;
		}
	}
	else
	{
		if( cnu.col_sts == 0 )
		{
			printf("INFO: cnu is off line\n");
			ret = CMM_FAILED;
			goto json_ack;
		}
		
		/* get switch settings before write mod */
		ret = http2cmm_getSwitchSettings(&iNode, &rtl8306e);
		if( CMM_SUCCESS != ret )
		{
			printf("ERROR: http2cmm_getSwitchSettings\n");
			goto json_ack;
		}

		cnu.col_auth = (1==glbJsonVar.cnuPermit)?1:0;
		http2dbs_setCnu(iNode.cnu, &cnu);
		
		/* modify settings */
		for( i=0; i<5; i++ )
		{
			rtl8306e.portControl.port[i].enable = (1==glbJsonVar.cnuPermit)?1:0;
		}
		
		rtl8306e.vlanConfig.vlan_enable = (1==glbJsonVar.cnuVlanSts)?1:0;

		
		if(rtl8306e.vlanConfig.vlan_enable)
		{
			rtl8306e.vlanConfig.vlan_tag_aware = 1;
			rtl8306e.vlanConfig.ingress_filter = 0;
			rtl8306e.vlanConfig.g_admit_control = 0;
			rtl8306e.vlanConfig.vlan_port[0].pvid = glbJsonVar.cnuEth1Vid;
			rtl8306e.vlanConfig.vlan_port[1].pvid = glbJsonVar.cnuEth2Vid;
			rtl8306e.vlanConfig.vlan_port[2].pvid = glbJsonVar.cnuEth3Vid;
			rtl8306e.vlanConfig.vlan_port[3].pvid = glbJsonVar.cnuEth4Vid;
			rtl8306e.vlanConfig.vlan_port[4].pvid = 1;
			if(1 != rtl8306e.vlanConfig.vlan_port[0].pvid ) rtl8306e.vlanConfig.vlan_port[0].egress_mode = 2; else rtl8306e.vlanConfig.vlan_port[0].egress_mode = 1;
			if(1 != rtl8306e.vlanConfig.vlan_port[1].pvid ) rtl8306e.vlanConfig.vlan_port[1].egress_mode = 2; else rtl8306e.vlanConfig.vlan_port[1].egress_mode = 1;
			if(1 != rtl8306e.vlanConfig.vlan_port[2].pvid ) rtl8306e.vlanConfig.vlan_port[2].egress_mode = 2; else rtl8306e.vlanConfig.vlan_port[2].egress_mode = 1;
			if(1 != rtl8306e.vlanConfig.vlan_port[3].pvid ) rtl8306e.vlanConfig.vlan_port[3].egress_mode = 2; else rtl8306e.vlanConfig.vlan_port[3].egress_mode = 1;
			rtl8306e.vlanConfig.vlan_port[4].egress_mode = 3;
			rtl8306e.vlanConfig.vlan_port[0].admit_control = 0;
			rtl8306e.vlanConfig.vlan_port[1].admit_control = 0;
			rtl8306e.vlanConfig.vlan_port[2].admit_control = 0;
			rtl8306e.vlanConfig.vlan_port[3].admit_control = 0;
			rtl8306e.vlanConfig.vlan_port[4].admit_control = 0;
		}
		else
		{
			rtl8306e.vlanConfig.vlan_tag_aware = 0;
			rtl8306e.vlanConfig.ingress_filter = 0;
			rtl8306e.vlanConfig.g_admit_control = 0;
			rtl8306e.vlanConfig.vlan_port[0].pvid = 1;
			rtl8306e.vlanConfig.vlan_port[1].pvid = 1;
			rtl8306e.vlanConfig.vlan_port[2].pvid = 1;
			rtl8306e.vlanConfig.vlan_port[3].pvid = 1;
			rtl8306e.vlanConfig.vlan_port[4].pvid = 1;
			rtl8306e.vlanConfig.vlan_port[0].egress_mode = 3;
			rtl8306e.vlanConfig.vlan_port[1].egress_mode = 3;
			rtl8306e.vlanConfig.vlan_port[2].egress_mode = 3;
			rtl8306e.vlanConfig.vlan_port[3].egress_mode = 3;
			rtl8306e.vlanConfig.vlan_port[4].egress_mode = 3;
			rtl8306e.vlanConfig.vlan_port[0].admit_control = 0;
			rtl8306e.vlanConfig.vlan_port[1].admit_control = 0;
			rtl8306e.vlanConfig.vlan_port[2].admit_control = 0;
			rtl8306e.vlanConfig.vlan_port[3].admit_control = 0;
			rtl8306e.vlanConfig.vlan_port[4].admit_control = 0;
		}
		
		rtl8306e.bandwidthConfig.g_rx_bandwidth_control_enable = (1==glbJsonVar.cnuRxRateLimitSts)?1:0;
		if(rtl8306e.bandwidthConfig.g_rx_bandwidth_control_enable)
		{
			if (glbJsonVar.cnuCpuPortRxRate == 0)
			{
				rtl8306e.bandwidthConfig.rxPort[4].bandwidth_value = 2047;
			}
			else 
			{
				rtl8306e.bandwidthConfig.rxPort[4].bandwidth_value = (glbJsonVar.cnuCpuPortRxRate*32)/64;
			}
			if (glbJsonVar.cnuEth1RxRate == 0)
			{
				rtl8306e.bandwidthConfig.rxPort[0].bandwidth_value = 2047;
			}
			else 
			{
				rtl8306e.bandwidthConfig.rxPort[0].bandwidth_value = (glbJsonVar.cnuEth1RxRate*32)/64;
			}
			if (glbJsonVar.cnuEth2RxRate == 0)
			{
				rtl8306e.bandwidthConfig.rxPort[1].bandwidth_value = 2047;
			}
			else
			{
				rtl8306e.bandwidthConfig.rxPort[1].bandwidth_value = (glbJsonVar.cnuEth2RxRate*32)/64;
			}
			if (glbJsonVar.cnuEth3RxRate == 0)
			{
				rtl8306e.bandwidthConfig.rxPort[2].bandwidth_value = 2047;
			}
			else
			{
				rtl8306e.bandwidthConfig.rxPort[2].bandwidth_value = (glbJsonVar.cnuEth3RxRate*32)/64;
			}
			if (glbJsonVar.cnuEth4RxRate == 0)
			{
				rtl8306e.bandwidthConfig.rxPort[3].bandwidth_value = 2047;
			}
			else
			{
				rtl8306e.bandwidthConfig.rxPort[3].bandwidth_value = (glbJsonVar.cnuEth4RxRate*32)/64;
			}
			rtl8306e.bandwidthConfig.rxPort[0].bandwidth_control_enable = 1;
			rtl8306e.bandwidthConfig.rxPort[0].bandwidth_control_enable = 1;
			rtl8306e.bandwidthConfig.rxPort[1].bandwidth_control_enable = 1;
			rtl8306e.bandwidthConfig.rxPort[2].bandwidth_control_enable = 1;
			rtl8306e.bandwidthConfig.rxPort[3].bandwidth_control_enable = 1;
			rtl8306e.bandwidthConfig.rxPort[4].bandwidth_control_enable = 1;
		}
		else
		{
			rtl8306e.bandwidthConfig.rxPort[4].bandwidth_value = 2047;
			rtl8306e.bandwidthConfig.rxPort[0].bandwidth_value = 2047;
			rtl8306e.bandwidthConfig.rxPort[1].bandwidth_value = 2047;
			rtl8306e.bandwidthConfig.rxPort[2].bandwidth_value = 2047;
			rtl8306e.bandwidthConfig.rxPort[3].bandwidth_value = 2047;
			rtl8306e.bandwidthConfig.rxPort[0].bandwidth_control_enable = 0;
			rtl8306e.bandwidthConfig.rxPort[0].bandwidth_control_enable = 0;
			rtl8306e.bandwidthConfig.rxPort[1].bandwidth_control_enable = 0;
			rtl8306e.bandwidthConfig.rxPort[2].bandwidth_control_enable = 0;
			rtl8306e.bandwidthConfig.rxPort[3].bandwidth_control_enable = 0;
			rtl8306e.bandwidthConfig.rxPort[4].bandwidth_control_enable = 0;
		}
		
		rtl8306e.bandwidthConfig.g_tx_bandwidth_control_enable = (1==glbJsonVar.cnuTxRateLimitSts)?1:0;
		if(rtl8306e.bandwidthConfig.g_tx_bandwidth_control_enable)
		{
			if (glbJsonVar.cnuCpuPortTxRate == 0)
			{
				rtl8306e.bandwidthConfig.txPort[4].bandwidth_value = 2047;
			}
			else 
			{
				rtl8306e.bandwidthConfig.txPort[4].bandwidth_value = (glbJsonVar.cnuCpuPortTxRate*32)/64;
			}
			if (glbJsonVar.cnuEth1TxRate == 0)
			{
				rtl8306e.bandwidthConfig.txPort[0].bandwidth_value = 2047;
			}
			else 
			{
				rtl8306e.bandwidthConfig.txPort[0].bandwidth_value = (glbJsonVar.cnuEth1TxRate*32)/64;
			}
			if (glbJsonVar.cnuEth2TxRate == 0)
			{
				rtl8306e.bandwidthConfig.txPort[1].bandwidth_value = 2047;
			}
			else
			{
				rtl8306e.bandwidthConfig.txPort[1].bandwidth_value = (glbJsonVar.cnuEth2TxRate*32)/64;
			}
			if (glbJsonVar.cnuEth3TxRate == 0)
			{
				rtl8306e.bandwidthConfig.txPort[2].bandwidth_value = 2047;
			}
			else
			{
				rtl8306e.bandwidthConfig.txPort[2].bandwidth_value = (glbJsonVar.cnuEth3TxRate*32)/64;
			}
			if (glbJsonVar.cnuEth4TxRate == 0)
			{
				rtl8306e.bandwidthConfig.txPort[3].bandwidth_value = 2047;
			}
			else
			{
				rtl8306e.bandwidthConfig.txPort[3].bandwidth_value = (glbJsonVar.cnuEth4TxRate*32)/64;
			}
			rtl8306e.bandwidthConfig.txPort[0].bandwidth_control_enable = 1;
			rtl8306e.bandwidthConfig.txPort[1].bandwidth_control_enable = 1;
			rtl8306e.bandwidthConfig.txPort[2].bandwidth_control_enable = 1;
			rtl8306e.bandwidthConfig.txPort[3].bandwidth_control_enable = 1;
			rtl8306e.bandwidthConfig.txPort[4].bandwidth_control_enable = 1;
		}
		else
		{
			rtl8306e.bandwidthConfig.txPort[4].bandwidth_value = 2047;
			rtl8306e.bandwidthConfig.txPort[0].bandwidth_value = 2047;
			rtl8306e.bandwidthConfig.txPort[1].bandwidth_value = 2047;
			rtl8306e.bandwidthConfig.txPort[2].bandwidth_value = 2047;
			rtl8306e.bandwidthConfig.txPort[3].bandwidth_value = 2047;
			rtl8306e.bandwidthConfig.txPort[0].bandwidth_control_enable = 0;
			rtl8306e.bandwidthConfig.txPort[1].bandwidth_control_enable = 0;
			rtl8306e.bandwidthConfig.txPort[2].bandwidth_control_enable = 0;
			rtl8306e.bandwidthConfig.txPort[3].bandwidth_control_enable = 0;
			rtl8306e.bandwidthConfig.txPort[4].bandwidth_control_enable = 0;
		}	

		//cnu.col_auth = 1;
		/* write to device */
		ret = http2cmm_setSwitchSettings(&iNode, &rtl8306e);
		if( CMM_SUCCESS != ret )
		{
			printf("ERROR: http2cmm_setSwitchSettings\n");
			goto json_ack;
		}
		
	}

json_ack:
	/* send ack to nms */
	my_object = json_object_new_object();
	json_object_object_add(my_object, "status", json_object_new_int(ret?1:0));
	jsonSendAck(fs, CMM_SUCCESS, json_object_to_json_string(my_object));
	/* free json object !!! */
	json_object_put(my_object);

	/* write opt-log here */
	clt_index = (iNode.cnu -1)/MAX_CNUS_PER_CLT + 1;
	cnu_index = (iNode.cnu -1)%MAX_CNUS_PER_CLT + 1;
	sprintf(strlog, "json set profile to cnu/%d/%d", clt_index, cnu_index);
	http2dbs_writeOptlog(ret, strlog);
	
	return CMM_SUCCESS;
}

/* check input parameters , return 0: success; else failed */
int jsonGetCnuCheckInput(void)
{
	return CMM_SUCCESS;
}

/* prepare setting parameters to profile, return 0: success; else failed */
int jsonGetCnuPrepare(st_dbsProfile * profile, st_dbsCnu * cnu)
{
	/* over write vlan settings */
	glbJsonVar.cnuVlanSts = profile->col_vlanSts?1:2;
	if(profile->col_vlanSts)
	{
		glbJsonVar.cnuEth1Vid = profile->col_eth1vid;
		glbJsonVar.cnuEth2Vid = profile->col_eth2vid;
		glbJsonVar.cnuEth3Vid = profile->col_eth3vid;
		glbJsonVar.cnuEth4Vid = profile->col_eth4vid;
	}
	else
	{
		glbJsonVar.cnuEth1Vid = 1;
		glbJsonVar.cnuEth2Vid = 1;
		glbJsonVar.cnuEth3Vid = 1;
		glbJsonVar.cnuEth4Vid = 1;
	}

	/* over write rate limit settings*/
	glbJsonVar.cnuRxRateLimitSts = profile->col_rxLimitSts?1:2;
	if(profile->col_rxLimitSts)
	{
		glbJsonVar.cnuCpuPortRxRate = profile->col_cpuPortRxRate;
		glbJsonVar.cnuEth1RxRate = profile->col_eth1rx;
		glbJsonVar.cnuEth2RxRate = profile->col_eth2rx;
		glbJsonVar.cnuEth3RxRate = profile->col_eth3rx;
		glbJsonVar.cnuEth4RxRate = profile->col_eth4rx;
	}
	else
	{
		glbJsonVar.cnuCpuPortRxRate = 0;
		glbJsonVar.cnuEth1RxRate = 0;
		glbJsonVar.cnuEth2RxRate = 0;
		glbJsonVar.cnuEth3RxRate = 0;
		glbJsonVar.cnuEth4RxRate = 0;
	}
	glbJsonVar.cnuTxRateLimitSts = profile->col_txLimitSts?1:2;
	if(profile->col_txLimitSts)
	{
		glbJsonVar.cnuCpuPortTxRate = profile->col_cpuPortTxRate;
		glbJsonVar.cnuEth1TxRate = profile->col_eth1tx;
		glbJsonVar.cnuEth2TxRate = profile->col_eth2tx;
		glbJsonVar.cnuEth3TxRate = profile->col_eth3tx;
		glbJsonVar.cnuEth4TxRate = profile->col_eth4tx;
	}
	else
	{
		glbJsonVar.cnuCpuPortTxRate = 0;
		glbJsonVar.cnuEth1TxRate = 0;
		glbJsonVar.cnuEth2TxRate = 0;
		glbJsonVar.cnuEth3TxRate = 0;
		glbJsonVar.cnuEth4TxRate = 0;
	}

	/* cnu permit */
	if( 0 == cnu->col_row_sts )
		glbJsonVar.cnuPermit = 2;
	else
		glbJsonVar.cnuPermit = cnu->col_auth?1:2;
	
	return CMM_SUCCESS;
}

int jsonGetCnuProfile(FILE * fs)
{	
	int ret = CMM_SUCCESS;
	int i = 0;
	char strlog[128] = {0};
	stCnuNode iNode;
	int clt_index = 0;
	int cnu_index = 0;
	st_dbsProfile myProfile;
	st_rtl8306eSettings rtl8306e;
	st_dbsCnu cnu;
	json_object *my_object;

	/* for debug */
	//printf("\n-->call jsonGetCnuProfile()\n");
	
	/* process json get request here */
	ret = boardapi_mac2Uppercase(glbJsonVar.macAddr);
	if( CMM_SUCCESS != ret )
	{
		printf("ERROR: jsonConvertMac2Uppercase\n");
		goto json_out;
	}
	/* 1. get cnu index by input mac address */
	ret = http2dbs_getCnuIndexByMacaddress(glbJsonVar.macAddr, &iNode);
	if( CMM_SUCCESS != ret )
	{	
		/* system error */
		printf("ERROR: selectCnuIndex(%s)\n", glbJsonVar.macAddr);
		goto json_out;
	}
	else if( 0 == iNode.cnu )
	{
		/* can not select this cnu */
		printf("ERROR: selectCnuIndex\n");
		ret = CMM_FAILED;
		goto json_out;
	}

	/* 2. check input */
	ret = jsonGetCnuCheckInput();
	if( CMM_SUCCESS != ret )
	{
		printf("ERROR: jsonGetCnuCheckInput\n");
		goto json_out;
	}

	/* 3. get cnu permit status */
	ret = http2dbs_getCnu(iNode.cnu, &cnu);
	if( CMM_SUCCESS != ret )
	{
		printf("ERROR: http2dbs_getCnu\n");
		goto json_out;
	}

	if( CNU_SWITCH_TYPE_AR8236 == boardapi_getCnuSwitchType(cnu.col_model ))
	{
		/* 4. get profile by id */
		ret = http2dbs_getProfile(iNode.cnu, &myProfile);
		if( CMM_SUCCESS != ret )
		{
			printf("ERROR: http2dbs_getProfile\n");
			goto json_out;
		}
	}
	else
	{
		if( cnu.col_sts == 0 )
		{
			printf("INFO: cnu is off line\n");
			ret = CMM_FAILED;
			goto json_out;
		}
		/* get switch settings */
		ret = http2cmm_getSwitchSettings(&iNode, &rtl8306e);
		if( CMM_SUCCESS != ret )
		{
			printf("ERROR: http2cmm_getSwitchSettings\n");
			goto json_out;
		}
		myProfile.col_vlanSts = rtl8306e.vlanConfig.vlan_enable;
		myProfile.col_eth1vid = rtl8306e.vlanConfig.vlan_port[0].pvid;
		myProfile.col_eth2vid = rtl8306e.vlanConfig.vlan_port[1].pvid;
		myProfile.col_eth3vid = rtl8306e.vlanConfig.vlan_port[2].pvid;
		myProfile.col_eth4vid = rtl8306e.vlanConfig.vlan_port[3].pvid;
		myProfile.col_rxLimitSts = rtl8306e.bandwidthConfig.g_rx_bandwidth_control_enable;
		if (rtl8306e.bandwidthConfig.rxPort[4].bandwidth_value >= 1600)
		{
			myProfile.col_cpuPortRxRate = 3200;
		}
		else
		{
			myProfile.col_cpuPortRxRate = rtl8306e.bandwidthConfig.rxPort[4].bandwidth_value*2;	
		}
		if (rtl8306e.bandwidthConfig.rxPort[0].bandwidth_value >= 1600)
		{
			myProfile.col_eth1rx = 3200;
		}
		else
		{
			myProfile.col_eth1rx = rtl8306e.bandwidthConfig.rxPort[0].bandwidth_value*2;
		}
		if (rtl8306e.bandwidthConfig.rxPort[1].bandwidth_value >= 1600)
		{
			myProfile.col_eth2rx = 3200;
		}
		else
		{
			myProfile.col_eth2rx = rtl8306e.bandwidthConfig.rxPort[1].bandwidth_value*2;
		}
		if (rtl8306e.bandwidthConfig.rxPort[2].bandwidth_value >=1600)
		{
			myProfile.col_eth3rx = 3200;
		}
		else
		{
			myProfile.col_eth3rx = rtl8306e.bandwidthConfig.rxPort[2].bandwidth_value*2;
		}
		if (rtl8306e.bandwidthConfig.rxPort[3].bandwidth_value >=1600)
		{
			myProfile.col_eth4rx = 3200;
		}
		else 
		{
			myProfile.col_eth4rx = rtl8306e.bandwidthConfig.rxPort[3].bandwidth_value*2;
		}		
		myProfile.col_txLimitSts = rtl8306e.bandwidthConfig.g_tx_bandwidth_control_enable;
		if (rtl8306e.bandwidthConfig.txPort[4].bandwidth_value >=1600)
		{
			myProfile.col_cpuPortTxRate = 3200;
		}
		else
		{
			myProfile.col_cpuPortTxRate= rtl8306e.bandwidthConfig.txPort[4].bandwidth_value*2;
		}
		if (rtl8306e.bandwidthConfig.txPort[0].bandwidth_value >=1600)
		{
			myProfile.col_eth1tx = 3200;
		}
		else 
		{
			myProfile.col_eth1tx = rtl8306e.bandwidthConfig.txPort[0].bandwidth_value*2;
		}
		if (rtl8306e.bandwidthConfig.txPort[1].bandwidth_value >=1600)
		{
			myProfile.col_eth2tx = 3200;
		}
		else
		{
			myProfile.col_eth2tx = rtl8306e.bandwidthConfig.txPort[1].bandwidth_value*2;
		}
		if (rtl8306e.bandwidthConfig.txPort[2].bandwidth_value >=1600)
		{
			myProfile.col_eth3tx = 3200;
		}
		else
		{
			myProfile.col_eth3tx = rtl8306e.bandwidthConfig.txPort[2].bandwidth_value*2;
		}
		if (rtl8306e.bandwidthConfig.txPort[3].bandwidth_value >=1600)
		{
			myProfile.col_eth4tx =3200;
		}
		else
		{
			myProfile.col_eth4tx = rtl8306e.bandwidthConfig.txPort[3].bandwidth_value*2;
		}
		cnu.col_auth = rtl8306e.portControl.port[4].enable;
	}
	
	/* 5. get parameters from myProfile and cnu to glbJsonVar */
	ret = jsonGetCnuPrepare(&myProfile, &cnu);
	if( CMM_SUCCESS != ret )
	{
		printf("ERROR: jsonGetCnuPrepare\n");
		goto json_out;
	}

json_out:	
	/* send ack to NMS */
	my_object = json_object_new_object();
	json_object_object_add(my_object, "status", json_object_new_int(ret?1:0));
	if( CMM_SUCCESS == ret )
	{
		for ( i = 0; jsonSetTable[i].variable != NULL; i++ )
		{
			switch ( jsonSetTable[i].type )
			{
				case CGI_TYPE_STR:
				{
					json_object_object_add(my_object, jsonSetTable[i].variable, json_object_new_string(jsonSetTable[i].value));
					break;
				}
				case CGI_TYPE_NUM:
				{
					json_object_object_add(my_object, jsonSetTable[i].variable, json_object_new_int(*((int *)jsonSetTable[i].value)));
					break;
				}
			}
		}		
	}
	jsonSendAck(fs, CMM_SUCCESS, json_object_to_json_string(my_object));
	json_object_put(my_object);

	/* write opt-log here */
	clt_index = (iNode.cnu -1)/MAX_CNUS_PER_CLT + 1;
	cnu_index = (iNode.cnu -1)%MAX_CNUS_PER_CLT + 1;
	sprintf(strlog, "json get profile from cnu/%d/%d", clt_index, cnu_index);
	http2dbs_writeOptlog(ret, strlog);
	
	return CMM_SUCCESS;
}

/******************************************************************************************
curl -X "POST"  -H "Application/json" http://192.168.1.150/getcnu.json -d "{'mac':'30:71:B2:00:02:1E'}"
*******************************************************************************************/
int do_json(char *path, FILE *fs, int jstrLen)
{
	char jsonString[JSON_STRING_SIZE] = {0};	

	/* get post json string */
	fgets(jsonString, JSON_STRING_SIZE, fs);

	if( jstrLen>= JSON_STRING_SIZE )
	{
		printf("ERROR: json string length is out of range\n");
		jsonSendAck(fs, -1, NULL );
		return 0;
	}
	
	/* for debug */
	json_debug(jsonString, jstrLen);
	
	/* parse jason string and save in glbJsonVar */	
	jsonParseSet(jsonString);

	/* process json request */
	if ( strcmp( path, "/setcnu.json" ) == 0 )
	{
		return jsonSetCnuProfile(fs);
	}
	else if ( strcmp( path, "/getcnu.json" ) == 0 )
	{
		return jsonGetCnuProfile(fs);
	}
	else
	{
		jsonSendAck(fs, -1, NULL );
		return 0;
	}
}

