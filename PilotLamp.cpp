//#include "PlatformConfig.h"
//#include "RoomInf.h"
//#include "database.h"
//#include "mmsystem.h"
//#include "systemmsg.h"
//#include "gpio.h"
//#include "MyTimer.h"
//#include"PilotLamp.h"
//
//#define 	LED_NUMBER      5
//
//UINT8	bLedAvCon[LED_NUMBER];  //0 不控制  1 控制
//UINT8	bLedAv[LED_NUMBER];     //0 灭      1 亮
//
//UINT8	LedTimCon[LED_NUMBER];		
//UINT8	LedAvSet[LED_NUMBER];
//UINT8	LedNoAvSet[LED_NUMBER];
//UINT8	LedAvNumCon[LED_NUMBER];
//UINT8	LedTimerCon;
//
//extern HANDLE GIO[21];
//extern DWORD g_ClockTenthSecs;
//
//void LedParaInit()
//{	
//    memset(bLedAvCon,0,sizeof(bLedAvCon));
//    memset(bLedAv,0,sizeof(bLedAv));
//    memset(LedTimCon,0,sizeof(LedTimCon));
//    memset(LedAvSet,0,sizeof(LedAvSet));
//    memset(LedNoAvSet,0,sizeof(LedNoAvSet));
//    memset(LedAvNumCon,0,sizeof(LedAvNumCon));	
//    LedTimerCon=0;
//}
//
//void LedCon(UINT8 LedNum,UINT8 bAvail)
//{	
//	switch(LedNum)
//	{   
//	    case NRunLED:
//	    if(bAvail)
//	    {  SetGpioVal(GIO[7],0);}
//	    else
//	    {  SetGpioVal(GIO[7],1);}
//	    break;
//	    
//	    case NCAN_RunGLEDT:
//		if(bAvail)
//	    {  SetGpioVal(GIO[8],0);}
//	    else
//	    {  SetGpioVal(GIO[8],1);}
//	    break;
//	    
//	    case NCAN_RunRLEDR:
//		if(bAvail)
//	    {  SetGpioVal(GIO[9],0);}
//	    else
//	    {  SetGpioVal(GIO[9],1);}
//	    break;
//	    
//	    case NRS485_RunLEDT:
//	    if(bAvail)
//	    {  SetGpioVal(GIO[19],0);}
//	    else
//	    {  SetGpioVal(GIO[19],1);}
//	    break;
//	    
//	    case NRS485_RunLEDR:
//		if(bAvail)
//	    {  SetGpioVal(GIO[20],0);}
//	    else
//	    {  SetGpioVal(GIO[20],1);}
//	    break;
//	    
//	    default: 
//		break;
//	}    
//}
//
////显示灯亮时间 AvDeSec,显示灯灭时间 NoAvDeSec,闪烁次数 AvNum
//// AvDeSec=0 NoAvDeSec=非0 常灭 
//// AvDeSec=非0 NoAvDeSec=0 常亮
//// AvNum 永远闪烁
//void LedFlashCon(UINT8 LedNum,UINT8 AvDeSec,UINT8 NoAvDeSec,UINT8 AvNum)
//{	
//	if((AvDeSec==0)&&(NoAvDeSec!=0))
//	{   
//	    bLedAvCon[LedNum]=0;
//	    LedCon(LedNum,0);
//	    return;
//	}
//	if((AvDeSec!=0)&&(NoAvDeSec==0))
//	{   
//	    bLedAvCon[LedNum]=0;
//	    LedCon(LedNum,1);///////////////////////return?
//	}
//	
//	LedTimCon[LedNum]=AvDeSec;
//	LedAvSet[LedNum]=AvDeSec;
//	LedNoAvSet[LedNum]=NoAvDeSec;
//	LedAvNumCon[LedNum]=AvNum;
//	
//	bLedAv[LedNum]=1;//0 灭      1 亮
//	bLedAvCon[LedNum]=1;//0 不控制  1 控制
//	LedCon(LedNum,1);
//}
//void LedInit()
//{	
//	LedParaInit();
//    LedFlashCon(NRunLED,10,10,0);
//    LedFlashCon(NCAN_RunGLEDT,2,2,10);
//    LedFlashCon(NCAN_RunRLEDR,2,2,11);
//    LedFlashCon(NRS485_RunLEDT,4,4,10);
//    LedFlashCon(NRS485_RunLEDR,4,4,11);    
//}
//
//
//extern HANDLE LedRun_event;
//static DWORD LedRunThread(HANDLE lparam)
//{	
//    UINT8	i;
//    
//	while(1)
//	{
//		//WaitForSingleObject(LedRun_event,100);
//		i=(UINT8)g_ClockTenthSecs;
//		
//		if(i==LedTimerCon)
//		{   continue;}
//		LedTimerCon=i;//过了0.1s
//	    
//		for(i=0;i<LED_NUMBER;i++)
//		{
//			if(bLedAvCon[i]==1)//控制
//    		{	
//    			if(LedTimCon[i]==1)//常亮时间数
//    			{	if(bLedAv[i]==1)//亮
//    				{	LedTimCon[i]=LedNoAvSet[i];
//    					LedCon(i,0);
//						bLedAv[i]=0;
//    				}
//    				else//灭
//    				{	
//    					LedTimCon[i]=LedAvSet[i];
//    					LedCon(i,1);
//						bLedAv[i]=1;
//    				}
//	    			
//    				if(LedAvNumCon[i]==1)//亮1次
//    				{	bLedAvCon[i]=0;}//
//    				else if(LedAvNumCon[i]==0)
//    				{   ;}
//    				else
//    				{	LedAvNumCon[i]--;}
//    			}
//    			else
//    			{   LedTimCon[i]--;}
//    		}
//		}
//	}
//	return FALSE;
//}
//
//BOOL enable_LED_RunThread()
//{
//	HANDLE hThread;
//	hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)LedRunThread, NULL, 0, NULL);
//	if(hThread  == NULL)
//	{
//		printf("CreateThread fail,error is %d",GetLastError());
//		return FALSE;
//	}
//	CloseHandle(hThread);
//	return TRUE;
//}
