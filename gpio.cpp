#include "PlatformConfig.h"
#include "RoomInf.h"
#include "database.h"
#include "mmsystem.h"
#include "systemmsg.h"
#include "gpio.h"
#include "eth.h"

HANDLE GIO[GIO_num];/*GIO[0]=hAudio_VV_con,	GIO[1]=hVideo_V_con,
				 GIO[2]=hRelay_K1,		GIO[3]=hRelay_K2,		GIO[4]=hRelay_K3,		GIO[5]=hRelay_K4,	GIO[6]=hRelay_K5,
				 GIO[7]=hRunLED,		GIO[8]=hCAN_RunGLEDT,	GIO[9]=hCAN_RunRLEDR,
				 GIO[10]=hSW1,			GIO[11]=hSW2,			GIO[12]=hSW3,			GIO[13]=hSW4,
				 GIO[14]=hSW5,			GIO[15]=hSW6,			GIO[16]=hSW7,			GIO[17]=hSW8,
				 GIO[18]=h485_DE,		GIO[19]=h485_RunLEDT,	GIO[20]=h485_RunLEDR
				 GIO[21]=hRelay_K0		GIO[22]=WatchDog706_WDI*/



BOOL InitGpio(HANDLE *hgpio,GPIO_CFG gpio_cfg)
{
	BOOL ret = TRUE;
	DWORD bytesreturned;

	*hgpio = CreateFile(_T("PIO1:"),GENERIC_READ|GENERIC_WRITE,0,NULL,OPEN_EXISTING,0,NULL);
	if(*hgpio == INVALID_HANDLE_VALUE)
	{
		printf("CreateFile PIO1 fail,error is %d\r\n",GetLastError());
		ret = FALSE;
		goto end;
	}
	
	if(!DeviceIoControl(*hgpio,IOCTRL_GPIO_INIT,(LPVOID)&gpio_cfg,sizeof(GPIO_CFG),NULL,0,&bytesreturned,NULL))
	{
		printf("IOCTRL_GPIO_INIT  fail error is %d\r\n",GetLastError());
		ret = FALSE;
		goto end;
	}
end:
	return ret;
}

BOOL SetGpioVal (HANDLE hgpio,BOOL value)
{
	//GPIO_CFG gpio_cfg;
	DWORD dwRet;

	if(!DeviceIoControl(hgpio, IOCTRL_SET_GPIO_VALUE, (LPVOID)&value, sizeof(value), NULL, 0, &dwRet, NULL))
	{
		DBGMSG(DPERROR, "IOCTRL_SET_GPIO_VALUE   fail error is %d\r\n", GetLastError());//fail err 31 ??
		return FALSE;
	}
	return TRUE;
}

BOOL GetGpioVal(HANDLE hgpio,BYTE*data)
{
	DWORD bytesreturned;
	if(!DeviceIoControl(hgpio,IOCTRL_GET_GPIO_VALUE,(LPVOID)data,1,NULL,0,&bytesreturned,NULL))
	{
		printf("IOCTRL_SET_GPIO_VALUE  fail error is %d\r\n",GetLastError());
		return FALSE;
	}
	else
		return TRUE;
}

BOOL InitAllGIO(void)
{
	GPIO_CFG gpio_cfg[GIO_num];
	BOOL GIO_initValue[GIO_num]={0,0,0,0,0,0,0,1,1,1,0,0,0,0,0,0,0,0,0,1,1,0,0};
	for(BYTE n=0;n<GIO_num;n++)
	{
		memset(&gpio_cfg[n],0,sizeof(GPIO_CFG));
	}
	gpio_cfg[0].gpio_num = Audio_VV_con;//L
	gpio_cfg[0].gpio_cfg = GIO_output;
	gpio_cfg[0].gpio_pull=PULL_DOWN;
	gpio_cfg[1].gpio_num = Video_V_con;	//L
	gpio_cfg[1].gpio_cfg = GIO_output;
	gpio_cfg[1].gpio_pull=PULL_DOWN;
	gpio_cfg[2].gpio_num = Relay_K1;	//L
	gpio_cfg[2].gpio_cfg = GIO_output;
	gpio_cfg[2].gpio_pull=PULL_DISABLE;
	gpio_cfg[3].gpio_num = Relay_K2;	//L
	gpio_cfg[3].gpio_cfg = GIO_output;
	gpio_cfg[3].gpio_pull=PULL_DISABLE;
	gpio_cfg[4].gpio_num = Relay_K3;	//L
	gpio_cfg[4].gpio_cfg = GIO_output;
	gpio_cfg[4].gpio_pull=PULL_DISABLE;
	gpio_cfg[5].gpio_num = Relay_K4;	//L
	gpio_cfg[5].gpio_cfg = GIO_output;
	gpio_cfg[5].gpio_pull=PULL_DISABLE;
	gpio_cfg[6].gpio_num = Relay_K5;	//L
	gpio_cfg[6].gpio_cfg = GIO_output;
	gpio_cfg[6].gpio_pull=PULL_DISABLE;
	gpio_cfg[7].gpio_num = RunLED;		//H
	gpio_cfg[7].gpio_cfg = GIO_output;
	gpio_cfg[7].gpio_pull=PULL_UP;
	gpio_cfg[8].gpio_num = CAN_RunGLEDT;//H
	gpio_cfg[8].gpio_cfg = GIO_output;
	gpio_cfg[8].gpio_pull=PULL_UP;
	gpio_cfg[9].gpio_num = CAN_RunRLEDR;//H
	gpio_cfg[9].gpio_cfg = GIO_output;
	gpio_cfg[9].gpio_pull=PULL_UP;
	gpio_cfg[10].gpio_num = SW1;
	gpio_cfg[10].gpio_cfg = GIO_input;
	gpio_cfg[10].gpio_pull=PULL_DISABLE;;
	gpio_cfg[11].gpio_num = SW2;
	gpio_cfg[11].gpio_cfg = GIO_input;
	gpio_cfg[11].gpio_pull=PULL_DISABLE;
	gpio_cfg[12].gpio_num = SW3;
	gpio_cfg[12].gpio_cfg = GIO_input;
	gpio_cfg[12].gpio_pull=PULL_DISABLE;
	gpio_cfg[13].gpio_num = SW4;
	gpio_cfg[13].gpio_cfg = GIO_input;
	gpio_cfg[13].gpio_pull=PULL_DISABLE;
	gpio_cfg[14].gpio_num = SW5;
	gpio_cfg[14].gpio_cfg = GIO_input;
	gpio_cfg[14].gpio_pull=PULL_DISABLE;
	gpio_cfg[15].gpio_num = SW6;
	gpio_cfg[15].gpio_cfg = GIO_input;
	gpio_cfg[15].gpio_pull=PULL_DISABLE;
	gpio_cfg[16].gpio_num = SW7;
	gpio_cfg[16].gpio_cfg = GIO_input;
	gpio_cfg[16].gpio_pull=PULL_DISABLE;
	gpio_cfg[17].gpio_num = SW8;
	gpio_cfg[17].gpio_cfg = GIO_input;
	gpio_cfg[17].gpio_pull=PULL_DISABLE;
	gpio_cfg[18].gpio_num = RS485_DE;//L
	gpio_cfg[18].gpio_cfg = GIO_output;
	gpio_cfg[18].gpio_pull=PULL_DOWN;
	gpio_cfg[19].gpio_num = RS485_RunLEDT;//H
	gpio_cfg[19].gpio_cfg = GIO_output;
	gpio_cfg[19].gpio_pull=PULL_UP;
	gpio_cfg[20].gpio_num = RS485_RunLEDR;//H
	gpio_cfg[20].gpio_cfg = GIO_output;
	gpio_cfg[20].gpio_pull=PULL_UP;
	gpio_cfg[21].gpio_num = Relay_K0;//L--151108：视频后加通道0
	gpio_cfg[21].gpio_cfg = GIO_output;
	gpio_cfg[21].gpio_pull=PULL_DISABLE;
	gpio_cfg[22].gpio_num = WatchDog706_WDI;//L--151226：看门狗WDI
	gpio_cfg[22].gpio_cfg = GIO_output;
	gpio_cfg[22].gpio_pull=PULL_DISABLE;
	for(BYTE i=0;i<GIO_num;i++)
	{
		gpio_cfg[i].gpio_driver=0;
		if(!InitGpio(&GIO[i],gpio_cfg[i]))
		{
			//system("chcp 936");
			DBGMSG(DPERROR, "InitGpio,error is %d\r\n",GetLastError());
			return FALSE;
		}
		if(!SetGpioVal(GIO[i],GIO_initValue[i]))
		{
			DBGMSG(DPERROR, "SetGpioVal fail,error is %d\r\n",GetLastError());
			return FALSE;
		} 
	}
	DBGMSG(DPINFO, "InitGpio OK\r\n");
	return TRUE;
}

BYTE GetSelfAddr(void)
{
	BYTE SWBit_Value;
	BYTE SWByte_Value=0;
	for (BYTE i=0;i<8;i++)
	{
		GetGpioVal(GIO[10+i],&SWBit_Value);
		SWByte_Value=SWByte_Value+(SWBit_Value<<i);
	}
	SWByte_Value=~SWByte_Value;
	return SWByte_Value;

}