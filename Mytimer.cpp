#include "PlatformConfig.h"
#include "stdafx.h"
#include <Atlcoll.h>
#include "RoomInf.h"
#include "database.h"
#include "mmsystem.h"
#include "phoneappexport.h"
#include "systemmsg.h"
#include "gpio.h"
#include "eth.h"
#include "MyTimer.h"
#include"PilotLamp.h"
#include"CanThread.h"
#pragma comment(lib, "Mmtimer.lib")

DWORD g_ClockTenthSecs=0;
bool ledrun_i=0;
MMRESULT TimerID_1s;
extern BOOL MinsIndex;

#define 	LED_NUMBER      5

UINT8	bLedAvCon[LED_NUMBER];  //0 不控制  1 控制
UINT8	bLedAv[LED_NUMBER];     //0 灭      1 亮

UINT8	LedTimCon[LED_NUMBER];		
UINT8	LedAvSet[LED_NUMBER];
UINT8	LedNoAvSet[LED_NUMBER];
UINT8	LedAvNumCon[LED_NUMBER];
UINT8	LedTimerCon;
extern TalkEventParam TalkEventSaveBuf;
extern BOOL SixSecindex;


void HandTimeDef()
{
	Clock5TMSecs=ClockCleared;
	Clock5Mins=Clock_Dis;
	MinsIndex=FALSE;
}

void HandTimeDone()
{
	HandTimeDef();
	m_MedeiaLApp.CloseMedia();
	DPPostMessage(MSG_PHONECALL, MSG_CALL_HUNGUP, 0, 0);
}

void LedParaInit()
{	
    memset(bLedAvCon,0,sizeof(bLedAvCon));
    memset(bLedAv,0,sizeof(bLedAv));
    memset(LedTimCon,0,sizeof(LedTimCon));
    memset(LedAvSet,0,sizeof(LedAvSet));
    memset(LedNoAvSet,0,sizeof(LedNoAvSet));
    memset(LedAvNumCon,0,sizeof(LedAvNumCon));	
    LedTimerCon=0;
}

void LedCon(UINT8 LedNum,UINT8 bAvail)
{	
	switch(LedNum)
	{   
	    case NRunLED:
	    if(bAvail)
	    {  SetGpioVal(GIO[7],0);}
	    else
	    {  SetGpioVal(GIO[7],1);}
	    break;
	    
	    case NCAN_RunGLEDT:
		if(bAvail)
	    {  SetGpioVal(GIO[8],0);}
	    else
	    {  SetGpioVal(GIO[8],1);}
	    break;
	    
	    case NCAN_RunRLEDR:
		if(bAvail)
	    {  SetGpioVal(GIO[9],0);}
	    else
	    {  SetGpioVal(GIO[9],1);}
	    break;
	    
	    case NRS485_RunLEDT:
	    if(bAvail)
	    {  SetGpioVal(GIO[19],0);}
	    else
	    {  SetGpioVal(GIO[19],1);}
	    break;
	    
	    case NRS485_RunLEDR:
		if(bAvail)
	    {  SetGpioVal(GIO[20],0);}
	    else
	    {  SetGpioVal(GIO[20],1);}
	    break;
	    
	    default: 
		break;
	}    
}

//显示灯亮时间 AvDeSec,显示灯灭时间 NoAvDeSec,闪烁次数 AvNum
// AvDeSec=0 NoAvDeSec=非0 常灭 
// AvDeSec=非0 NoAvDeSec=0 常亮
// AvNum 永远闪烁
void LedFlashCon(UINT8 LedNum,UINT8 AvDeSec,UINT8 NoAvDeSec,UINT8 AvNum,UINT8 LedAvValue)
{	
	if((AvDeSec==0)&&(NoAvDeSec!=0))
	{   
	    bLedAvCon[LedNum]=0;
	    LedCon(LedNum,0);
	    return;
	}
	if((AvDeSec!=0)&&(NoAvDeSec==0))
	{   
	    bLedAvCon[LedNum]=0;
	    LedCon(LedNum,1);///////////////////////return?
	}
	
	LedTimCon[LedNum]=AvDeSec;
	LedAvSet[LedNum]=AvDeSec;
	LedNoAvSet[LedNum]=NoAvDeSec;
	LedAvNumCon[LedNum]=AvNum;
	
	if (LedAvValue)
	{
		bLedAv[LedNum]=1;//0 灭      1 亮
	}
	else
		bLedAv[LedNum]=0;//0 灭      1 亮

	bLedAvCon[LedNum]=1;//0 不控制  1 控制
	LedCon(LedNum,1);
}
void LedInit()
{	
	LedParaInit();
    LedFlashCon(NRunLED,10,10,0,1);
    LedFlashCon(NCAN_RunGLEDT,2,2,10,1);
    LedFlashCon(NCAN_RunRLEDR,2,2,10,0);
    LedFlashCon(NRS485_RunLEDT,4,4,10,1);
    LedFlashCon(NRS485_RunLEDR,4,4,10,0);    
}



void PASCAL CALLBACK SysLedBlinkThread(UINT uTimerID, UINT uMsg, DWORD dwUser, DWORD dw1, DWORD dw2)
{
	g_ClockTenthSecs++;
	
	//SetEvent(LedRun_event);
	for(BYTE i=0;i<LED_NUMBER;i++)
	{
		if(bLedAvCon[i]==1)//控制
		{	
			if(LedTimCon[i]==1)//常亮时间数
			{	if(bLedAv[i]==1)//亮
				{	LedTimCon[i]=LedNoAvSet[i];
					LedCon(i,0);
					bLedAv[i]=0;
				}
				else//灭
				{	
					LedTimCon[i]=LedAvSet[i];
					LedCon(i,1);
					bLedAv[i]=1;
				}
    			
				if(LedAvNumCon[i]==1)//亮1次
				{	bLedAvCon[i]=0;}//
				else if(LedAvNumCon[i]==0)
				{   ;}
				else
				{	LedAvNumCon[i]--;}
			}
			else
			{   LedTimCon[i]--;}
		}
	}

}
void enable_SysLedBlinkThread()
{
	g_ClockTenthSecs=0;
	MMRESULT hthread=timeSetEvent(tickclock, tickAccuracy, (LPTIMECALLBACK)SysLedBlinkThread, NULL, TIME_PERIODIC | TIME_CALLBACK_FUNCTION);
	if(hthread  == NULL)
		DBGMSG(DPERROR,"createthread SysLedBlinkThread fail,error is %d",GetLastError());
}


void WatchDog_Con(unsigned char type)
{
	if (type)
	{
		SetGpioVal(GIO[22],1);
	}
	else
	{
		SetGpioVal(GIO[22],0);	
	}
}


void WatchDog706(void)
{
	//static unsigned char feed_time_count = 0;
	static unsigned char feed_value = 0;
	//feed_time_count ++;
	//if(feed_time_count == 1)
	{ 
		//feed_time_count = 0;
		if(feed_value == 0){
			WatchDog_Con(1);
			feed_value = 1;
		}
		else{
			WatchDog_Con(0);
			feed_value = 0;
		}
	}
}

extern BYTE VideoLenthLeveal;
extern BYTE VideoLevealSwitch;
void PASCAL CALLBACK TimerID_50msThread(UINT uTimerID, UINT uMsg, DWORD dwUser, DWORD dw1, DWORD dw2)//500ms 150422
{
	WatchDog706();
	//网管发起heatbeat事件
	if(Clock4Secs==Clock_En)
	{
		Clock5TenMSecs++;
		if(Clock5TenMSecs==ClockfourSecs)
		{			
			Clock5TenMSecs=ClockCleared;
			//网管死机,无心跳，数据超时自动挂机
			DPPostMessage(MSG_PHONECALL, MSG_CALL_HUNGUP, 0, 0);
			m_MedeiaLApp.CloseMedia();//150305 add ,需验证逻辑是否需要此操作处理！！---OK
			HandTimeDef();
			DBGMSG(DPINFO,"void PASCAL CALLBACK TimerID_50msThread(UINT uTimerID, UINT uMsg, DWORD dwUser, DWORD dw1, DWORD dw2)");
			Clock4Secs=Clock_Dis;
		}
	}
	else
		Clock5TenMSecs=ClockCleared;

	//主动发起heartbeat事件
	if(Clock3Secs==Clock_En)
	{
		Clock50MSecs++;
		if(Clock50MSecs==ClockTreeSecs)
		{			
			Clock50MSecs=ClockCleared;
			HeartBeatTmp=TRUE;
			SetEvent(ETHTransThread_event);
			Clock3Secs=Clock_Dis;
		}
	}
	else
		Clock50MSecs=ClockCleared;

	//终端设备错误、导致无法释放、此处加入自动超时释放机制
	if(Clock5Mins==Clock_En)
	{
		Clock5TMSecs++;
		if(Clock5TMSecs==ClockFiveMins)
		{			
			HandTimeDone();
			//DBGMSG(DPINFO,"void PASCAL CALLBACK TimerID_50msThread(UINT uTimerID, UINT uMsg, DWORD dwUser, DWORD dw1, DWORD dw2)");
		}
	}
	else
		Clock5TMSecs=ClockCleared;
	
	//20150616  大门呼叫下属设备、下属设备不在线且大门断开不发送3a状况解决
	if(Clock6Secs==Clock_En)
	{
		Clock60MSecs++;
		if(Clock60MSecs==ClockSixSecs)
		{			
			Clock60MSecs=ClockCleared;
			Clock6Secs=Clock_Dis;

			HeartBeatTmp=FALSE;
			Clock50MSecs=ClockCleared;
			Clock3Secs=Clock_Dis;
			SendDIg_handshack(TalkEventSaveBuf.dAddr,TalkEventSaveBuf.dIP,closeall);
			DBGMSG(DPINFO, "38---->TalkEventSaveBuf clear\r\n");
			handshack_value=closeall;
			VideoLevealSwitch=0;
			VideoLenthLeveal=0;
			SixSecindex=FALSE;
			ZeroMemory(&TalkEventSaveBuf,sizeof(TalkEventSaveBuf));
		}
	}
	else
		Clock60MSecs=ClockCleared;

	//20151201:延时调用callips
	if(ClockDelayTCallIps==Clock_En)
	{
		ClockDelayCallIps++;
		if(ClockDelayCallIps==ClockOneSecs)
		{			
			ClockDelayCallIps=ClockCleared;
			ClockDelayTCallIps=Clock_Dis;
			//PhoneCallIps(TalkEventSaveBuf.dIP);
			DPPostMessage(MSG_PHONECALL, MSG_NEW_CALLOUT, 0, 0);
			DBGMSG(DPINFO,"MSG_PHONECALL----------------------------->>PhoneCallIpsNow\r\n");
		}
	}
	else
		ClockDelayCallIps=ClockCleared;


	SetEvent(OperComDataThread_event);
}


void enable_TimerID_msAccuracy_Thread()
{
	MMRESULT Timerthread=timeSetEvent(500, 10, (LPTIMECALLBACK)TimerID_50msThread, NULL, TIME_PERIODIC | TIME_CALLBACK_FUNCTION);
	if(Timerthread  == NULL)
		DBGMSG(DPERROR,"createthread enable_TimerID_msAccuracy_Thread fail,error is %d",GetLastError());
		
}

C3sTimerApp::C3sTimerApp()
{
	Timerthread=NULL;
}
C3sTimerApp::~C3sTimerApp()
{
	Timerthread=NULL;
}
void C3sTimerApp::TimerID_3sAccuracy_Open()
{
	Timerthread=timeSetEvent(20, 10, (LPTIMECALLBACK)TimerID_50msThread, NULL, TIME_PERIODIC | TIME_CALLBACK_FUNCTION);
	if(Timerthread  == NULL)
		DBGMSG(DPERROR,"createthread TimerID_msAccuracy_Open fail,error is %d",GetLastError());
		
}
void C3sTimerApp::TimerID_3sAccuracy_Close()
{
	if(timeKillEvent(Timerthread)==TIMERR_NOERROR)
		DBGMSG(DPERROR,"timeKillEvent(Timerthread)==TIMERR_NOERROR");
	else
		DBGMSG(DPERROR,"timeKillEvent(Timerthread)==MMSYSERR_INVALPARAM");
}