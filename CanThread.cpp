#include <RoomInf.h>
#include <Atlcoll.h>
#include "systemmsg.h"
#include  "CanThread.h"
#include"PilotLamp.h"
#include "Mytimer.h"
#include "SysMonitor.h"
#include "IniFile.h"
#pragma comment(lib, "ws2.lib")
  
//static BYTE BusCanStatus=S_CAN_IDLE;      //状态变量
//DWORD CLkCanStatus;
HANDLE hcan;
BufCan Tmpcbuf;
CAtlList<BufCan> BufCan_list;
BufCan BufCanWaitSave;
BYTE RcvResult;

BYTE TxdbufCanPrevi=0;

BYTE TxdbufCanNow=0;		//现在接收地址
BYTE TxdbufCanNum=0;		
CanTxMsg CanTxBuf[CAN_TXD_NBUF];

BYTE RcvbufCanPrevi=0;		//以前接收地址
BYTE RcvbufCanNow=0;		//现在接收地址
BYTE RcvbufCanNum=0;
CanRxMsg CanRxBuf[CAN_RXD_NBUF];

BufCan BigPak;			//大包缓存
CanTxMsg BigCanPak;
BYTE BigPakNum;		//大包值

TalkEventParam TalkEventSaveBuf;

extern DWORD Clock60MSecs;
extern void HandTimeDef();

//返回: 0:发送成功		1:发送失败
BYTE MTransDataCan(BufCan *p)//CAN总线的数据发送
{   
    BYTE cNumPAk;
    BYTE iLen;
    BYTE i;
    BYTE lCanData;
    BYTE iCanNow;
    CanTxMsg TxMessage;
    DWORD  CanID;
	ZeroMemory(&TxMessage,sizeof(TxMessage));

	if(p->CanLen>CAN_MAX_BYTE)
	{	p->CanLen=CAN_MAX_BYTE-1;}
	
	iLen=p->CanLen;
	if(iLen<=6)
	{   cNumPAk=1;}
	else
	{   lCanData=5;
		if((iLen%5)==0)
		{	cNumPAk=iLen/5;}
		else
		{	cNumPAk=iLen/5+1;}
	}
	if((CAN_TXD_NBUF-TxdbufCanNum)<(cNumPAk+2))
	{   return 1;}
	
	iCanNow=TxdbufCanNow;
	TxdbufCanPrevi=TxdbufCanNow;//////////////////////////////1551

	//printf("TxdbufCanNow is %d\r\n",TxdbufCanNow);
	for(i=cNumPAk;i>0;i--)
	{   
	    TxdbufCanNow++;
    	TxdbufCanNum++;
        if(TxdbufCanNow>=CAN_TXD_NBUF)
        {	TxdbufCanNow=0;}
    }
	//设置广播包标志位
    if((p->dAddr==0xffffffff)||(p->sAddr==0xffffffff))
    {   p->bCanBroadcast=1;}
    else
    {   p->bCanBroadcast=0;}
	//设置目的、源地址ExtId
    if(p->bSelAddr==0)
    {   TxMessage.ExtId=p->dAddr/256;
        //BigtoLittle32(TxMessage.ExtId);
        TxMessage.Data[0]=static_cast<BYTE>(p->dAddr&0x0ff);
        TxMessage.Data[1]=static_cast<BYTE>(p->sAddr&0x0ff);
    }
    else
    {   TxMessage.ExtId=static_cast<BYTE>(p->dAddr&0x0ff);
        TxMessage.ExtId=TxMessage.ExtId*0x10000+(p->sAddr/0x10000);
        //BigtoLittle32(TxMessage.ExtId);
        TxMessage.Data[0]=static_cast<BYTE>((p->sAddr&0x0ff00)/0x100);
        TxMessage.Data[1]=static_cast<BYTE>(p->sAddr&0x0ff);
        TxMessage.ExtId=TxMessage.ExtId|SEL_ADDR8_32;
    }
	//优先级设置ExtId
    if(p->bPri==0)
    {   TxMessage.ExtId=TxMessage.ExtId|BIT_LOW_PRIOR;}
	//广播包设置ExtId
    if(p->bCanBroadcast==1)
    {   TxMessage.ExtId=TxMessage.ExtId|BIT_BRDCAST;}
    
    TxMessage.IDE=CAN_ID_EXT;			        // 使用扩展标识符
    TxMessage.RTR=CAN_RTR_DATA;		            // 消息类型为数据帧，一帧8位-2000E默认数据帧，暂时无需设置2014-11-22

    if(cNumPAk==1)
    {   TxMessage.ExtId=TxMessage.ExtId|FIRST_CAN_PACK|END_CAN_PACK;
        memcpy(&(TxMessage.Data[2]),&(p->InfoCan[0]),p->CanLen);
        TxMessage.DLC=p->CanLen+2;
        TxMessage.StdId=TxMessage.ExtId;
        memcpy(&(CanTxBuf[iCanNow].StdId),&(TxMessage.StdId),sizeof(TxMessage));///////////////CanTxBuf[iCanNow]//fs/////////////////?
		//printf("iCanNow is %d\r\n",iCanNow);
		//DBGCanPak(CanTxBuf[iCanNow].ExtId,(BYTE *)&CanTxBuf[iCanNow].Data,CanTxBuf[iCanNow].DLC);
        return 0;
    }
    CanID=TxMessage.ExtId;
	for(i=cNumPAk;i>0;i--)
	{   
	    TxMessage.Data[2]=cNumPAk-i;
	    TxMessage.DLC=8;
	    if(i==cNumPAk)
	    {   TxMessage.ExtId=CanID|FIRST_CAN_PACK;
            memcpy(&(TxMessage.Data[3]),&(p->InfoCan[0]),lCanData);
	    }
	    else if(i==1)
	    {   TxMessage.ExtId=CanID|END_CAN_PACK;
	        memcpy(&(TxMessage.Data[3]),&(p->InfoCan[(cNumPAk-i)*lCanData]),p->CanLen-(cNumPAk-1)*lCanData);
            TxMessage.DLC=p->CanLen-(cNumPAk-1)*lCanData+3;
	    }
	    else
	    {   TxMessage.ExtId=CanID;
	        memcpy(&(TxMessage.Data[3]),&(p->InfoCan[(cNumPAk-i)*lCanData]),lCanData); 
	    }
	    TxMessage.StdId=TxMessage.ExtId;
	    memcpy(&(CanTxBuf[iCanNow].StdId),&(TxMessage.StdId),sizeof(TxMessage));/////////////////CanTxBuf[iCanNow]//fs/////////////////?
    	iCanNow++;
        if(iCanNow>=CAN_TXD_NBUF)
        {	iCanNow=0;}
		//printf("iCanNow is %d\r\n",iCanNow);
		//DBGCanPak(CanTxBuf[iCanNow].ExtId,(BYTE *)&CanTxBuf[iCanNow].Data,CanTxBuf[iCanNow].DLC);
    }
    return 0;
}

//返回: 0:接收成功(无缓存数据)		1:接收成功(有缓存数据) 2:无接收数据 3:错误数据
BYTE RcvDataFromCan(BufCan *p)//CAN总线接收数据
{   
    BYTE lCanData;
    CanRxMsg RxMessage;
	DWORD  cAddr;
	DWORD  CanID;
    BYTE iPakNum;
//	DWORD  BigToLit;
	
	if(RcvbufCanNum==0)//(RxdbufCanPrevi==RxdbufCanNow)
	{   return CAN_RCV_NO;} 
	//目的地址非自己的非呼叫包直接删除
    memcpy(&(RxMessage.StdId),&(CanRxBuf[RcvbufCanPrevi].StdId),sizeof(CanRxBuf)/CAN_RXD_NBUF);
#if 0
    UartPrintHex(20,&(RxMessage.StdId));
#endif
    RcvbufCanPrevi++;
    if(RcvbufCanNum!=0)
    {   RcvbufCanNum--;}
    if(RcvbufCanPrevi>=CAN_RXD_NBUF)
    {	RcvbufCanPrevi=0;}
    
    if(RxMessage.RTR!=CAN_RTR_DATA)
    {   return CAN_RCV_ERR;}
    
    if(RxMessage.IDE!=CAN_ID_EXT)
    {   return CAN_RCV_ERR1;}
    if((RxMessage.DLC>8)||(RxMessage.DLC<2))
    {   return CAN_RCV_ERR5;}
    
//    BigToLit=RxMessage.ExtId;
    //BigtoLittle32(RxMessage.ExtId);
	//单包：首包尾包都标志1
    if(((RxMessage.ExtId&FIRST_CAN_PACK)==FIRST_CAN_PACK)&&((RxMessage.ExtId&END_CAN_PACK)==END_CAN_PACK))
    {   
        if((RxMessage.ExtId&BIT_LOW_PRIOR)==BIT_LOW_PRIOR)
        {   p->bPri=0;}
        else
        {   p->bPri=1;}
        if((RxMessage.ExtId&SEL_ADDR8_32)==SEL_ADDR8_32)
        {   p->bSelAddr=1;}
        else
        {   p->bSelAddr=0;}
        //广播包组包
        if((RxMessage.ExtId&BIT_BRDCAST)==BIT_BRDCAST)
        {   p->bCanBroadcast=1;
            p->CanLen=RxMessage.DLC-2;
            p->dAddr=0xffffffff;
            p->sAddr=0xffffffff;
        	if(p->bSelAddr==0)
        	{   p->sAddr=RxMessage.Data[1];}
        	else
        	{   p->dAddr=RxMessage.Data[1];}//////////////////////////////141202
            memcpy(&(p->InfoCan[0]),&(RxMessage.Data[2]),RxMessage.DLC-2);
        }
		//非广播包组包
        else
        {   p->bCanBroadcast=0;
            p->CanLen=RxMessage.DLC-2;
            if((p->bSelAddr)==0)
            {   //cAddr=RxMessage.ExtId&0xffffff;
                p->dAddr=(RxMessage.ExtId&0xffffff)*0x100+RxMessage.Data[0];    
                p->sAddr=RxMessage.Data[1]; 
            }
            else
            {   cAddr=RxMessage.Data[0];
                cAddr=cAddr*0x100+RxMessage.Data[1];
                p->sAddr=(RxMessage.ExtId&0xffff)*0x10000+cAddr;    
                p->dAddr=(RxMessage.ExtId&0xff0000)/0x10000;            
            }
            memcpy(&(p->InfoCan[0]),&(RxMessage.Data[2]),RxMessage.DLC-2);
        }
        if(RcvbufCanNum==0)
        {   return CAN_RCV_OK;}
        else
        {   return CAN_RCV_OK1;}
    }
	//首包
    if((RxMessage.ExtId&FIRST_CAN_PACK)==FIRST_CAN_PACK)
    {   
        memcpy(&(BigCanPak.StdId),&(RxMessage.StdId),sizeof(RxMessage));
        memset(&(BigPak.flag),0,sizeof(BigPak));
        
        if((RxMessage.ExtId&BIT_LOW_PRIOR)==BIT_LOW_PRIOR)
        {   BigPak.bPri=0;}
        else
        {   BigPak.bPri=1;}
        if((RxMessage.ExtId&SEL_ADDR8_32)==SEL_ADDR8_32)
        {   p->bSelAddr=1;}
        else
        {   p->bSelAddr=0;}
        if((RxMessage.ExtId&BIT_BRDCAST)==BIT_BRDCAST)
        {   BigPak.bCanBroadcast=0;
            BigPak.CanLen=RxMessage.DLC-3;
            BigPak.dAddr=0xffffffff;
            BigPak.sAddr=0xffffffff;
        	if(p->bSelAddr==0)
        	{   p->sAddr=RxMessage.Data[1];}
        	else
        	{   p->dAddr=RxMessage.Data[1];}
            //memcpy(&(BigPak.InfoCan[0]),&(RxMessage.Data[3]),RxMessage.DLC-3);
        }
        else
        {   BigPak.bCanBroadcast=1;
            BigPak.CanLen=RxMessage.DLC-3;
            if((p->bSelAddr)==0)
            {   //cAddr=RxMessage.ExtId&0xffffff;
                BigPak.dAddr=(RxMessage.ExtId&0xffffff)*0x100+RxMessage.Data[0];    
                BigPak.sAddr=RxMessage.Data[1];     
            }
            else
            {   cAddr=RxMessage.Data[0];
                cAddr=cAddr*0x100+RxMessage.Data[1];
                BigPak.sAddr=(RxMessage.ExtId&0xffff)*0x10000+cAddr;
                BigPak.dAddr=(RxMessage.ExtId&0xff0000)/0x10000;    
            }
        }
        memcpy(&(BigPak.InfoCan[0]),&(RxMessage.Data[3]),RxMessage.DLC-3);
        if(BigPakNum==0)
        {   BigPakNum=1;
            return CAN_RCV_NO;
        }
        else
        {   BigPakNum=1;
            return CAN_RCV_ERR2;//已存在，冲掉首包，重新接收
        }
    }
    cAddr=BigCanPak.ExtId;
    CanID=RxMessage.ExtId;
    if((cAddr&(~(FIRST_CAN_PACK|END_CAN_PACK)))!=(CanID&(~(FIRST_CAN_PACK|END_CAN_PACK))))
    {   return CAN_RCV_ERR3;}
    iPakNum=RxMessage.Data[2];
    lCanData=RxMessage.DLC-3;
    
    if(BigPakNum!=iPakNum)
    {   return CAN_RCV_ERR4;}
    memcpy(&(BigPak.InfoCan[BigPak.CanLen]),&(RxMessage.Data[3]),lCanData);//将非中间包拷贝至对应大包缓存位置
    BigPakNum++;
    BigPak.CanLen=BigPak.CanLen+lCanData;    
    if((RxMessage.ExtId&END_CAN_PACK)==END_CAN_PACK)
    {   BigPakNum=0;
		//BigPak.CanLen=iPakNum*5+lCanData;
        memcpy(&(p->flag),&(BigPak.flag),sizeof(BigPak));
        if(RcvbufCanNum==0)
        {   return CAN_RCV_OK;}
        else
        {   return CAN_RCV_OK1;}
    }
    return CAN_RCV_NO;
}

DWORD ReadAddrMap(DWORD Addr)
{
	//TCHAR buffer[20];
	DWORD IP;
	wchar_t buildingString[20],UnitString[20];
	LPCTSTR Building,Unit;
	DWORD build_temp,Unit_temp;
	build_temp=(Addr>>22)&(0x3ff);
	Unit_temp=(Addr>>16)&(0x3f);
	ZeroMemory(buildingString,sizeof(buildingString));
	ZeroMemory(UnitString,sizeof(UnitString));
	//_itoa(build_temp,buildingString,10);
	_itow(build_temp,buildingString,10);
	Building=(LPCTSTR)buildingString;
	//_itoa(Unit_temp,UnitString,10);
	_itow(Unit_temp,UnitString,10);
	Unit=(LPCTSTR)UnitString;
	IP=(DWORD)CeGetPrivateProfileInt(Building, Unit, -1, _T("\\UserDev\\SysSet.ini"));
	return IP;
}

void CanRSendBufCanPAK(BufCan Tmpcbuf,BYTE CODE)
{
	BufCan CanRSendPak;
	CanRSendPak.dAddr=Tmpcbuf.sAddr;
	CanRSendPak.sAddr=Tmpcbuf.dAddr;			
	CanRSendPak.CanLen=3;
	CanRSendPak.bSelAddr=SelAddr_s32_d8;
	CanRSendPak.bPri=Low_pri;
	CanRSendPak.InfoCan[0]=Tmpcbuf.InfoCan[0]+0x80;
	CanRSendPak.InfoCan[1]=Tmpcbuf.InfoCan[1];
	CanRSendPak.InfoCan[2]=CODE;
	if(MTransDataCan((BufCan*)(&CanRSendPak)))//1：发送失败
		DBGMSG(DPINFO, "MTransDataCan((BufCan*)(&CanRSendPak)) fail\r\n");
	if(TxdbufCanNum!=0)
	{
		SetEvent(Can_RunWriteThread_event);
	}
}

void CanRSendTCPPAK(Network_Ipswich_TCP MSG,BYTE CODE)
{
	BufCan CanRSendPak;
	CanRSendPak.dAddr=MSG.nitcphead.fromaddr;
	CanRSendPak.sAddr=MSG.nitcphead.toaddr;			
	CanRSendPak.CanLen=3;
	CanRSendPak.bSelAddr=SelAddr_s32_d8;
	CanRSendPak.bPri=Low_pri;
	CanRSendPak.InfoCan[0]=MSG.nitcphead.headcommand+0x80;
	CanRSendPak.InfoCan[1]=MSG.nitcphead.headmsgsession;
	CanRSendPak.InfoCan[2]=CODE;
	if(MTransDataCan((BufCan*)(&CanRSendPak)))//1：发送失败
		DBGMSG(DPINFO, "MTransDataCan((BufCan*)(&CanRSendPak)) fail\r\n");
	if(TxdbufCanNum!=0)
	{
		SetEvent(Can_RunWriteThread_event);
	}
}

BOOL WaitSaveProcess(BufCan Tmpcbuf)
{
	
	TalkEventSaveBuf.dAddr=Tmpcbuf.dAddr;//mudi
	TalkEventSaveBuf.sAddr=Tmpcbuf.sAddr;
	TalkEventSaveBuf.Command=Tmpcbuf.InfoCan[0];
	TalkEventSaveBuf.Session=Tmpcbuf.InfoCan[1];
	TalkEventSaveBuf.dIP=ReadAddrMap(Tmpcbuf.dAddr);
	if (TalkEventSaveBuf.dIP==0xffffffff)
	{
		return FALSE;
	}
	return TRUE;
}

extern BYTE VideoLenthLeveal;
extern BYTE VideoLevealSwitch;
extern BYTE OpenvaUpMonitor;
DWORD CanDataProcess(BufCan Tmpcbuf)
{
	if (Tmpcbuf.dAddr<=0x0f)
	{
		DBGMSG(DPINFO, "Tmpcbuf.dAddr<=0x0f\r\n");
		if(Tmpcbuf.InfoCan[0]==CODET_Call)
		{
			VideoLevealSwitch=1;
			VideoLenthLeveal=Tmpcbuf.InfoCan[2];
		}
		else if(Tmpcbuf.InfoCan[0]==RCODET_MONITOR)
		{
			if (OpenvaUpMonitor)
			{
				OpenvaUpMonitor=0;
				VideoLevealSwitch=1;
				VideoLenthLeveal=Tmpcbuf.InfoCan[3];					
				m_MedeiaLApp.OpenvaUpMonitorMedia();
			}
		}
		return SendPak;
	}
	else
	{
		switch (Tmpcbuf.InfoCan[0])
		{
			case CODET_Call:
				if (ReadAddrMap(Tmpcbuf.dAddr)==My_ip)//自己下挂设备
				{
					DBGMSG(DPINFO, "ReadAddrMap(Tmpcbuf.dAddr)==My_ip\r\n");
					return ThrowPak;
				}
				//else if (Tmpcbuf.dAddr<=0x0f)//网管
				//{
				//	DBGMSG(DPINFO, "Tmpcbuf.dAddr<=0x0f\r\n");
				//	return SendPak;
				//}
				else
				{
					if (WaitSaveProcess(Tmpcbuf))
					{
						Clock60MSecs=ClockCleared;
						if(handshack_value==closeall)
						{
							m_MedeiaLApp.NmcNumNow=TalkEventSaveBuf.dAddr;
							m_MedeiaLApp.NmcIpNow=TalkEventSaveBuf.dIP;
							if(SendDIg_handshack(m_MedeiaLApp.NmcNumNow,m_MedeiaLApp.NmcIpNow,open_va_down_call))//下ips为vadown
							{
								VideoLevealSwitch=1;
								VideoLenthLeveal=Tmpcbuf.InfoCan[2];
								handshack_value=open_va_up_call;
								DBGMSG(DPINFO, "WaitSaveProcess(Tmpcbuf)=true, handshack_value=open_va_up_call\r\n");
								return WaitSave;	
							}
							else//对接的ips不在线或有误
							{
								DBGMSG(DPINFO, "WaitSaveProcess(Tmpcbuf)=true,but device is not online\r\n");
								CanRSendBufCanPAK(BufCanWaitSave,RCODET_Call_BUSY);
								handshack_value=closeall;
								VideoLevealSwitch=0;
								VideoLenthLeveal=0;
								ZeroMemory(&TalkEventSaveBuf,sizeof(TalkEventSaveBuf));
								return ThrowPak;
							}
						}
						else if((handshack_value==open_v_up)||(handshack_value==open_va_up_monitor))
						{
							SendDIg_handshack(m_MedeiaLApp.NmcNumNow,m_MedeiaLApp.NmcIpNow,closeall);//关闭现在优先级小的呼叫操作
							DBGMSG(DPINFO, "---->else if((handshack_value==open_v_up)||(handshack_value==open_va_up_monitor))\r\n");
							m_MedeiaLApp.CloseMedia();
							HandTimeDef();
							DPPostMessage(MSG_PHONECALL, MSG_CALL_HUNGUP, 0, 0);
							InterruptMonitorAddr=m_MedeiaLApp.NmcNumNow;
							//*********主动发起，当前为将发起至的目的地址和目的IP**************
							m_MedeiaLApp.NmcNumNow=TalkEventSaveBuf.dAddr;
							m_MedeiaLApp.NmcIpNow=TalkEventSaveBuf.dIP;
							if (SendDIg_handshack(m_MedeiaLApp.NmcNumNow,m_MedeiaLApp.NmcIpNow,open_va_down_call))
							{
								handshack_value=open_va_up_call;
								VideoLevealSwitch=1;
								VideoLenthLeveal=Tmpcbuf.InfoCan[2];
								return WaitSave;
							}
							else//对接的ips不在线或有误
							{
								DBGMSG(DPINFO, "WaitSaveProcess(Tmpcbuf)=true,but is not online\r\n");
								CanRSendBufCanPAK(BufCanWaitSave,RCODET_Call_BUSY);
								handshack_value=closeall;
								VideoLevealSwitch=0;
								VideoLenthLeveal=0;
								ZeroMemory(&TalkEventSaveBuf,sizeof(TalkEventSaveBuf));
								return ThrowPak;
							}

						}
						else if ((handshack_value!=closeall)&&(Tmpcbuf.sAddr!=TalkEventSaveBuf.sAddr))//自身呼叫忙,其他设备来呼叫
						{
							CanRSendBufCanPAK(Tmpcbuf,RCODET_Call_BUSY);
							DBGMSG(DPINFO, "((handshack_value!=closeall)&&(Tmpcbuf.sAddr!=TalkEventSaveBuf.sAddr))\r\n");
							return ThrowPak;
						}
						else
							return SendPak;
					}
					else
					{
						DBGMSG(DPINFO, "This device is not present in the address table\r\n");
						return ThrowPak;//地址表无此地址设备
					}
				}
			case END_Application:
				if ((handshack_value!=closeall)&&((TalkEventSaveBuf.sAddr)==Tmpcbuf.sAddr))
				{
					HeartBeatTmp=FALSE;
					Clock50MSecs=ClockCleared;
					Clock3Secs=Clock_Dis;
					SendDIg_handshack(TalkEventSaveBuf.dAddr,TalkEventSaveBuf.dIP,closeall);
					DBGMSG(DPINFO, "---->(handshack_value!=closeall)&&((TalkEventSaveBuf.sAddr)==Tmpcbuf.sAddr),Tmpcbuf.InfoCan[0]=%x\r\n",Tmpcbuf.InfoCan[0]);
					m_MedeiaLApp.CloseMedia();
					HandTimeDef();
					DPPostMessage(MSG_PHONECALL, MSG_CALL_HUNGUP, 0, 0);
					ZeroMemory(&TalkEventSaveBuf,sizeof(TalkEventSaveBuf));
				}
				return SendPak;
/*			case RCODET_MONITOR:
				if (OpenvaUpMonitor)
				{
					OpenvaUpMonitor=0;
					VideoLevealSwitch=1;
					VideoLenthLeveal=Tmpcbuf.InfoCan[3];					
					m_MedeiaLApp.OpenvaUpMonitorMedia();
				}
				return SendPak;*/
			case RCODET_HangUp:
			default:
				return SendPak;
		}
	
	}
}

static DWORD Can_RunReadThread(HANDLE lparam)
{
	CAN_INFO readinfo;	
	DWORD len;
	while(1)
	{
		memset(&readinfo,0,sizeof(CAN_INFO));
		len=0;
		if(ReadFile(hcan,&readinfo,sizeof(CAN_INFO),&len,NULL)) 
		{
			CanRxBuf[RcvbufCanNow].ExtId=readinfo.Id;
			CanRxBuf[RcvbufCanNow].DLC=readinfo.datalen;
			if(readinfo.ExternFlag)//Externalflag=1，拓展标识符
				CanRxBuf[RcvbufCanNow].IDE=CAN_ID_EXT;
			else
				CanRxBuf[RcvbufCanNow].IDE=CAN_ID_STD;
			if(readinfo.RemoteFlag)//RemoteFlag =1，远程标识符
				CanRxBuf[RcvbufCanNow].RTR=CAN_RTR_REMOTE;
			else
				CanRxBuf[RcvbufCanNow].RTR=CAN_RTR_DATA;
			memcpy(&(CanRxBuf[RcvbufCanNow].Data),&(readinfo.data),readinfo.datalen);
			
			printf("========CanRecPAK=======\r\n");
			DBGTime();
			printf("CANRecPAK:");
			DBGCanPak(CanRxBuf[RcvbufCanNow].ExtId,(BYTE *)&CanRxBuf[RcvbufCanNow].Data,CanRxBuf[RcvbufCanNow].DLC);

			RcvbufCanNum++;
    		RcvbufCanNow++;
    		if( RcvbufCanNow>=CAN_RXD_NBUF)
    		{	RcvbufCanNow=0;}	

			LedFlashCon(NCAN_RunRLEDR,2,2,2,0);//LedCanRx			
			RcvResult=RcvDataFromCan((BufCan*)(&(Tmpcbuf.flag)));
			if((RcvResult==CAN_RCV_OK)||(RcvResult==CAN_RCV_OK1))
			{
				switch(CanDataProcess(Tmpcbuf))
				{
					case ThrowPak:
						break;
					case SendPak:
						BufCan_list.AddTail(Tmpcbuf);
						SetEvent(ETHTransThread_event);
						break;
					case WaitSave:
						memcpy(&(BufCanWaitSave.flag),&(Tmpcbuf.flag),sizeof(BufCanWaitSave));
						break;
				}
				
			}
			else if(RcvResult>=CAN_RCV_ERR)
			{  
				RcvResult=CAN_RCV_NO;
			}
		}
		else if (GetLastError()==0)
		{
			printf("GetLastE is 0!!!\r\n");
			CanRxBuf[RcvbufCanNow].ExtId=readinfo.Id;
			CanRxBuf[RcvbufCanNow].DLC=readinfo.datalen;
			if(readinfo.ExternFlag)//Externalflag=1，拓展标识符
				CanRxBuf[RcvbufCanNow].IDE=CAN_ID_EXT;
			else
				CanRxBuf[RcvbufCanNow].IDE=CAN_ID_STD;
			if(readinfo.RemoteFlag)//RemoteFlag =1，远程标识符
				CanRxBuf[RcvbufCanNow].RTR=CAN_RTR_REMOTE;
			else
				CanRxBuf[RcvbufCanNow].RTR=CAN_RTR_DATA;
			memcpy(&(CanRxBuf[RcvbufCanNow].Data),&(readinfo.data),readinfo.datalen);

			printf("========CanRecPAK=======\r\n");
			DBGTime();
			printf("CANRecPAK:");
			DBGCanPak(CanRxBuf[RcvbufCanNow].ExtId,(BYTE *)&CanRxBuf[RcvbufCanNow].Data,CanRxBuf[RcvbufCanNow].DLC);

			RcvbufCanNum++;
			RcvbufCanNow++;
			if( RcvbufCanNow>=CAN_RXD_NBUF)
			{	RcvbufCanNow=0;}	

			LedFlashCon(NCAN_RunRLEDR,2,2,2,0);//LedCanRx			
			RcvResult=RcvDataFromCan((BufCan*)(&(Tmpcbuf.flag)));
			if((RcvResult==CAN_RCV_OK)||(RcvResult==CAN_RCV_OK1))
			{
				switch(CanDataProcess(Tmpcbuf))
				{
					case ThrowPak:
						break;
					case SendPak:
						BufCan_list.AddTail(Tmpcbuf);
						SetEvent(ETHTransThread_event);
						break;
					case WaitSave:
						memcpy(&(BufCanWaitSave.flag),&(Tmpcbuf.flag),sizeof(BufCanWaitSave));
						break;
				}

			}
			else if(RcvResult>=CAN_RCV_ERR)
			{  
				RcvResult=CAN_RCV_NO;
			}

		}
		else
			printf("ReadFile fail!!,error is %d\r\n",GetLastError());		 
		
	}
	CloseHandle(hcan);
	return TRUE;
}

static DWORD Can_RunWriteThread(HANDLE lparam)
{
	CAN_INFO writeinfo[50];
	CanTxMsg TxMessage;
	DWORD len;
	while(1)
	{
		SetEvent(ThreadRunEvent[MSG_EVENT_CANWRITE]);
		//DBGMSG(DPINFO, "MSG_EVENT_CANWRITE\r\n");
		WaitForSingleObject(Can_RunWriteThread_event,2000);
		{
			//ZeroMemory(&writeinfo,sizeof(writeinfo));
			if(TxdbufCanNum!=0)
			{
				for(BYTE i=0;i<TxdbufCanNum;i++)
				{
					//memcpy(&TxMessage.StdId,&CanTxBuf[TxdbufCanPrevi].StdId,sizeof(CanTxBuf)/CAN_TXD_NBUF);
					if(CanTxBuf[TxdbufCanPrevi].StdId!=0)
					{
						memcpy(&TxMessage.StdId,&CanTxBuf[TxdbufCanPrevi].StdId,sizeof(CanTxBuf)/CAN_TXD_NBUF);
						writeinfo[i].Id=TxMessage.ExtId;
						if(TxMessage.IDE==CAN_ID_EXT)
							writeinfo[i].ExternFlag=Penable;
						else
							writeinfo[i].ExternFlag=Pdisable;
						if(TxMessage.RTR==CAN_RTR_REMOTE)
							writeinfo[i].RemoteFlag=Penable;
						else
							writeinfo[i].RemoteFlag=Pdisable;
						writeinfo[i].datalen=TxMessage.DLC;
						memcpy(&(writeinfo[i].data),&(CanTxBuf[TxdbufCanPrevi].Data),writeinfo[i].datalen);
						printf("=======CanSendPAK=======\r\n");
						DBGTime();
						printf("CanSendPAK command is 0x%x,session is 0x%x,TxdbufCanPrevi is %d,TxdbufCanNum is0x%x\r\n",writeinfo[i].data[2],writeinfo[i].data[3],TxdbufCanPrevi,TxdbufCanNum);
						TxdbufCanPrevi++;
						if(TxdbufCanPrevi>=CAN_TXD_NBUF)
						{	TxdbufCanPrevi=0;}
					}
				}
				if(WriteFile(hcan,&writeinfo,TxdbufCanNum*sizeof(CAN_INFO),&len,NULL))//WriteFile(hcan,writeinfo,5*sizeof(CAN_INFO),&len,NULL)
				{
					TxdbufCanNum=0;
					LedFlashCon(NCAN_RunGLEDT,2,2,2,1);//LedCanTx//
				}
				else
				{
					TxdbufCanNum=0;//写数据出错，不再进行写操作，数据包丢弃，应用层防止此处死循环无法结束
					printf("write wrong\r\n");
				}	
			}
		}
	}
	CloseHandle(hcan);
	return TRUE;
}

BOOL enable_Can_RunThread()
{
	HANDLE hThread1,hThread2;
	//CanBandRate bandrate = BandRate_100kbps;
	BYTE cnfregdata[3];
	//DWORD ctrolcode = 0x1234567d;
	//SJW=1,SOC=8MHZ----->bandrate=100K			
	//cnfregdata[0] = 0x1;
	//cnfregdata[1] = 0xbe;
	//cnfregdata[2] = 0x3;
	//SJW=1,TBS1=SYN+PS1=7+7=14,TBS2=PS2=2,BRP=2*(brp+1)=8,SOC=8MHZ----->bandrate=58.82K
	cnfregdata[0] = 0x3;
	cnfregdata[1] = 0xb6;
	cnfregdata[2] = 0x1;

	hcan =  CreateFile(_T("CAN1:"),GENERIC_READ|GENERIC_WRITE,0,NULL,OPEN_EXISTING,0,NULL);
	if(hcan == INVALID_HANDLE_VALUE)
	{
		printf("OPEN CAN1 device fail,error is %d \r\n",GetLastError());
		return FALSE;
	}

	//DeviceIoControl(hcan,CAN_IOCTL_SETBANDRATE,&bandrate,1,NULL,0,NULL,NULL);
	DeviceIoControl(hcan,CAN_IOCTL_SETBANDRATE,cnfregdata,3,NULL,0,NULL,NULL);

	hThread1 = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)Can_RunReadThread, NULL, 0,NULL);
	if(hThread1  == NULL)
		printf("CreateThread fail,error is %d",GetLastError());
	//CeSetThreadPriority(hThread1,95);
	CeSetThreadPriority(hThread1,202);
	CloseHandle(hThread1);
	
	hThread2 = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)Can_RunWriteThread, NULL, 0, NULL);
	if(hThread2  == NULL)
		printf("CreateThread fail,error is %d",GetLastError());
	CeSetThreadPriority(hThread2,201);
	CloseHandle(hThread2);
	return TRUE;
}
