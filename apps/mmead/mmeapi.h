#ifndef __MME_API_H__
#define __MME_API_H__

#include "support/atheros/ihpapi/ihpapi.h"
#include "support/atheros/ihpapi/ihp.h"

/********************************************************************************************
*	��������:MME_Atheros_MsgNeRefresh
*	��������:����������Ԫ�ڵ���Ϣ���ų��Ѿ����ߵ��豸
*				   
*	����ֵ:��
*	����:frank
*	ʱ��:2010-07-23
*********************************************************************************************/
void MME_Atheros_MsgNeRefresh
(T_MME_SK_HANDLE *MME_SK, uint8_t ODA[], T_MMEAD_TOPOLOGY *pNEList);

/********************************************************************************************
*	��������:MME_Atheros_MsgGetNetWorkInfo
*	��������:ihpapi_GetNetworkInfo
*				   
*	����ֵ:�����Ƿ�ɹ���״̬��
*	����:frank
*	ʱ��:2010-07-23
*********************************************************************************************/
int MME_Atheros_MsgGetNetWorkInfo
(T_MME_SK_HANDLE *MME_SK, uint8_t ODA[], T_MMEAD_TOPOLOGY *pNEList);


/********************************************************************************************
*	��������:MME_Atheros_MsgGetNetWorkInfoStats
*	��������:ihpapi_GetNetworkInfoStats
*				   
*	����ֵ:�����Ƿ�ɹ���״̬��
*	����:Stan
*	ʱ��:2013-03-12
*********************************************************************************************/
int MME_Atheros_MsgGetNetWorkInfoStats
(T_MME_SK_HANDLE *MME_SK, uint8_t ODA[], T_MMEAD_TOPOLOGY *pNEList);


/********************************************************************************************
*	��������:MME_Atheros_MsgGetManufacturerInfo
*	��������:ihpapi_GetManufacturerInfo
*				   
*	����ֵ:�����Ƿ�ɹ���״̬��
*	����:frank
*	ʱ��:2010-07-23
*********************************************************************************************/
int MME_Atheros_MsgGetManufacturerInfo
(T_MME_SK_HANDLE *MME_SK, uint8_t ODA[], uint8_t *pManufacturerInfo);

/********************************************************************************************
*	��������:MME_Atheros_MsgGetSwVer
*	��������:��ȡ�豸������汾
*				   
*	����ֵ:�����Ƿ�ɹ���״̬��
*	����:frank
*	ʱ��:2010-07-23
*********************************************************************************************/
int MME_Atheros_MsgGetSwVer
(T_MME_SK_HANDLE *MME_SK, uint8_t ODA[], uint8_t *pStr);

/********************************************************************************************
*	��������:MME_Atheros_MsgGetCltMac
*	��������:��ȡCLT��MAC��ַ
*				   
*	����ֵ:�����Ƿ�ɹ���״̬��
*	����:frank
*	ʱ��:2010-07-23
*********************************************************************************************/
int MME_Atheros_MsgGetCltMac(T_MME_SK_HANDLE *MME_SK, uint8_t clt_mac[]);

/********************************************************************************************
*	��������:MME_Atheros_MsgResetDevice
*	��������:ihpapi_ResetDevice
*				   
*	����ֵ:�����Ƿ�ɹ���״̬��
*	����:frank
*	ʱ��:2010-07-23
*********************************************************************************************/
int MME_Atheros_MsgResetDevice(T_MME_SK_HANDLE *MME_SK, uint8_t ODA[]);

/********************************************************************************************
*	��������:MME_Atheros_MsgGetTopology
*	��������:��ȡ��Ԫ�ڵ���Ϣ
*				   
*	����ֵ:�����Ƿ�ɹ���״̬��
*	����:frank
*	ʱ��:2010-07-23
*********************************************************************************************/
int MME_Atheros_MsgGetTopology
(T_MME_SK_HANDLE *MME_SK, uint8_t ODA[], T_MMEAD_TOPOLOGY *pNEList);

/********************************************************************************************
*	��������:MME_Atheros_MsgGetTopology
*	��������:��ȡ��Ԫ�ڵ���Ϣ
*				   
*	����ֵ:�����Ƿ�ɹ���״̬��
*	����:frank
*	ʱ��:2010-07-23
*********************************************************************************************/
int MME_Atheros_MsgGetTopologyStats
(T_MME_SK_HANDLE *MME_SK, uint8_t ODA[], T_MMEAD_TOPOLOGY *pNEList);


/********************************************************************************************
*	��������:MME_Atheros_MsgGetRxToneMapInfo
*	��������:��ȡ��Ԫ�ڵ���Ϣ
*				   
*	����ֵ:�����Ƿ�ɹ���״̬��
*	����:frank
*	ʱ��:2010-07-23
*********************************************************************************************/
int MME_Atheros_MsgGetRxToneMapInfo
(
	T_MME_SK_HANDLE *MME_SK, 
	uint8_t ODA[], 
	ihpapi_toneMapCtl_t * inputToneMapInfo, 
	ihpapi_getRxToneMapData_t *outputToneMapInfo
);

/********************************************************************************************
*	��������:MME_Atheros_MsgGetTxToneMapInfo
*	��������:
*				   
*	����ֵ:�����Ƿ�ɹ���״̬��
*	����:frank
*	ʱ��:2010-07-23
*********************************************************************************************/
int MME_Atheros_MsgGetTxToneMapInfo
(
	T_MME_SK_HANDLE *MME_SK, 
	uint8_t ODA[], 
	ihpapi_toneMapCtl_t * inputToneMapInfo, 
	ihpapi_getToneMapData_t *outputToneMapInfo
);

/********************************************************************************************
*	��������:MME_Atheros_MsgGetConnectionInfo
*	��������:
*				   
*	����ֵ:�����Ƿ�ɹ���״̬��
*	����:frank
*	ʱ��:2010-07-23
*********************************************************************************************/
int MME_Atheros_MsgGetConnectionInfo
(
	T_MME_SK_HANDLE *MME_SK, 
	uint8_t ODA[], 
	ihpapi_connectCtl_t * inputConnectInfo, 
	ihpapi_getConnectInfoData_t *outputConnectInfo
);

/********************************************************************************************
*	��������:MME_Atheros_MsgGetNetInfo
*	��������:ihpapi_GetNetworkInfo
*				   
*	����ֵ:�����Ƿ�ɹ���״̬��
*	����:frank
*	ʱ��:2010-07-23
*********************************************************************************************/
int MME_Atheros_MsgGetNetInfo
(
	T_MME_SK_HANDLE *MME_SK, 
	uint8_t ODA[], 
	ihpapi_getNetworkInfoData_t *outputNetInfo
);

int MME_Atheros_MsgGetFrequencyBandSelection
(
	T_MME_SK_HANDLE *MME_SK, 
	uint8_t ODA[], 
	T_MMEAD_FBS *pdata
);

int MME_Atheros_MsgSetFrequencyBandSelection
(
	T_MME_SK_HANDLE *MME_SK, 
	uint8_t ODA[], 
	T_MMEAD_FBS *pdata
);

#endif

