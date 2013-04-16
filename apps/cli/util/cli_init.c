/****************************************************************************
 �ļ���  : CLI_Init.c
 ����    : liuzequn
 �汾    :
 �������:
 �ļ�����:  ���ļ�ʵ��������ģ����Ҫ���ֵĳ�ʼ������
 �޸���ʷ:
        1. �޸���   :
           ʱ��     :
           �汾     :
           �޸�ԭ�� :
 ****************************************************************************/
#ifdef __cplusplus
extern "C" {
#endif

#include "cli_private.h"
#include "cli_term.h"
#include "cli_interpret.h"
#include "cli_io.h"
#include "../app/cli_cmd.h"
#include "cli_cmdreg.h"
#include "../cli_comm.h"
#include <signal.h>

/* demo version info */
char               m_szHostName[32] = "CLI";             /* �����豸��     */
//char               m_szHostName[32] = "WEC9720EK";             /* �����豸��     */

// ģʽע����Ϣ
ST_CMD_MODE_INFO m_stCmdModeInfo[] =
  /* ģʽ       ��ģʽ       Ȩ��           ģʽ��      ��ʾ��       ��������ִ�в����������Ϣ */
{{CTM_GLOBAL,  NULL_MODE,  CLI_AL_QUERY,   "Global",   ">>"       , NULL, CLI_ML_NULL},
 {CTM_GENL,    CTM_GLOBAL,  CLI_AL_QUERY,   "Root",     ">"       , NULL, CLI_ML_NULL},
 //{CTM_CONFIG,  CTM_GENL,   CLI_AL_QUERY,   "Config",   ">>config>", NULL, CMDHELP_GENL_CM_CONFIG},
 /* �����޸�ģʽ*/
 //{CTM_INTERFACE,  CTM_GENL,   CLI_AL_ADMIN,   "Interface",   ">>interface>", NULL, CMDHELP_GENL_CM_INTERFACE},
 {CTM_IF_1_CLT,  CTM_GENL,   CLI_AL_ADMIN,   "clt/1",   ">>clt/1>", NULL, CMDHELP_GENL_CM_IF_CLT},
// {CTM_IF_2_CLT,  CTM_GENL,   CLI_AL_ADMIN,   "clt/2",   ">>clt/2>", NULL, CMDHELP_GENL_CM_IF_CLT},
 {CTM_IF_1_1_CNU,  CTM_GENL,   CLI_AL_ADMIN,   "cnu/1/1",   ">>cnu/1/1>", NULL, CMDHELP_GENL_CM_IF_CNU},
 {CTM_IF_1_2_CNU,  CTM_GENL,   CLI_AL_ADMIN,   "cnu/1/2",   ">>cnu/1/2>", NULL, CMDHELP_GENL_CM_IF_CNU},
 {CTM_IF_1_3_CNU,  CTM_GENL,   CLI_AL_ADMIN,   "cnu/1/3",   ">>cnu/1/3>", NULL, CMDHELP_GENL_CM_IF_CNU},
 {CTM_IF_1_4_CNU,  CTM_GENL,   CLI_AL_ADMIN,   "cnu/1/4",   ">>cnu/1/4>", NULL, CMDHELP_GENL_CM_IF_CNU},
 {CTM_IF_1_5_CNU,  CTM_GENL,   CLI_AL_ADMIN,   "cnu/1/5",   ">>cnu/1/5>", NULL, CMDHELP_GENL_CM_IF_CNU},
 {CTM_IF_1_6_CNU,  CTM_GENL,   CLI_AL_ADMIN,   "cnu/1/6",   ">>cnu/1/6>", NULL, CMDHELP_GENL_CM_IF_CNU},
 {CTM_IF_1_7_CNU,  CTM_GENL,   CLI_AL_ADMIN,   "cnu/1/7",   ">>cnu/1/7>", NULL, CMDHELP_GENL_CM_IF_CNU},
 {CTM_IF_1_8_CNU,  CTM_GENL,   CLI_AL_ADMIN,   "cnu/1/8",   ">>cnu/1/8>", NULL, CMDHELP_GENL_CM_IF_CNU},
 {CTM_IF_1_9_CNU,  CTM_GENL,   CLI_AL_ADMIN,   "cnu/1/9",   ">>cnu/1/9>", NULL, CMDHELP_GENL_CM_IF_CNU},
 {CTM_IF_1_10_CNU,  CTM_GENL,   CLI_AL_ADMIN,   "cnu/1/10",   ">>cnu/1/10>", NULL, CMDHELP_GENL_CM_IF_CNU},
 {CTM_IF_1_11_CNU,  CTM_GENL,   CLI_AL_ADMIN,   "cnu/1/11",   ">>cnu/1/11>", NULL, CMDHELP_GENL_CM_IF_CNU},
 {CTM_IF_1_12_CNU,  CTM_GENL,   CLI_AL_ADMIN,   "cnu/1/12",   ">>cnu/1/12>", NULL, CMDHELP_GENL_CM_IF_CNU},
 {CTM_IF_1_13_CNU,  CTM_GENL,   CLI_AL_ADMIN,   "cnu/1/13",   ">>cnu/1/13>", NULL, CMDHELP_GENL_CM_IF_CNU},
 {CTM_IF_1_14_CNU,  CTM_GENL,   CLI_AL_ADMIN,   "cnu/1/14",   ">>cnu/1/14>", NULL, CMDHELP_GENL_CM_IF_CNU},
 {CTM_IF_1_15_CNU,  CTM_GENL,   CLI_AL_ADMIN,   "cnu/1/15",   ">>cnu/1/15>", NULL, CMDHELP_GENL_CM_IF_CNU},
 {CTM_IF_1_16_CNU,  CTM_GENL,   CLI_AL_ADMIN,   "cnu/1/16",   ">>cnu/1/16>", NULL, CMDHELP_GENL_CM_IF_CNU},
 {CTM_IF_1_17_CNU,  CTM_GENL,   CLI_AL_ADMIN,   "cnu/1/17",   ">>cnu/1/17>", NULL, CMDHELP_GENL_CM_IF_CNU},
 {CTM_IF_1_18_CNU,  CTM_GENL,   CLI_AL_ADMIN,   "cnu/1/18",   ">>cnu/1/18>", NULL, CMDHELP_GENL_CM_IF_CNU},
 {CTM_IF_1_19_CNU,  CTM_GENL,   CLI_AL_ADMIN,   "cnu/1/19",   ">>cnu/1/19>", NULL, CMDHELP_GENL_CM_IF_CNU},
 {CTM_IF_1_20_CNU,  CTM_GENL,   CLI_AL_ADMIN,   "cnu/1/20",   ">>cnu/1/20>", NULL, CMDHELP_GENL_CM_IF_CNU},
 {CTM_IF_1_21_CNU,  CTM_GENL,   CLI_AL_ADMIN,   "cnu/1/21",   ">>cnu/1/21>", NULL, CMDHELP_GENL_CM_IF_CNU},
 {CTM_IF_1_22_CNU,  CTM_GENL,   CLI_AL_ADMIN,   "cnu/1/22",   ">>cnu/1/22>", NULL, CMDHELP_GENL_CM_IF_CNU},
 {CTM_IF_1_23_CNU,  CTM_GENL,   CLI_AL_ADMIN,   "cnu/1/23",   ">>cnu/1/23>", NULL, CMDHELP_GENL_CM_IF_CNU},
 {CTM_IF_1_24_CNU,  CTM_GENL,   CLI_AL_ADMIN,   "cnu/1/24",   ">>cnu/1/24>", NULL, CMDHELP_GENL_CM_IF_CNU},
 {CTM_IF_1_25_CNU,  CTM_GENL,   CLI_AL_ADMIN,   "cnu/1/25",   ">>cnu/1/25>", NULL, CMDHELP_GENL_CM_IF_CNU},
 {CTM_IF_1_26_CNU,  CTM_GENL,   CLI_AL_ADMIN,   "cnu/1/26",   ">>cnu/1/26>", NULL, CMDHELP_GENL_CM_IF_CNU},
 {CTM_IF_1_27_CNU,  CTM_GENL,   CLI_AL_ADMIN,   "cnu/1/27",   ">>cnu/1/27>", NULL, CMDHELP_GENL_CM_IF_CNU},
 {CTM_IF_1_28_CNU,  CTM_GENL,   CLI_AL_ADMIN,   "cnu/1/28",   ">>cnu/1/28>", NULL, CMDHELP_GENL_CM_IF_CNU},
 {CTM_IF_1_29_CNU,  CTM_GENL,   CLI_AL_ADMIN,   "cnu/1/29",   ">>cnu/1/29>", NULL, CMDHELP_GENL_CM_IF_CNU},
 {CTM_IF_1_30_CNU,  CTM_GENL,   CLI_AL_ADMIN,   "cnu/1/30",   ">>cnu/1/30>", NULL, CMDHELP_GENL_CM_IF_CNU},
 {CTM_IF_1_31_CNU,  CTM_GENL,   CLI_AL_ADMIN,   "cnu/1/31",   ">>cnu/1/31>", NULL, CMDHELP_GENL_CM_IF_CNU},
 {CTM_IF_1_32_CNU,  CTM_GENL,   CLI_AL_ADMIN,   "cnu/1/32",   ">>cnu/1/32>", NULL, CMDHELP_GENL_CM_IF_CNU},
 {CTM_IF_1_33_CNU,  CTM_GENL,   CLI_AL_ADMIN,   "cnu/1/33",   ">>cnu/1/33>", NULL, CMDHELP_GENL_CM_IF_CNU},
 {CTM_IF_1_34_CNU,  CTM_GENL,   CLI_AL_ADMIN,   "cnu/1/34",   ">>cnu/1/34>", NULL, CMDHELP_GENL_CM_IF_CNU},
 {CTM_IF_1_35_CNU,  CTM_GENL,   CLI_AL_ADMIN,   "cnu/1/35",   ">>cnu/1/35>", NULL, CMDHELP_GENL_CM_IF_CNU},
 {CTM_IF_1_36_CNU,  CTM_GENL,   CLI_AL_ADMIN,   "cnu/1/36",   ">>cnu/1/36>", NULL, CMDHELP_GENL_CM_IF_CNU},
 {CTM_IF_1_37_CNU,  CTM_GENL,   CLI_AL_ADMIN,   "cnu/1/37",   ">>cnu/1/37>", NULL, CMDHELP_GENL_CM_IF_CNU},
 {CTM_IF_1_38_CNU,  CTM_GENL,   CLI_AL_ADMIN,   "cnu/1/38",   ">>cnu/1/38>", NULL, CMDHELP_GENL_CM_IF_CNU},
 {CTM_IF_1_39_CNU,  CTM_GENL,   CLI_AL_ADMIN,   "cnu/1/39",   ">>cnu/1/39>", NULL, CMDHELP_GENL_CM_IF_CNU},
 {CTM_IF_1_40_CNU,  CTM_GENL,   CLI_AL_ADMIN,   "cnu/1/40",   ">>cnu/1/40>", NULL, CMDHELP_GENL_CM_IF_CNU},
 {CTM_IF_1_41_CNU,  CTM_GENL,   CLI_AL_ADMIN,   "cnu/1/41",   ">>cnu/1/41>", NULL, CMDHELP_GENL_CM_IF_CNU},
 {CTM_IF_1_42_CNU,  CTM_GENL,   CLI_AL_ADMIN,   "cnu/1/42",   ">>cnu/1/42>", NULL, CMDHELP_GENL_CM_IF_CNU},
 {CTM_IF_1_43_CNU,  CTM_GENL,   CLI_AL_ADMIN,   "cnu/1/43",   ">>cnu/1/43>", NULL, CMDHELP_GENL_CM_IF_CNU},
 {CTM_IF_1_44_CNU,  CTM_GENL,   CLI_AL_ADMIN,   "cnu/1/44",   ">>cnu/1/44>", NULL, CMDHELP_GENL_CM_IF_CNU},
 {CTM_IF_1_45_CNU,  CTM_GENL,   CLI_AL_ADMIN,   "cnu/1/45",   ">>cnu/1/45>", NULL, CMDHELP_GENL_CM_IF_CNU},
 {CTM_IF_1_46_CNU,  CTM_GENL,   CLI_AL_ADMIN,   "cnu/1/46",   ">>cnu/1/46>", NULL, CMDHELP_GENL_CM_IF_CNU},
 {CTM_IF_1_47_CNU,  CTM_GENL,   CLI_AL_ADMIN,   "cnu/1/47",   ">>cnu/1/47>", NULL, CMDHELP_GENL_CM_IF_CNU},
 {CTM_IF_1_48_CNU,  CTM_GENL,   CLI_AL_ADMIN,   "cnu/1/48",   ">>cnu/1/48>", NULL, CMDHELP_GENL_CM_IF_CNU},
 {CTM_IF_1_49_CNU,  CTM_GENL,   CLI_AL_ADMIN,   "cnu/1/49",   ">>cnu/1/49>", NULL, CMDHELP_GENL_CM_IF_CNU},
 {CTM_IF_1_50_CNU,  CTM_GENL,   CLI_AL_ADMIN,   "cnu/1/50",   ">>cnu/1/50>", NULL, CMDHELP_GENL_CM_IF_CNU},
 {CTM_IF_1_51_CNU,  CTM_GENL,   CLI_AL_ADMIN,   "cnu/1/51",   ">>cnu/1/51>", NULL, CMDHELP_GENL_CM_IF_CNU},
 {CTM_IF_1_52_CNU,  CTM_GENL,   CLI_AL_ADMIN,   "cnu/1/52",   ">>cnu/1/52>", NULL, CMDHELP_GENL_CM_IF_CNU},
 {CTM_IF_1_53_CNU,  CTM_GENL,   CLI_AL_ADMIN,   "cnu/1/53",   ">>cnu/1/53>", NULL, CMDHELP_GENL_CM_IF_CNU},
 {CTM_IF_1_54_CNU,  CTM_GENL,   CLI_AL_ADMIN,   "cnu/1/54",   ">>cnu/1/54>", NULL, CMDHELP_GENL_CM_IF_CNU},
 {CTM_IF_1_55_CNU,  CTM_GENL,   CLI_AL_ADMIN,   "cnu/1/55",   ">>cnu/1/55>", NULL, CMDHELP_GENL_CM_IF_CNU},
 {CTM_IF_1_56_CNU,  CTM_GENL,   CLI_AL_ADMIN,   "cnu/1/56",   ">>cnu/1/56>", NULL, CMDHELP_GENL_CM_IF_CNU},
 {CTM_IF_1_57_CNU,  CTM_GENL,   CLI_AL_ADMIN,   "cnu/1/57",   ">>cnu/1/57>", NULL, CMDHELP_GENL_CM_IF_CNU},
 {CTM_IF_1_58_CNU,  CTM_GENL,   CLI_AL_ADMIN,   "cnu/1/58",   ">>cnu/1/58>", NULL, CMDHELP_GENL_CM_IF_CNU},
 {CTM_IF_1_59_CNU,  CTM_GENL,   CLI_AL_ADMIN,   "cnu/1/59",   ">>cnu/1/59>", NULL, CMDHELP_GENL_CM_IF_CNU},
 {CTM_IF_1_60_CNU,  CTM_GENL,   CLI_AL_ADMIN,   "cnu/1/60",   ">>cnu/1/60>", NULL, CMDHELP_GENL_CM_IF_CNU},
 {CTM_IF_1_61_CNU,  CTM_GENL,   CLI_AL_ADMIN,   "cnu/1/61",   ">>cnu/1/61>", NULL, CMDHELP_GENL_CM_IF_CNU},
 {CTM_IF_1_62_CNU,  CTM_GENL,   CLI_AL_ADMIN,   "cnu/1/62",   ">>cnu/1/62>", NULL, CMDHELP_GENL_CM_IF_CNU},
 {CTM_IF_1_63_CNU,  CTM_GENL,   CLI_AL_ADMIN,   "cnu/1/63",   ">>cnu/1/63>", NULL, CMDHELP_GENL_CM_IF_CNU},
 {CTM_IF_1_64_CNU,  CTM_GENL,   CLI_AL_ADMIN,   "cnu/1/64",   ">>cnu/1/64>", NULL, CMDHELP_GENL_CM_IF_CNU},

 // TODO: �˴���Ӹ�������ģʽ

 {NULL_MODE}
};

/*********************************************************************/
/* �������� : CLI_ProcessInit()                                      */
/* �������� : ģ��ֶγ�ʼ������                                     */
/* ������� :                                                        */
/* ������� :                                                        */
/* ����     :                                                        */
/* �ϲ㺯�� :                                                        */
/* ������   :                                                        */
/* �޸ļ�¼ :                                                        */
/*********************************************************************/
ULONG  CLI_ProcessInit()
{
    ULONG  ulRet = TBS_SUCCESS;

    signal(SIGINT, CLI_ProcForCtrlC);

    ulRet += CLI_CommInit();

    /* ģʽ�ṹ��ʼ�� */
    CLI_InitSysModes();
    /* �ն�������ʼ��*/
    CLI_TermDataInit();
    /* ����������ʼ��  */
    ulRet += CLI_ResetEnviroment();

    /* ע������ģʽ��ģʽ���� */
    ulRet += CLI_InitCmdTree(m_stCmdModeInfo);

    /* --------��ģ������ע�Ὺʼλ��--------{                  */

	/* ע�᱾ģ������ */
    ulRet += CLI_SysCmdReg();

    // TODO: ��ģ������ע��

    /* --------��ģ������ע�����λ��--------}                  */


    /* ��ȫ���������ӵ�����ģʽ */
    ulRet += CLI_GlobalCmdLink();

    return ulRet;
}


void CLI_ProcessDestory()
{
	CLI_CommDestroy();	
	CLI_TermDestroy();
}

/*********************************************************************/
/* �������� : CLI_GetHostName()                                      */
/* �������� : �����ṩ����������ȡ����                               */
/* ������� :                                                        */
/* ������� :                                                        */
/* ����     : �������ַ���                                           */
/*********************************************************************/
char  *CLI_GetHostName(_VOID   )
{
    return m_szHostName;
}
    

#ifdef __cplusplus
}
#endif

