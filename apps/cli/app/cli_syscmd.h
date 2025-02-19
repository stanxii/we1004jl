#ifndef __CLI_SYSCMD_H__
#define __CLI_CMD_FORMAT_H__

#define CLI_CMD_FORMAT_SCROLL "scroll: {automatic}|{manual}"
#define CLI_CMD_FORMAT_REBOOT "reboot"
#define CLI_CMD_FORMAT_SETPASSWORD "password: {admin}|{operator}|{user} <password(s15)>"
#define CLI_CMD_FORMAT_MIP "ip-address: <nipaddr(p)> <netmask(m)>"
#define CLI_CMD_FORMAT_MIPG "ip-gateway: <ipgateway(p)>"
#define CLI_CMD_FORMAT_SETMVLAN "mgmt-vlan: <vlan(u1~4094)>"
#define CLI_CMD_FORMAT_DIS_TOP "display topology"
#define CLI_CMD_FORMAT_DIS_OPT "display opt-log"
#define CLI_CMD_FORMAT_DIS_ALARM "display alarm-log"
#define CLI_CMD_FORMAT_DIS_SYSLOG "display sys-log"
#define CLI_CMD_FORMAT_DIS_NETWORK "display network-info"
#define CLI_CMD_FORMAT_DIS_SYSINFO "display sysinfo"
#define CLI_CMD_FORMAT_DIS_FTP "display ftp-server"
#define CLI_CMD_FORMAT_DIS_USERS "display user-white-list"
#define CLI_CMD_FORMAT_DIS_SNMP "display snmp"
#define CLI_CMD_FORMAT_UNDO_VLAN "undo mgmt-vlan"
#define CLI_CMD_FORMAT_UNDO_ADDRESS "undo ip-address"
#define CLI_CMD_FORMAT_UNDO_GATEWAY "undo ip-gateway"
#define CLI_CMD_FORMAT_FTP_SERVER "ftp-server set: {ip-address <ip(p)>}|{port <sport(u1~65535)>}|{user-name <username(s31)>}|{passwd <password(s15)>}|{file-path <filepath(s255)>}"
#define CLI_CMD_FORMAT_STE_SNMP "snmp set: {read-community <rc(s63)>}|{write-community <wc(s63)>}|{trap-community <tc(s63)>}|{trap-server <ts(p)>}|{trap-desport <port(u1~65535)>}"
#define CLI_CMD_FORMAT_UPGRADE "upgrade"
#define CLI_CMD_FORMAT_DEBUG "debug: {cmm}|{dbs}|{sql} {enable}|{disable}"
#define CLI_CMD_FORMAT_AR8236_SMI_REG "ar8236-reg: {read}|{write <regvalue(h0x00~0xffffffff)>} {register <regad(h0x00~0xffff)>}"
#define CLI_CMD_FORMAT_AR8236_SMI_PHY "ar8236-phy: {read}|{write <regvalue(h0x00~0xffff)>} {phy <phyad(h0~4)>} {register <regad(h0x00~0xff)>}"
#define CLI_CMD_FORMAT_CNU_SWITCH "cnu-switch: {read}|{write <regvalue(h0x0000~0xffff)>} {phy <phyad(u0~6)>} {register <regad(u0~32)>} {page <pageid(u0~3)>}"
#define CLI_CMD_FORMAT_CNU_ACL "acl-drop-mme"
#define CLI_CMD_FORMAT_MME_MDIO "mme-mdio: {read}|{write <regvalue(h0x00~0xffff)>} {phy <phyad(h0x0~0xf)>} {register <regad(h0x00~0xff)>}"
#define CLI_CMD_FORMAT_VLAN "vlan: {set <port(eth1|eth2|eth3|eth4)>} {vlan-id <vid(u1~4094)>}"
#define CLI_CMD_FORMAT_UNDO_CNU_VLAN "undo vlan"
#define CLI_CMD_FORMAT_SAVE "save-config"
#define CLI_CMD_FORMAT_RATELIMIT "rate-limit: {set <port(eth1|eth2|eth3|eth4|cpu-port)>} {uplink <uprate(u0~10240)> <upunit(Kb|Mb)>} {downlink <dwrate(u0~10240)> <dwunit(Kb|Mb)>}"
#define CLI_CMD_FORMAT_UNDO_CNU_RATE_LIMIT "undo rate-limit"
#define CLI_CMD_FORMAT_SHUTDOWN "shutdown: <port(eth1|eth2|eth3|eth4)>"
#define CLI_CMD_FORMAT_UNDO_SHUTDOWN "undo shutdown"
#define CLI_CMD_FORMAT_MAC_LIMIT "mac-limit set: {bridged-host-number <hosts(u0~8)>}"
#define CLI_CMD_FORMAT_UNDO_MAC_LIMIT "undo mac-limit"
#define CLI_CMD_FORMAT_AGING_TIME "aging-time set: {local <time(u1~2000)>}|{remote <time(u1~2000)>} {minutes}"
#define CLI_CMD_FORMAT_UNDO_AGING_TIME "undo aging-time"
#define CLI_CMD_FORMAT_QOS_TYPE "qos: {using <base(cos|tos)>}"
#define CLI_CMD_FORMAT_QOS_QUEUE_MAP "qos-mapping: {cos <vlanpri(u0~7)>}|{tos <tc(u0~7)>} {cap <queue(u0~3)>}"
#define CLI_CMD_FORMAT_UNDO_QOS "undo qos"
#define CLI_CMD_FORMAT_STORM_FILTER "storm-filter: {broadcast}|{unknown-unicast}|{unknown-multicast} {enable}|{disable}"
#define CLI_CMD_FORMAT_UNDO_STORM_FILTER "undo storm-filter"
#define CLI_CMD_FORMAT_DISPLAY_CUR_PROFILE "display cur-profile"
#define CLI_CMD_FORMAT_RESTORE "restore-default"
#define CLI_CMD_FORMAT_RELOAD_PROFILE "reload-profile"
#define CLI_CMD_FORMAT_WLIST_CONTROL "white-list: {enable}|{disable}"
#define CLI_CMD_FORMAT_WDT_CONTROL "wdt: {enable}|{disable}"
#define CLI_CMD_FORMAT_HB_CONTROL "heartbeat: {enable}|{disable}"
#define CLI_CMD_FORMAT_DELETE_CNU "delete: {cnu <index(s4)>}"
//#define CLI_CMD_FORMAT_USER_PERMIT "permit: {cnu <index(1/1|1/2|1/3|1/4|1/5|1/6|1/7|1/8|1/9|1/10|1/11|1/12|1/13|1/14|1/15|1/16|1/17|1/18|1/19|1/20|1/21|1/22|1/23|1/24|1/25|1/26|1/27|1/28|1/29|1/30|1/31|1/32|1/33|1/34|1/35|1/36|1/37|1/38|1/39|1/40|1/41|1/42|1/43|1/44|1/45|1/46|1/47|1/48|1/49|1/50|1/51|1/52|1/53|1/54|1/55|1/56|1/57|1/58|1/59|1/60|1/61|1/62|1/63|1/64)>}"
#define CLI_CMD_FORMAT_USER_PERMIT "permit: {cnu <index(s4)>}"
#define CLI_CMD_FORMAT_UNDO_USER_PERMIT "undo permit: {cnu <index(s4)>}"
#define CLI_CMD_FORMAT_UNDO_CNU_ACL "undo acl-drop-mme"
#define CLI_CMD_FORMAT_CREATE_CNU "create: {cnu <mac(c)>}"
#define CLI_CMD_FORMAT_DEBUG_DUMP "dump: {register}|{mod}|{pib}"
#define CLI_CMD_FORMAT_DSDT_DBG_STAS "dsdt-stats: {print}|{clear}"
#define CLI_CMD_FORMAT_DSDT_TIMING_DELAY "dsdt-rgmii-delay: {get}|{enable}|{disable} {port5}|{port6} {rx}|{tx}|{all}"
#define CLI_CMD_FORMAT_DSDT_PORT_MIRROR "dsdt-port-mirror: {from <sport(p0|p1|p2|p3|p4|p5|p6)>} {to <deport(p0|p1|p2|p3|p4|p5|p6)>}"
#define CLI_CMD_FORMAT_DSDT_MAC_BINDING "dsdt-binding: {mac-address <mac(c)>} {to <deport(p0|p1|p2|p3|p4|p5|p6)>}"

#endif

