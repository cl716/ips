#if 0
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
#include "CMediaLogic.h"

CMedeiaLogicApp::CMedeiaLogicApp()
{
	handshack_value=closeall;
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

void CMedeiaLogicApp::CloseMedia()
{
	while(hCallSessionList.GetCount())
	{
		hCallSessionList.RemoveHead();
	}
	handshack_value=closeall;//关断模拟通道

	Channel_switch(Relay_audio,Relay_audio_close);
	Channel_switch(Relay_video,Relay_video_up);
	Channel_switch(audio_power,audio_power_close);
	Channel_switch(video_power,video_power_close);

	DBGMSG(DPERROR, "CMedeiaLogicApp::CloseMedia()\r\n");
}
void CMedeiaLogicApp::OpenvideoUpMedia()
{	
	//handshack_value=open_v_up;
	Channel_switch(audio_power,audio_power_close);
	Channel_switch(video_power,video_power_open);
	Channel_switch(Relay_audio,Relay_audio_close);
	Channel_switch(Relay_video,Relay_video_up);	
	hCallSession=open_v_up;
	hCallSessionList.AddTail(hCallSession);
}
void CMedeiaLogicApp::OpenvaUpMonitorMedia()
{	
	//handshack_value=open_va_up_monitor;
	Channel_switch(audio_power,audio_power_open);
	Channel_switch(video_power,video_power_open);
	Channel_switch(Relay_audio,Relay_audio_open);
	Channel_switch(Relay_video,Relay_video_up); 
	hCallSession=open_va_up_monitor;
	hCallSessionList.AddTail(hCallSession);
}
void CMedeiaLogicApp::OpenvaUpCallMedia()
{	
	//handshack_value=open_va_up_call;
	Channel_switch(audio_power,audio_power_close);
	Channel_switch(video_power,video_power_open);
	Channel_switch(Relay_audio,Relay_audio_close);
	Channel_switch(Relay_video,Relay_video_up);
	hCallSession=open_va_up_call;
	hCallSessionList.AddTail(hCallSession);
}
void CMedeiaLogicApp::OpenvaDownCallMedia()
{	
	//handshack_value=open_va_down_call;
	Channel_switch(audio_power,audio_power_close);
	Channel_switch(video_power,video_power_open);
	Channel_switch(Relay_audio,Relay_audio_close);
	Channel_switch(Relay_video,Relay_video_down);
	hCallSession=open_va_down_call;
	hCallSessionList.AddTail(hCallSession);
}
void CMedeiaLogicApp::AddOpenAudioMedia()
{
	Channel_switch(audio_power,audio_power_open);
	Channel_switch(Relay_audio,Relay_audio_open);
}


#endif