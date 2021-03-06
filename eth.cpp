#include <RoomInf.h>
#include <ws2tcpip.h>
#include <Atlcoll.h>
#include "RoomInf.h"
#include "mmsystem.h"
#include "systemmsg.h"
#include"CanThread.h"
#include  "eth.h"
#include"gpio.h"
#include "IniFile.h"
#include "afx.h"
#include "MyTimer.h"
#include "SysMonitor.h"

//CAtlList<Ip_Vtype_table> NHandshake_list;//握手包list
CAtlList<Ip_Fromadd_table> NetIF_list;//全局list，实时更新ip与Fadd的关系
extern CAtlList<BufCan> BufCan_list;
char *NMC_ip;
BYTE handshack_value;
DWORD ip_head_len=sizeof(Network_Ipswich_TCP_HEAD);
DWORD InterruptMonitorAddr=InterMonitorAddrDefault;
BYTE session_num=0;

BYTE	Clock4Secs=Clock_Dis;
DWORD Clock5TenMSecs=ClockCleared;

DWORD Clock50MSecs=ClockCleared;
BYTE	Clock3Secs=Clock_Dis;

DWORD Clock60MSecs=ClockCleared;
BYTE	Clock6Secs=Clock_Dis;

DWORD Clock5TMSecs=ClockCleared;
BYTE	Clock5Mins=Clock_Dis;

DWORD ClockDelayCallIps=ClockCleared;
BYTE	ClockDelayTCallIps=Clock_Dis;


CAtlList<BYTE> hCallSessionList;
BYTE hCallSession;
CMedeiaLogicApp m_MedeiaLApp;
Network_Ipswich_TCP MSG_NMCHandshack;

HANDLE	CallEvent[CallEventNum];
BOOL HeartBeatTmp=FALSE;

BOOL SixSecindex=FALSE;

BYTE OpenvaUpMonitor=0;

extern void HandTimeDef();
extern BYTE VideoLenthLeveal;
extern BYTE VideoLevealSwitch;



//extern CTimerApp  *m_CTimerApp;

static BOOL ConnectManager(CTcpClientSock* sock)
{
	int ip;
	ip_get pget;

	if(!ManagerGet(&pget))
		return FALSE;
	ip = pget.param->ip;
	free(pget.param);
	
	return sock->Connect(pget.param->ip, NEWSERVER_PORT);
}

void IF_tableMap(Network_Ipswich_TCP MSG_ETHRec,sockaddr_in sin)
{
	Ip_Fromadd_table IF_table;
	if(NetIF_list.GetCount()==0)
	{
		IF_table.FromAddr=MSG_ETHRec.nitcphead.fromaddr;
		IF_table.FromIP=inet_addr(inet_ntoa(sin.sin_addr));
		NetIF_list.AddTail(IF_table);	
	}
	else
	{//地址映射表，确认返回数据包目的地址ip
		POSITION pos=NetIF_list.GetHeadPosition();//更新地址映射list
		BYTE i;
		for(i=0;i<NetIF_list.GetCount();i++)
		{
			if(NetIF_list.GetAt(pos).FromAddr==MSG_ETHRec.nitcphead.fromaddr)//同一网管号
			{
				if(NetIF_list.GetAt(pos).FromIP!=inet_addr(inet_ntoa(sin.sin_addr)))//不同ip
				{
					NetIF_list.RemoveAt(pos);
					IF_table.FromAddr=MSG_ETHRec.nitcphead.fromaddr;
					IF_table.FromIP=inet_addr(inet_ntoa(sin.sin_addr));
					NetIF_list.AddTail(IF_table);	
					DBGMSG(DPERROR, "same nmc:%d;diff ip:%d\r\n",IF_table.FromAddr,IF_table.FromIP);
					break;
				}
				else if(NetIF_list.GetAt(pos).FromIP==inet_addr(inet_ntoa(sin.sin_addr)))
					break;
			}
			else
				NetIF_list.GetNext(pos);
		}
		if (i==NetIF_list.GetCount())//链表查询完毕，未找到相同网管号或设备、即不同网管号或设备，均添加链表更新
		{
			IF_table.FromAddr=MSG_ETHRec.nitcphead.fromaddr;
			IF_table.FromIP=inet_addr(inet_ntoa(sin.sin_addr));
			NetIF_list.AddTail(IF_table);
			DBGMSG(DPERROR, "diff nmc:%d or diff ip:%d\r\n",IF_table.FromAddr,IF_table.FromIP);
		}	
	}
}


CMedeiaLogicApp::CMedeiaLogicApp()
{
	handshack_value=closeall;
	VideoLevealSwitch=0;
	VideoLenthLeveal=0;
	NmcNumNow=FreeStatus;
	NmcIpNow=FreeStatus;
}
CMedeiaLogicApp::~CMedeiaLogicApp()
{
	DBGMSG(DPINFO, ":~CMedeiaLogicApp\r\n");
}

BOOL CMedeiaLogicApp::Channel_switch(BYTE Switch_type,BOOL value)
{	
	if(!SetGpioVal(GIO[Switch_type],value))
	{
		DBGMSG(DPERROR, "Relay_switch(%d,%d) error is d%\r\n",Switch_type,value,GetLastError());
		return FALSE;
	}
	return TRUE;
}


void CMedeiaLogicApp::VideoLenth_switch(BYTE Switch_type)
{	
	switch(Switch_type)
	{
		case 0:
			Channel_switch(RelayK3_video,RelayCoilPower_OFF);
			Channel_switch(RelayK4_video,RelayCoilPower_OFF);
			Channel_switch(RelayK5_video,RelayCoilPower_OFF);
			Channel_switch(RelayK0_video,RelayCoilPower_OFF);
			DBGMSG(DPINFO, "VideoLenth_switch --->>0\r\n");
			break;
		case 1:
			Channel_switch(RelayK3_video,RelayCoilPower_OFF);
			Channel_switch(RelayK4_video,RelayCoilPower_OFF);
			Channel_switch(RelayK5_video,RelayCoilPower_OFF);
			Channel_switch(RelayK0_video,RelayCoilPower_ON);
			DBGMSG(DPINFO, "VideoLenth_switch --->>1\r\n");
			break;
		case 2:
			Channel_switch(RelayK3_video,RelayCoilPower_OFF);
			Channel_switch(RelayK4_video,RelayCoilPower_ON);
			Channel_switch(RelayK5_video,RelayCoilPower_OFF);
			Channel_switch(RelayK0_video,RelayCoilPower_ON);
			DBGMSG(DPINFO, "VideoLenth_switch --->>2\r\n");
			break;
		case 3:
			Channel_switch(RelayK3_video,RelayCoilPower_ON);
			Channel_switch(RelayK4_video,RelayCoilPower_OFF);
			Channel_switch(RelayK5_video,RelayCoilPower_OFF);
			Channel_switch(RelayK0_video,RelayCoilPower_ON);
			DBGMSG(DPINFO, "VideoLenth_switch --->>3\r\n");
			break;
		case 4:
			Channel_switch(RelayK3_video,RelayCoilPower_ON);
			Channel_switch(RelayK4_video,RelayCoilPower_OFF);
			Channel_switch(RelayK5_video,RelayCoilPower_ON);
			Channel_switch(RelayK0_video,RelayCoilPower_ON);
			DBGMSG(DPINFO, "VideoLenth_switch --->>4\r\n");
			break;
		default:
			break;
	}
}


void CMedeiaLogicApp::CloseMedia()
{
	while(hCallSessionList.GetCount())
	{
		hCallSessionList.RemoveHead();
	}
	handshack_value=closeall;//关断模拟通道
	VideoLevealSwitch=0;
	VideoLenthLeveal=0;
	OpenvaUpMonitor=0;
	VideoLenth_switch(0);

	ClockDelayCallIps=ClockCleared;
	ClockDelayTCallIps=Clock_Dis;

	Channel_switch(RelayK1_audio,RelayCoilPower_OFF);
	Channel_switch(RelayK2_video,RelayCoilPower_OFF);
	Channel_switch(audio_power,power_close);
	Channel_switch(video_power,power_close);
	DBGMSG(DPERROR, "CMedeiaLogicApp::CloseMedia()\r\n");
}
void CMedeiaLogicApp::OpenvideoUpMedia()
{	
	Channel_switch(audio_power,power_close);
	Channel_switch(video_power,power_open);	
	if(VideoLevealSwitch)
		VideoLenth_switch(VideoLenthLeveal);
	Channel_switch(RelayK1_audio,RelayCoilPower_OFF);
	Channel_switch(RelayK2_video,RelayCoilPower_OFF);	
	hCallSession=open_v_up;
	hCallSessionList.AddTail(hCallSession);
}
void CMedeiaLogicApp::OpenvaUpMonitorMedia()
{	
	Channel_switch(audio_power,power_open);
	Channel_switch(video_power,power_open);
	if(VideoLevealSwitch)
		VideoLenth_switch(VideoLenthLeveal);
	Channel_switch(RelayK1_audio,RelayCoilPower_OFF);//监视无需监听150912
	Channel_switch(RelayK2_video,RelayCoilPower_OFF); 
	hCallSession=open_va_up_monitor;
	hCallSessionList.AddTail(hCallSession);
}
void CMedeiaLogicApp::OpenvaUpCallMedia()
{	
	Channel_switch(audio_power,power_close);
	Channel_switch(video_power,power_open);
	if(VideoLevealSwitch)
		VideoLenth_switch(VideoLenthLeveal);
	Channel_switch(RelayK1_audio,RelayCoilPower_OFF);
	Channel_switch(RelayK2_video,RelayCoilPower_OFF);
	hCallSession=open_va_up_call;
	hCallSessionList.AddTail(hCallSession);
}
void CMedeiaLogicApp::OpenvaDownCallMedia()
{	
	Channel_switch(audio_power,power_close);
	Channel_switch(video_power,power_open);
	if(VideoLevealSwitch)
		VideoLenth_switch(VideoLenthLeveal);
	Channel_switch(RelayK1_audio,RelayCoilPower_OFF);
	Channel_switch(RelayK2_video,RelayCoilPower_ON);
	hCallSession=open_va_down_call;
	hCallSessionList.AddTail(hCallSession);
}
void CMedeiaLogicApp::AddOpenAudioMedia()
{
	Channel_switch(audio_power,power_open);
	Channel_switch(RelayK1_audio,RelayCoilPower_ON);//RelayCoilPower_ON-----------------20151126：此处实验室李恒跳线版本未OFF，当为正式杨工做的新版本硬件为ON
}

void SetNMCTimeToSystemTime(Network_Ipswich_TCP MSG_ETHRec)
{
	SYSTEMTIME SystemTime;
	GetLocalTime(&SystemTime);
	SystemTime.wYear=MSG_ETHRec.content[EthComConData]+2000;
	SystemTime.wMonth=MSG_ETHRec.content[EthComConData+1];
	SystemTime.wDay=MSG_ETHRec.content[EthComConData+2];
	SystemTime.wHour=MSG_ETHRec.content[EthComConData+3];
	SystemTime.wMinute=MSG_ETHRec.content[EthComConData+4];
	SystemTime.wSecond=0;
	SystemTime.wDayOfWeek=0;
	SystemTime.wMilliseconds=0;
	BOOL TimeType= SetLocalTime(&SystemTime);
	printf("TimeType is %d \r\n",TimeType);
	HANDLE hfile = CreateFile(_T("RTC1:"),GENERIC_READ|GENERIC_WRITE,0,NULL,OPEN_EXISTING,0,NULL);
	if(hfile == INVALID_HANDLE_VALUE)
	{
		printf("open RTC1 fail, error is %d \r\n",GetLastError());
	}
	CloseHandle(hfile);
}



BOOL ETHSend(Network_Ipswich_TCP MSG,char *ip,int port)
{
	//	char *SendCmd;
	//CTcpClientSock CSock;
	SOCKET sockClient;
	sockClient=TcpConnect(ip,port,CONNECT_TIMEOUT);
	if(sockClient == INVALID_SOCKET)
	{
		DBGMSG(DPINFO, "ETHSend Connect %s:%d fail\r\n",ip,port);
		SocketClose(sockClient);
		return FALSE;
	}
	DWORD ret=TcpSendData(sockClient,(char *)(&MSG),MSG.nitcphead.length,SEND_TIMEOUT);
	if(ret!=MSG.nitcphead.length)
	{
		DBGMSG(DPWARNING, "ETHSend Msg Failed\r\n"); 
		SocketClose(sockClient);
		return FALSE;
	}
	SocketClose(sockClient);
	return TRUE;
}

BOOL NMC_reply(Network_Ipswich_TCP MSG,char *ip,int port,BYTE type)
{
	Network_Ipswich_TCP TCPIP_reply;
	//CTcpClientSock CSock;	
	TCPIP_reply.nitcphead.fromaddr=MSG.nitcphead.toaddr;
	TCPIP_reply.nitcphead.toaddr=MSG.nitcphead.fromaddr;
	TCPIP_reply.nitcphead.headcommand=MSG.nitcphead.headcommand+0x80;
	TCPIP_reply.nitcphead.headmsgsession=MSG.nitcphead.headmsgsession;
	TCPIP_reply.nitcphead.length=handshack_length;
	TCPIP_reply.content[0]=MSG.content[0];//*content=data length
	TCPIP_reply.content[1]=MSG.nitcphead.headcommand+0x80;
	TCPIP_reply.content[2]=MSG.nitcphead.headmsgsession;
	//TCPIP_reply.value=MSG.content[2];
	TCPIP_reply.content[3]=type;
	if (ETHSend(TCPIP_reply,ip,IPS_SERVER_PORT))
	{
		DBGMSG(DPINFO, "NMC_reply is %d\r\n",type);
		session_num++;
		return TRUE;
	}
	else
	{
		DBGMSG(DPINFO, "NMC_reply wrong\r\n");
		return FALSE;
	}
}

BOOL SendDIg_handshack(DWORD Toaddr,DWORD ip,BYTE Valuetype)
{
	Network_Ipswich_TCP IN_handshack;
	ZeroMemory(&IN_handshack,sizeof(IN_handshack));
	//CTcpClientSock CSock;	
	struct in_addr addr;
	addr.S_un.S_addr=ip;
	
	IN_handshack.nitcphead.fromaddr=My_SAddr;
	IN_handshack.nitcphead.toaddr=Toaddr;
	IN_handshack.nitcphead.headcommand=CODET_Handshake;
	IN_handshack.nitcphead.headmsgsession=session_num;
	IN_handshack.nitcphead.length=handshack_length;
	IN_handshack.content[0]=handshack_length-sizeof(Network_Ipswich_TCP_HEAD)-1;
	IN_handshack.content[1]=CODET_Handshake;
	IN_handshack.content[2]=session_num;
	IN_handshack.content[3]=Valuetype;
	
	if (ETHSend(IN_handshack,inet_ntoa(addr),IPS_SERVER_PORT))
	{
		DBGMSG(DPINFO, "DIgHandshack_code is %d\r\n",Valuetype);
		session_num++;
		return TRUE;
	}
	else
	{
		DBGMSG(DPINFO, "DIgHandshack_code wrong\r\n");
		return FALSE;
	}	
}


void D0CODET_Handshake(Network_Ipswich_TCP MSG_ETHRec,sockaddr_in sin)
{
	BYTE CODEValue=EthData_lenthB+EthComSess;
	//Network_Ipswich_TCP MSG_NMCHandshack;
	
	switch (MSG_ETHRec.content[CODEValue])
	{	//音视频通道配置标志位
		case closeall:
			if (InterruptMonitorAddr==MSG_ETHRec.nitcphead.fromaddr)
			{
				NMC_reply(MSG_ETHRec,inet_ntoa(sin.sin_addr),IPS_SERVER_PORT,NMC_reply_code_ture_end);
				InterruptMonitorAddr=InterMonitorAddrDefault;
			}
			else if((MSG_ETHRec.nitcphead.fromaddr==MSG_NMCHandshack.nitcphead.fromaddr)&&(MSG_ETHRec.nitcphead.toaddr==MSG_NMCHandshack.nitcphead.toaddr))
			{
				Clock4Secs=Clock_Dis;
				Clock5TenMSecs=ClockCleared;
				NMC_reply(MSG_ETHRec,inet_ntoa(sin.sin_addr),IPS_SERVER_PORT,NMC_reply_code_ture_end);//回复此NMC确认包
				m_MedeiaLApp.CloseMedia();
				HandTimeDef();
				DPPostMessage(MSG_PHONECALL, MSG_CALL_HUNGUP, 0, 0);
				DBGMSG(DPINFO, "handshack closeall OK\r\n");
				//if((MSG_ETHRec.nitcphead.fromaddr==MSG_NMCHandshack.nitcphead.fromaddr)&&(MSG_ETHRec.nitcphead.toaddr==MSG_NMCHandshack.nitcphead.toaddr))
				//{
					ZeroMemory(&MSG_NMCHandshack,sizeof(MSG_NMCHandshack));//超时处添加清除网管握手存储标志
				//}
			}
			else
			{
				NMC_reply(MSG_ETHRec,inet_ntoa(sin.sin_addr),IPS_SERVER_PORT,NMC_reply_code_ture_end);//回复此NMC确认包
				DBGMSG(DPINFO, "Ex handshack closeall Reply\r\n");
			}
			break;
		case open_v_up://查看留影
			if(handshack_value==closeall)
			{
				m_MedeiaLApp.NmcNumNow=MSG_ETHRec.nitcphead.fromaddr;//static_cast<BYTE>
				m_MedeiaLApp.NmcIpNow=inet_addr(inet_ntoa(sin.sin_addr));
				m_MedeiaLApp.OpenvideoUpMedia();

				handshack_value=open_v_up;
				NMC_reply(MSG_ETHRec,inet_ntoa(sin.sin_addr),IPS_SERVER_PORT,NMC_reply_code_ture_start);
				Clock4Secs=Clock_En;
				Clock5TenMSecs=ClockCleared;
				DBGMSG(DPINFO, "handshack open_v_up OK\r\n");
				memcpy(&MSG_NMCHandshack,&MSG_ETHRec,sizeof(Network_Ipswich_TCP));

			}
			else if ((handshack_value==open_v_up)&&(MSG_ETHRec.nitcphead.fromaddr==MSG_NMCHandshack.nitcphead.fromaddr)&&(MSG_ETHRec.nitcphead.toaddr==MSG_NMCHandshack.nitcphead.toaddr))
			{
				NMC_reply(MSG_ETHRec,inet_ntoa(sin.sin_addr),IPS_SERVER_PORT,NMC_reply_code_ture_start);
				Clock4Secs=Clock_En;
				Clock5TenMSecs=ClockCleared;
				DBGMSG(DPINFO, "handshack open_v_up OK again\r\n");
			}
			else
			{
				NMC_reply(MSG_ETHRec,inet_ntoa(sin.sin_addr),IPS_SERVER_PORT,NMC_reply_code_flase_start);
				DBGMSG(DPINFO, "open_v_up:NMC_reply_code_flase_start\r\n");
			}
			break;

		case open_va_up_monitor://监视
			if(handshack_value==closeall)
			{
				m_MedeiaLApp.NmcNumNow=MSG_ETHRec.nitcphead.fromaddr;//static_cast<BYTE>
				m_MedeiaLApp.NmcIpNow=inet_addr(inet_ntoa(sin.sin_addr));
				//m_MedeiaLApp.OpenvaUpMonitorMedia();//-------------------------------->>>监视自动切换逻辑后加
				OpenvaUpMonitor=1;
				handshack_value=open_va_up_monitor;
				NMC_reply(MSG_ETHRec,inet_ntoa(sin.sin_addr),IPS_SERVER_PORT,NMC_reply_code_ture_start);
				Clock4Secs=Clock_En;
				Clock5TenMSecs=ClockCleared;
				DBGMSG(DPINFO, "handshack open_va_up_monitor OK\r\n");
				memcpy(&MSG_NMCHandshack,&MSG_ETHRec,sizeof(Network_Ipswich_TCP));
			}
			else if ((handshack_value==open_va_up_monitor)&&(MSG_ETHRec.nitcphead.fromaddr==MSG_NMCHandshack.nitcphead.fromaddr)&&(MSG_ETHRec.nitcphead.toaddr==MSG_NMCHandshack.nitcphead.toaddr))
			{
				NMC_reply(MSG_ETHRec,inet_ntoa(sin.sin_addr),IPS_SERVER_PORT,NMC_reply_code_ture_start);
				Clock4Secs=Clock_En;
				Clock5TenMSecs=ClockCleared;
				DBGMSG(DPINFO, "handshack open_va_up_monitor OK again\r\n");
			}
			else
			{
				NMC_reply(MSG_ETHRec,inet_ntoa(sin.sin_addr),IPS_SERVER_PORT,NMC_reply_code_flase_start);
				DBGMSG(DPINFO, "open_va_up_monitor:NMC_reply_code_flase_start\r\n");
			}
			break;
		case open_va_up_call://呼叫
			if(handshack_value==closeall)
			{
				m_MedeiaLApp.NmcNumNow=MSG_ETHRec.nitcphead.fromaddr;//static_cast<BYTE>
				m_MedeiaLApp.NmcIpNow=inet_addr(inet_ntoa(sin.sin_addr));
				m_MedeiaLApp.OpenvaUpCallMedia();
				handshack_value=open_va_up_call;
				NMC_reply(MSG_ETHRec,inet_ntoa(sin.sin_addr),IPS_SERVER_PORT,NMC_reply_code_ture_start);
				Clock4Secs=Clock_En;
				Clock5TenMSecs=ClockCleared;
				DBGMSG(DPINFO, "handshack open_va_up_call OK\r\n");
				memcpy(&MSG_NMCHandshack,&MSG_ETHRec,sizeof(Network_Ipswich_TCP));
			}
			else if((handshack_value==open_v_up)||(handshack_value==open_va_up_monitor))
			{	
				SendDIg_handshack(m_MedeiaLApp.NmcNumNow,m_MedeiaLApp.NmcIpNow,closeall);//关闭现在优先级小的呼叫操作
				DBGMSG(DPINFO, "---->((handshack_value==open_v_up)||(handshack_value==open_va_up_monitor))\r\n");
				m_MedeiaLApp.CloseMedia();
				HandTimeDef();
				DPPostMessage(MSG_PHONECALL, MSG_CALL_HUNGUP, 0, 0);
				InterruptMonitorAddr=m_MedeiaLApp.NmcNumNow;

				//挂断当前三次重发机制？****直接自身释放，无需三次重发***************************
				m_MedeiaLApp.NmcNumNow=MSG_ETHRec.nitcphead.fromaddr;
				m_MedeiaLApp.NmcIpNow=inet_addr(inet_ntoa(sin.sin_addr));
				m_MedeiaLApp.OpenvaUpCallMedia();
				handshack_value=open_va_up_call;
				NMC_reply(MSG_ETHRec,inet_ntoa(sin.sin_addr),IPS_SERVER_PORT,NMC_reply_code_ture_start);
				Clock4Secs=Clock_En;
				Clock5TenMSecs=ClockCleared;
				DBGMSG(DPINFO, "handshack open_va_up_call OK\r\n");
				memcpy(&MSG_NMCHandshack,&MSG_ETHRec,sizeof(Network_Ipswich_TCP));
			}
			else if ((handshack_value==open_va_up_call)&&(MSG_ETHRec.nitcphead.fromaddr==MSG_NMCHandshack.nitcphead.fromaddr)&&(MSG_ETHRec.nitcphead.toaddr==MSG_NMCHandshack.nitcphead.toaddr))
			{
				NMC_reply(MSG_ETHRec,inet_ntoa(sin.sin_addr),IPS_SERVER_PORT,NMC_reply_code_ture_start);
				Clock4Secs=Clock_En;
				Clock5TenMSecs=ClockCleared;
				DBGMSG(DPINFO, "handshack open_va_up_call OK again\r\n");
			}
			else
			{
				NMC_reply(MSG_ETHRec,inet_ntoa(sin.sin_addr),IPS_SERVER_PORT,NMC_reply_code_flase_start);
				DBGMSG(DPINFO, "NMC_reply_code_flase_start\r\n");
			}
			break;
		case open_va_down_call://呼叫
			if(handshack_value==closeall)
			{
				m_MedeiaLApp.NmcNumNow=MSG_ETHRec.nitcphead.fromaddr;
				m_MedeiaLApp.NmcIpNow=inet_addr(inet_ntoa(sin.sin_addr));
				m_MedeiaLApp.OpenvaDownCallMedia();
				handshack_value=open_va_down_call;
				NMC_reply(MSG_ETHRec,inet_ntoa(sin.sin_addr),IPS_SERVER_PORT,NMC_reply_code_ture_start);
				Clock4Secs=Clock_En;
				Clock5TenMSecs=ClockCleared;
				DBGMSG(DPINFO, "handshack open_va_down OK\r\n");
				memcpy(&MSG_NMCHandshack,&MSG_ETHRec,sizeof(Network_Ipswich_TCP));
			}
			else if((handshack_value==open_v_up)||(handshack_value==open_va_up_monitor))
			{
				SendDIg_handshack(m_MedeiaLApp.NmcNumNow,m_MedeiaLApp.NmcIpNow,closeall);
				DBGMSG(DPINFO, "closeall---->(handshack_value==open_v_up)||(handshack_value==open_va_up_monitor)\r\n");
				m_MedeiaLApp.CloseMedia();
				HandTimeDef();
				InterruptMonitorAddr=m_MedeiaLApp.NmcNumNow;
				DPPostMessage(MSG_PHONECALL, MSG_CALL_HUNGUP, 0, 0);
				//挂断当前三次重发机制？****直接自身释放，无需三次重发***************************
				m_MedeiaLApp.NmcNumNow=MSG_ETHRec.nitcphead.fromaddr;//static_cast<BYTE>
				m_MedeiaLApp.NmcIpNow=inet_addr(inet_ntoa(sin.sin_addr));
				m_MedeiaLApp.OpenvaDownCallMedia();
				handshack_value=open_va_down_call;
				NMC_reply(MSG_ETHRec,inet_ntoa(sin.sin_addr),IPS_SERVER_PORT,NMC_reply_code_ture_start);
				Clock4Secs=Clock_En;//使能心跳4S超时计时器
				Clock5TenMSecs=ClockCleared;
				DBGMSG(DPINFO, "handshack open_va_down OK\r\n");
				memcpy(&MSG_NMCHandshack,&MSG_ETHRec,sizeof(Network_Ipswich_TCP));
			}
			else if ((handshack_value==open_va_down_call)&&(MSG_ETHRec.nitcphead.fromaddr==MSG_NMCHandshack.nitcphead.fromaddr)&&(MSG_ETHRec.nitcphead.toaddr==MSG_NMCHandshack.nitcphead.toaddr))
			{
				NMC_reply(MSG_ETHRec,inet_ntoa(sin.sin_addr),IPS_SERVER_PORT,NMC_reply_code_ture_start);
				Clock4Secs=Clock_En;
				Clock5TenMSecs=ClockCleared;
				DBGMSG(DPINFO, "handshack open_va_down_call OK again\r\n");
			}
			else
			{
				NMC_reply(MSG_ETHRec,inet_ntoa(sin.sin_addr),IPS_SERVER_PORT,NMC_reply_code_flase_start);
				DBGMSG(DPINFO, "open_va_down:NMC_reply_code_flase_start\r\n");
			}
			break;
		case NMC_Heartbeat://NMC在线心跳
			if(MSG_ETHRec.nitcphead.fromaddr==m_MedeiaLApp.NmcNumNow)
			{
				Clock5TenMSecs=ClockCleared;
				Clock4Secs=Clock_En;//心跳标志启动复位，从1开始计数到5，防止4s超时主动挂断操作
				NMC_reply(MSG_ETHRec,inet_ntoa(sin.sin_addr),IPS_SERVER_PORT,NMC_reply_code_heartbeat);
				DBGMSG(DPINFO, "NMC_Heartbeat OK\r\n");
			}
			else
			{//双心跳在线状态，发送停止心跳挂断包
				SendDIg_handshack(MSG_ETHRec.nitcphead.fromaddr,inet_addr(inet_ntoa(sin.sin_addr)),closeall);
				m_MedeiaLApp.CloseMedia();
				HandTimeDef();
				DPPostMessage(MSG_PHONECALL, MSG_CALL_HUNGUP, 0, 0);
				DBGMSG(DPINFO, "NMC_double_Heartbeat SendDIg_handshack closeall\r\n");
			}
			break;
		case open_v_down:
			break;
		case add_open_audio:
			if((handshack_value==open_va_down_call)||(handshack_value==open_va_up_call))
			{
				m_MedeiaLApp.AddOpenAudioMedia();
				DBGMSG(DPINFO, "AddOpenAudioMedia\r\n");
				NMC_reply(MSG_ETHRec,inet_ntoa(sin.sin_addr),IPS_SERVER_PORT,NMC_reply_code_ture_audioOpen);
				//memcpy(&MSG_NMCHandshack,&MSG_ETHRec,sizeof(Network_Ipswich_TCP));
			}
			else
			{
				NMC_reply(MSG_ETHRec,inet_ntoa(sin.sin_addr),IPS_SERVER_PORT,NMC_reply_code_flase_audioOpen);
				DBGMSG(DPINFO, "add_open_audio:NMC_reply_code_flase_start\r\n");
			}
			break;
		case RebootIPS:
			DBGMSG(DPINFO, "RebootIPS!\r\n");
			//::DPPostMessage(MSG_SYSTEM, REBOOT_MACH, 0, 0);//NMC Session Err
			RebootSystem();
			//DBGMSG(DPINFO, "RebootIPS!\r\n");
			break;
		default:
			NMC_reply(MSG_ETHRec,inet_ntoa(sin.sin_addr),IPS_SERVER_PORT,NMC_reply_code_flase_start);
			DBGMSG(DPINFO, "wrong handshack_value\r\n");
			break;
	}
}
void D0RCODET_Handshak(Network_Ipswich_TCP MSG_ETHRec,sockaddr_in sin)
{
	if (MSG_ETHRec.nitcphead.fromaddr>0x0f)
	{
		if(TalkEventSaveBuf.dIP==inet_addr(inet_ntoa(sin.sin_addr)))
		{
			switch (MSG_ETHRec.content[EthData_lenthB+EthComSess])
			{
				case NMC_reply_code_ture_start:
					HeartBeatTmp=TRUE;
					Clock3Secs=Clock_En;
					Clock50MSecs=ClockCleared;

					if (SixSecindex==FALSE)
					{					
						//20150616  大门呼叫下属设备、下属设备不在线且大门断开不发送3a状况解决、第一次握手线路空闲启动38标志事件
						Clock6Secs=Clock_En;
						Clock60MSecs=ClockCleared;
						SixSecindex=TRUE;
					}

					BufCan_list.AddTail(BufCanWaitSave);
					SetEvent(ETHTransThread_event);
					break;
				case NMC_reply_code_flase_start:
					CanRSendBufCanPAK(BufCanWaitSave,RCODET_Call_BUSY);
					handshack_value=closeall;
					VideoLevealSwitch=0;
					VideoLenthLeveal=0;
					ZeroMemory(&TalkEventSaveBuf,sizeof(TalkEventSaveBuf));
					break;
				case NMC_reply_code_ture_end:
				case NMC_reply_code_flase_end:
				case NMC_reply_code_heartbeat:
				case NMC_reply_code_ture_audioOpen:
				case NMC_reply_code_flase_audioOpen:
					break;
			}
		}
		else
			printf("TalkEventSaveBuf.dIP!=inet_addr(inet_ntoa(sin.sin_addr))\r\n");		
	}
};
DWORD D0NMC_WriteAddrMap(Network_AddrTCP MSG_ETHRec)
{//LPCTSTR lpAppName,LPCTSTR lpKeyName,LPCTSTR lpString,LPCTSTR lpFileName
//CeWritePrivateProfileString(_T("section"), _T("key"), _T("value"), _T("\\UserDev\\test.ini"));

	CFile  mFile;
	CFileStatus status;
	//CFileFind finder;!wince
	BOOL bfinding = mFile.GetStatus(L"\\UserDev\\SysSet.ini",status);
	if (bfinding)
	{
		mFile.Remove(L"\\UserDev\\SysSet.ini");
	}
	if (!mFile.Open(L"\\UserDev\\SysSet.ini",CFile::modeCreate|CFile::modeReadWrite))//创建失败
	{
		return FALSE;
	}
	mFile.Close();

	//FILE *stream = t_fopen(_T("\\UserDev\\SysSet.ini"), _T("w+"));//无法正常再次写入?
	//if(stream == NULL)
	//	return FALSE;
	//fclose(stream);

	DWORD TMP,TMP_unit,bulidingLen,unitLen,IPLen;
	LPTSTR lpAppName,lpKeyName,lpAddrString;
	TCHAR *LeftPChar,*RightPChar,*pEqualChar,*IPChar,*EndChar,*OldUnitEndChar;
	DWORD DataLen=t_strtol(&MSG_ETHRec.nitcphead.DATALenth[oneLen],NULL, 10);
	for(TMP=0;TMP<DataLen;TMP++)
	{
		LeftPChar=t_strchr(&MSG_ETHRec.Addrcontent[TMP],L'[');
		EndChar=t_strchr(&MSG_ETHRec.Addrcontent[TMP],L'.');
		RightPChar=t_strchr(&MSG_ETHRec.Addrcontent[TMP],L']');
		if ((RightPChar==NULL)||(LeftPChar==NULL))
			return DataERR;
		bulidingLen=RightPChar-LeftPChar-1;
		if (bulidingLen>buildingBitNoMax)
			return DataERR;
		lpAppName=(WCHAR*)malloc((bulidingLen+1)*2);//lpAppName
		ZeroMemory(lpAppName,(bulidingLen+1)*2);
		t_strncpy(lpAppName,&MSG_ETHRec.Addrcontent[TMP+1],bulidingLen);
		pEqualChar=t_strchr(&MSG_ETHRec.Addrcontent[TMP],L'=');
		if (pEqualChar==NULL)
			return DataERR;
		unitLen=pEqualChar-RightPChar-1;
		if (unitLen>unitBitNoMax)
			return DataERR;
		lpKeyName=(WCHAR*)malloc((unitLen+1)*2);
		ZeroMemory(lpKeyName,(unitLen+1)*2);
		t_strncpy(lpKeyName,&MSG_ETHRec.Addrcontent[TMP+bulidingLen+BStartPNo],unitLen);//lpKeyName
		IPChar=t_strchr(&MSG_ETHRec.Addrcontent[TMP],L';');
		if (IPChar==NULL)
			return DataERR;
		IPLen=IPChar-pEqualChar-1;
		if (IPLen>IPBitNoMax)
			return DataERR;
		lpAddrString=(WCHAR*)malloc((IPLen+1)*2);
		ZeroMemory(lpAddrString,(IPLen+1)*2);
		t_strncpy(lpAddrString,&MSG_ETHRec.Addrcontent[TMP+bulidingLen+unitLen+BStartPNo+BEqualPNo],IPLen);//lpAddrString
		if(!(CeWritePrivateProfileString(lpAppName, lpKeyName, lpAddrString, _T("\\UserDev\\SysSet.ini"))))
			return FALSE;
		free(lpKeyName);
		free(lpAddrString);
		TMP=TMP+(IPChar-LeftPChar)+1;
		for (TMP_unit=0;TMP_unit<(unitNoMax-1);TMP_unit++)
		{
			if(MSG_ETHRec.Addrcontent[TMP]=='.')
				break;
			OldUnitEndChar=t_strchr(&MSG_ETHRec.Addrcontent[TMP-1],L';');
			pEqualChar=t_strchr(&MSG_ETHRec.Addrcontent[TMP],L'=');
			if (pEqualChar==NULL)
				return DataERR;
			unitLen=pEqualChar-OldUnitEndChar-1;
			if (unitLen>unitBitNoMax)
				return DataERR;
			lpKeyName=(WCHAR*)malloc((unitLen+1)*2);
			ZeroMemory(lpKeyName,(unitLen+1)*2);
			t_strncpy(lpKeyName,&MSG_ETHRec.Addrcontent[TMP],unitLen);//lpKeyName
			IPChar=t_strchr(&MSG_ETHRec.Addrcontent[TMP+unitLen],L';');
			if (IPChar==NULL)
				return DataERR;
			IPLen=IPChar-pEqualChar-1;
			if (IPLen>IPBitNoMax)
				return DataERR;
			lpAddrString=(WCHAR*)malloc((IPLen+1)*2);
			ZeroMemory(lpAddrString,(IPLen+1)*2);
			t_strncpy(lpAddrString,&MSG_ETHRec.Addrcontent[TMP+unitLen+BEqualPNo],IPLen);//lpAddrString
			if(!(CeWritePrivateProfileString(lpAppName, lpKeyName, lpAddrString, _T("\\UserDev\\SysSet.ini"))))
				return FALSE;
			free(lpAppName);
			free(lpKeyName);
			free(lpAddrString);
			TMP=TMP+(IPChar-OldUnitEndChar);		
		}
	}
	return TRUE;
}


LPSTR UnicodeToAnsi(LPCTSTR szStr)  
{  
	int nLen = WideCharToMultiByte( CP_ACP, 0, szStr, -1, NULL, 0, NULL, NULL );  
	if (nLen == 0)  
	{  
		return NULL;  
	}  
	char* pResult = new char[nLen];  
	WideCharToMultiByte( CP_ACP, 0, szStr, -1, pResult, nLen, NULL, NULL );  
	return pResult;  
}
DWORD IPSSendAddrMapReply(LPCTSTR Pstring,DWORD ip,DWORD port)
{
	SOCKET sockClient;
	int nLen = wcslen(Pstring);  
	//if (nLen == 0)  
	//{  
	//	return FALSE;  
	//}  
	//LPSTR pResultChar = new char[nLen];  
	//WideCharToMultiByte( CP_ACP, 0, Pstring, -1, pResultChar, nLen, NULL, NULL );  
	////LPSTR pResult=pResultChar; 
	//if (pResultChar)
	if (nLen)
	{
		sockClient=TcpConnect(ip,port,CONNECT_TIMEOUT);
		if(sockClient == INVALID_SOCKET)
		{
			DBGMSG(DPINFO, "IPSSendAddrMapReply Connect %s:%d fail\r\n",ip,port);
			SocketClose(sockClient);
		}
		int ret=TcpSendData(sockClient,(char *)Pstring,nLen*2,SEND_TIMEOUT);
		if(!ret)
		{
			DBGMSG(DPWARNING, "IPSSendAddrMapReply Msg Failed\r\n"); 
			SocketClose(sockClient);
			return FALSE;
		}
		SocketClose(sockClient);
		return TRUE;
	}
	else
		return FALSE;
}

void TalkEventCommandPro(Network_Ipswich_TCP MSG_ETHRec)
{
	if(MSG_ETHRec.nitcphead.headcommand==RCODET_Call)
	{
		//if(TalkEventSaveBuf.dIP==inet_addr(inet_ntoa(sin.sin_addr)))
		if((TalkEventSaveBuf.dAddr==MSG_ETHRec.nitcphead.fromaddr)&&((TalkEventSaveBuf.sAddr==MSG_ETHRec.nitcphead.toaddr)))
		{
			//20150616 收到b8，释放38标志事件
			Clock60MSecs=ClockCleared;
			Clock6Secs=Clock_Dis;

			switch (MSG_ETHRec.content[EthData_lenthB+EthComSess])
			{
				case RCODET_Call_BUSY:
					HeartBeatTmp=FALSE;
					Clock50MSecs=ClockCleared;
					Clock3Secs=Clock_Dis;
					SendDIg_handshack(TalkEventSaveBuf.dAddr,TalkEventSaveBuf.dIP,closeall);
					DBGMSG(DPINFO, "closeall---->RCODET_Call_BUSY\r\n");
					handshack_value=closeall;
					VideoLevealSwitch=0;
					VideoLenthLeveal=0;
					//SixSecindex=FALSE;
					ZeroMemory(&TalkEventSaveBuf,sizeof(TalkEventSaveBuf));
					break;
				case RCODET_Call_RING:
					if (Mediastatus==FALSE)
					{
						if (handshack_value==open_va_down_call)
						{
							m_MedeiaLApp.OpenvaDownCallMedia();
						} 
						else if(handshack_value==open_va_up_call)
						{
							m_MedeiaLApp.OpenvaUpCallMedia();
						}

						//DPPostMessage(MSG_PHONECALL, MSG_NEW_CALLOUT, 0, 0);
						DBGMSG(DPINFO,"MSG_PHONECALL----------------------------->>DelayPhoneCallIps\r\n");
						ClockDelayTCallIps=Clock_En;

					}
					else
						printf("Mediastatus=TRUE,But 0xb8 is on line");

			}
		}

	}
	else if(MSG_ETHRec.nitcphead.headcommand==CODET_HangUp)
	{
		if((TalkEventSaveBuf.dAddr==MSG_ETHRec.nitcphead.fromaddr)&&((TalkEventSaveBuf.sAddr==MSG_ETHRec.nitcphead.toaddr)))
		{
			if((handshack_value==open_va_down_call)||(handshack_value==open_va_up_call))
			{
				m_MedeiaLApp.AddOpenAudioMedia();
				DBGMSG(DPINFO, "AddOpenAudioMedia\r\n");
				SendDIg_handshack(TalkEventSaveBuf.dAddr,TalkEventSaveBuf.dIP,add_open_audio);
			}
			else if(handshack_value==closeall)
			{
				CanRSendTCPPAK(MSG_ETHRec,RCODET_HangUp_BUSY);
				HeartBeatTmp=FALSE;
				Clock50MSecs=ClockCleared;
				Clock3Secs=Clock_Dis;
				ZeroMemory(&TalkEventSaveBuf,sizeof(TalkEventSaveBuf));
				DBGMSG(DPINFO, "handshack_value==closeall----closeall-->Worng CODET_HangUp!!\r\n");
			}
			else
			{
				CanRSendTCPPAK(MSG_ETHRec,RCODET_HangUp_BUSY);
				HeartBeatTmp=FALSE;
				Clock50MSecs=ClockCleared;
				Clock3Secs=Clock_Dis;
				SendDIg_handshack(TalkEventSaveBuf.dAddr,TalkEventSaveBuf.dIP,closeall);
				DBGMSG(DPINFO, "else--closeall--> Worng CODET_HangUp\r\n");
				handshack_value=closeall;
				VideoLevealSwitch=0;
				VideoLenthLeveal=0;
				ZeroMemory(&TalkEventSaveBuf,sizeof(TalkEventSaveBuf));
				DBGMSG(DPINFO, "Worng handshack_value!!but 0xb9 is on line\r\n");
			}
		}	
	}
	else if (MSG_ETHRec.nitcphead.headcommand==END_Application)
	{
		//if(TalkEventSaveBuf.dIP==inet_addr(inet_ntoa(sin.sin_addr)))
		if((TalkEventSaveBuf.dAddr==MSG_ETHRec.nitcphead.fromaddr)&&((TalkEventSaveBuf.sAddr==MSG_ETHRec.nitcphead.toaddr)))
		{
			HeartBeatTmp=FALSE;
			Clock50MSecs=ClockCleared;
			Clock3Secs=Clock_Dis;
			SendDIg_handshack(TalkEventSaveBuf.dAddr,TalkEventSaveBuf.dIP,closeall);
			DBGMSG(DPINFO, "closeall-->END_Application\r\n");
			m_MedeiaLApp.CloseMedia();
			HandTimeDef();
			DPPostMessage(MSG_PHONECALL, MSG_CALL_HUNGUP, 0, 0);
			ZeroMemory(&TalkEventSaveBuf,sizeof(TalkEventSaveBuf));
			DBGMSG(DPINFO, "END_Application OK\r\n");
		}
	}
}
void ForwordEthPak(Network_Ipswich_TCP MSG_ETHRec)
{
	BufCan EthSendPak;
	DWORD ip_data_len=MSG_ETHRec.nitcphead.length - ip_head_len;
	EthSendPak.dAddr=MSG_ETHRec.nitcphead.toaddr;
	EthSendPak.sAddr=MSG_ETHRec.nitcphead.fromaddr;			
	EthSendPak.CanLen=static_cast<BYTE>(ip_data_len-EthData_lenthB);
	if (EthSendPak.sAddr<0xff)
		EthSendPak.bSelAddr=SelAddr_s8_d32;
	else
		EthSendPak.bSelAddr=SelAddr_s32_d8;
	
	//if(MSG_ETHRec.nitcphead.headcommand==CODET_Alarm)
		EthSendPak.bPri=High_pri;
	//else
	//	EthSendPak.bPri=Low_pri;
	memcpy(static_cast<BYTE*>(&EthSendPak.InfoCan[0]),static_cast<BYTE*>(&MSG_ETHRec.content[EthData_lenthB]),EthSendPak.CanLen);
	if(MTransDataCan((BufCan*)(&EthSendPak)))//1：发送失败
		DBGMSG(DPINFO, "MTransDataCan((BufCan*)(&EthSendPak)) fail\r\n");
	if(TxdbufCanNum!=0)
	{
		SetEvent(Can_RunWriteThread_event);
	}
}

static BOOL AddMapexsistSearch()
{
	CFile  mFile;
	CFileStatus status;
	BOOL bfinding = mFile.GetStatus(L"\\UserDev\\SysSet.ini",status);
	if (bfinding)
		return FALSE;
	else
		return TRUE;
}

static DWORD ETHRecThread(HANDLE lparam)
{
	DWORD ip_data_len=0;
	sockaddr_in sin,My_sin; 
	SOCKET sockServer;
	SOCKET sockAccept;
	Network_Ipswich_TCP MSG_ETHRec;
	ZeroMemory(&MSG_ETHRec,sizeof(MSG_ETHRec));
	ZeroMemory(&MSG_NMCHandshack,sizeof(MSG_NMCHandshack));
	My_sin.sin_addr.S_un.S_addr=My_ip;
	BOOL NMC_TIMER__tmp=FALSE;

	while(TRUE)
	{
		//sockServer = TcpListen("192.168.1.5" , IPS_SERVER_PORT);
		sockServer = TcpListen(inet_ntoa(My_sin.sin_addr) , IPS_SERVER_PORT_REC);
		if(sockServer==INVALID_SOCKET)
		{
			DBGMSG(DPINFO, "ETHRecThread Listen client fail\r\n");
			continue;
		}
		else
			break;
	}
	ZeroMemory(&MSG_ETHRec,sizeof(MSG_ETHRec));
	while(TRUE)//后续加上退出线程的完善代码
	{
		SetEvent(ThreadRunEvent[MSG_EVENT_ETHREC]);	
		//DBGMSG(DPINFO, "MSG_EVENT_ETHREC\r\n");
		sockAccept = TcpAccpet(sockServer, REC_TIMEOUT, &sin);		
		if(sockAccept==INVALID_SOCKET)
			continue;	
		if( !TcpRecvData(sockAccept,(char *)(&MSG_ETHRec.nitcphead.length), ip_head_len, REC_TIMEOUT))
		{
			DBGMSG(DPINFO, "ETHRecThread Recv head client fail\r\n");
			SocketClose(sockAccept);
			continue;
		}
		ip_data_len = MSG_ETHRec.nitcphead.length - ip_head_len;
		//MSG_ETHRec.content= (BYTE*)malloc(ip_data_len);
		if(!TcpRecvData(sockAccept,(char *)(&MSG_ETHRec.content),ip_data_len,2000))//static_cast<BYTE*>
		{
			DBGMSG(DPINFO, "ETHRecThread Recv pbuf client fail\r\n");
			SocketClose(sockAccept);
			continue;
		}
		printf("------------------EthRecPAK------------------\r\n");
		DBGTime();
		printf("ETHaccept command is 0x%x,value is 0x%x,Csession is 0x%x\r\n",  MSG_ETHRec.nitcphead.headcommand,MSG_ETHRec.content[EthData_lenthB+EthComSess],MSG_ETHRec.nitcphead.headmsgsession);	
		//refresh ip-addr table
		SocketClose(sockAccept);
		IF_tableMap(MSG_ETHRec,sin);
		//refresh NMC ip
		if(MSG_ETHRec.nitcphead.headcommand==CODET_NMC_time)
		{
			NMC_ip=inet_ntoa(sin.sin_addr);//主动上报事件主网管ip地址
			if (NMC_TIMER__tmp==FALSE)
			{
				NMC_TIMER__tmp=TRUE;
				SetNMCTimeToSystemTime(MSG_ETHRec);
				//if (AddMapexsistSearch())//判断地址表是否存在
				IPSSendAddrMapReply(_T("<AddrMap><ERRDATA>"),inet_addr(inet_ntoa(sin.sin_addr)),IPS_SERVER_PORT_REC);
			}	
		}
		//handshack judge
		if(MSG_ETHRec.nitcphead.headcommand==CODET_Handshake)
		{/*head+content(length（数据包长度)+command+session+value)*/
			D0CODET_Handshake(MSG_ETHRec,sin);
			//NHandshake_list.AddTail(MSG_ETHRec);//握手包链表，挂断当前通话删除此node
		}
		else if(MSG_ETHRec.nitcphead.headcommand==RCODET_Handshak)
		{
			//D0RCODET_Handshak(MSG_ETHRec,sin);
			D0RCODET_Handshak(MSG_ETHRec,sin);
		}
		else//需转发的数据包
		{
			if ((MSG_ETHRec.nitcphead.fromaddr>0x0f)&&((TalkEventSaveBuf.sAddr)==MSG_ETHRec.nitcphead.toaddr))
			{
				TalkEventCommandPro(MSG_ETHRec);
			}
			ForwordEthPak(MSG_ETHRec);
		}
		//SocketClose(sockAccept);
	}
	return TRUE;
}

static DWORD ETHRecAddrMapThread(HANDLE lparam)
{
	DWORD ip_data_len=0;
	sockaddr_in sin,My_sin;
	SOCKET sockServer;
	SOCKET sockAccept;
	Network_AddrTCP MSG_ETHRec;
	ZeroMemory(&MSG_ETHRec,sizeof(MSG_ETHRec));
	My_sin.sin_addr.S_un.S_addr=My_ip;
	while(TRUE)
	{
		sockServer = TcpListen(inet_ntoa(My_sin.sin_addr) , IPS_SERVER_PORT_RECAddr);
		if(sockServer==INVALID_SOCKET)
		{
			DBGMSG(DPINFO, "ETHRecAddrMapThread Listen client fail\r\n");
			continue;
		}
		else
			break;
	}
	ZeroMemory(&MSG_ETHRec,sizeof(MSG_ETHRec));
	while(TRUE)
	{
		SetEvent(ThreadRunEvent[MSG_EVENT_ETHMAP]);	
		//DBGMSG(DPINFO, "MSG_EVENT_ETHMAP\r\n");

		sockAccept = TcpAccpet(sockServer, REC_TIMEOUT, &sin);		
		if(sockAccept==INVALID_SOCKET)
			continue;	
		if( !TcpRecvData(sockAccept,(char *)(&MSG_ETHRec.nitcphead.START_Type), ETHMapHeadLenth, REC_TIMEOUT))
		{
			DBGMSG(DPINFO, "ETHRecAddrMapThread Recv head client fail!\r\n");
			SocketClose(sockAccept);
			IPSSendAddrMapReply(_T("<AddrMap><ERRDATA>"),inet_addr(inet_ntoa(sin.sin_addr)),IPS_SERVER_PORT_REC);
			continue;
		}

		if (t_strncmp(MSG_ETHRec.nitcphead.START_Type,_T("<AddrMap>"),ETHMapHeadTypeLenth)!=EQCharTrue)
		{
			DBGMSG(DPINFO, "ETHRecAddrMapThread Recv head EQCharFase!\r\n");
			SocketClose(sockAccept);
			IPSSendAddrMapReply(_T("<AddrMap><ERRDATA>"),inet_addr(inet_ntoa(sin.sin_addr)),IPS_SERVER_PORT_REC);
			continue;
		}
		ip_data_len=t_strtol(MSG_ETHRec.nitcphead.DATALenth+1, NULL, 10);
		if(!TcpRecvData(sockAccept,(char *)(&MSG_ETHRec.Addrcontent),ip_data_len*2,REC_TIMEOUT))//static_cast<BYTE*>
		{
			DBGMSG(DPINFO, "ETHRecThread Recv Addrcontent client fail\r\n");
			SocketClose(sockAccept);
			IPSSendAddrMapReply(_T("<AddrMap><ERRDATA>"),inet_addr(inet_ntoa(sin.sin_addr)),IPS_SERVER_PORT_REC);
			continue;
		}
		DBGMSG(DPINFO,"------------------EthRecMapPAK------------------\r\n");
		DBGTime();
		//refresh ip-addr table
		SocketClose(sockAccept);
		DWORD NMC_WriteAddr_ret=D0NMC_WriteAddrMap(MSG_ETHRec);
		if (NMC_WriteAddr_ret==TRUE)
		{
			IPSSendAddrMapReply(_T("<AddrMap><OK>"),inet_addr(inet_ntoa(sin.sin_addr)),IPS_SERVER_PORT_REC);
			DBGMSG(DPINFO,"D0NMC_WriteAddrMap ok\r\n");
		}
		else if(NMC_WriteAddr_ret==DataERR)
		{
			IPSSendAddrMapReply(_T("<AddrMap><ERRDATA>"),inet_addr(inet_ntoa(sin.sin_addr)),IPS_SERVER_PORT_REC);
			DBGMSG(DPINFO,"D0NMC_WriteAddrMap ERRDATA!\r\n");
		}
		else
		{
			IPSSendAddrMapReply(_T("<AddrMap><ERRINI>"),inet_addr(inet_ntoa(sin.sin_addr)),IPS_SERVER_PORT_REC);
			DBGMSG(DPINFO,"D0NMC_WriteAddrMap ERRINI!\r\n");
		}	
	}
	return TRUE;
}

extern CAtlList<BufCan> BufCan_list;
static DWORD ETHTransThread(HANDLE lparam)
{
	//BufCan Tmpcbuf;
	DWORD ip_data_len=0;
	DWORD  send_ip=0;
	Network_Ipswich_TCP NetTCPPak;
	struct in_addr addr;
	POSITION Tmpcbufpos;
	POSITION pos;
	BYTE TMP_i=0,TMP_j=0,TMP_num=0;
	ZeroMemory(&NetTCPPak,sizeof(NetTCPPak));
	while(TRUE)
	{
		SetEvent(ThreadRunEvent[MSG_EVENT_ETHTRANC]);
		//DBGMSG(DPINFO, "MSG_EVENT_ETHTRANC\r\n");
		WaitForSingleObject(ETHTransThread_event,2000);
		{   
			if((RcvResult==CAN_RCV_OK)||(RcvResult==CAN_RCV_OK1))
			{
				TMP_num=BufCan_list.GetCount();
				Tmpcbufpos=BufCan_list.GetHeadPosition();
				for (TMP_i=0;TMP_i<TMP_num;TMP_i++)
				{
					NetTCPPak.nitcphead.fromaddr=BufCan_list.GetAt(Tmpcbufpos).sAddr;
					NetTCPPak.nitcphead.toaddr=BufCan_list.GetAt(Tmpcbufpos).dAddr;
					NetTCPPak.nitcphead.headcommand=BufCan_list.GetAt(Tmpcbufpos).InfoCan[0];//commmand
					NetTCPPak.nitcphead.headmsgsession=BufCan_list.GetAt(Tmpcbufpos).InfoCan[PointerAddrOne];//session
					NetTCPPak.nitcphead.length=BufCan_list.GetAt(Tmpcbufpos).CanLen+sizeof(Network_Ipswich_TCP_HEAD)+EthData_lenthB;//总长
					ip_data_len= NetTCPPak.nitcphead.length - ip_head_len;
					//NetTCPPak.content= (BYTE*)malloc(ip_data_len);
					NetTCPPak.content[0]=(BYTE)ip_data_len;//content中总长
					memcpy(static_cast<BYTE*>(&NetTCPPak.content[EthData_lenthB]),static_cast<BYTE*>(&BufCan_list.GetAt(Tmpcbufpos).InfoCan[0]),BufCan_list.GetAt(Tmpcbufpos).CanLen);			
					pos=NetIF_list.GetHeadPosition();//根据地址映射list，网管号对应的ip地址
					for(BYTE TMP_j=0;TMP_j<NetIF_list.GetCount();TMP_j++)
					{
						if(NetIF_list.GetAt(pos).FromAddr==NetTCPPak.nitcphead.toaddr)//同一网管
						{
							send_ip=NetIF_list.GetAt(pos).FromIP;
							break;
						}
						else
							NetIF_list.GetNext(pos);

					}
					if(send_ip)
					{
						addr.S_un.S_addr=send_ip;
						ETHSend(NetTCPPak,inet_ntoa(addr),IPS_SERVER_PORT);									
						printf("---------------EthSendPAK-------------\r\n");
						DBGTime();
						printf("ETHTransPAK command is 0x%x,session is 0x%x,value is 0x%x\r\n",  NetTCPPak.nitcphead.headcommand,NetTCPPak.nitcphead.headmsgsession,NetTCPPak.content[EthData_lenthB+EthComSess]);
						send_ip=0;
					}
					else
						printf("NetTCPPak send_ip =0 !\r\n");
					BufCan_list.GetNext(Tmpcbufpos);
				}
				RcvResult=CAN_RCV_NO;
				TMP_num=0;
				BufCan_list.RemoveAll();
			}
			if ((HeartBeatTmp==TRUE)&&(handshack_value!=closeall))
			{
				HeartBeatTmp=FALSE;
				SendDIg_handshack(TalkEventSaveBuf.dAddr,TalkEventSaveBuf.dIP,NMC_Heartbeat);
				Clock50MSecs=ClockCleared;
				Clock3Secs=Clock_En;
				
			}

		}
	}
	return TRUE;
}

BOOL enable_ETH_RunThread()
{
	HANDLE hThread1,hThread2,hThread3;

	hThread1 =CreateThread(NULL, 0, ETHRecThread,0, 0, NULL);
	if(hThread1  == NULL)
		printf("CreateThread fail,error is %d",GetLastError());
	CeSetThreadPriority(hThread1,203);
	CloseHandle(hThread1);
	
	hThread2 = CreateThread(NULL, 0, ETHTransThread, NULL, 0, NULL);
	if(hThread2  == NULL)
		printf("CreateThread fail,error is %d",GetLastError());
	CeSetThreadPriority(hThread1,204);
	CloseHandle(hThread2);

	hThread3 = CreateThread(NULL, 0, ETHRecAddrMapThread, NULL, 0, NULL);
	if(hThread3  == NULL)
		printf("CreateThread fail,error is %d",GetLastError());
	CeSetThreadPriority(hThread1,200);
	CloseHandle(hThread3);
	return TRUE;
}