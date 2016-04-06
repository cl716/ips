#include "stdafx.h"
#include <windows.h>
#include <commctrl.h>

#define CAN_IOCTL_SETBANDRATE	0x1234567a

typedef struct
{
	UINT32 Id;
	BYTE RemoteFlag;
	BYTE ExternFlag;
	BYTE datalen;	
	BYTE data[8];	
	BYTE reserved;
}CAN_INFO;

typedef enum{
	BandRate_100kbps,
	BandRate_125kbps,
	BandRate_250kbps,
	BandRate_500kbps,
	BandRate_1Mbps
}CanBandRate;


static HANDLE hcan;
DWORD Can_TestReadThread()
{
	HANDLE hwaitevent = CreateEvent(NULL,FALSE,FALSE,NULL);
	CAN_INFO readinfo;
	
	DWORD len;
	while(1)
	{
		//DeviceIoControl(hfile,0x12345678,NULL,0,NULL,0,NULL,NULL);
		//WaitForSingleObject(hwaitevent,1000);

		memset(&readinfo,0,sizeof(CAN_INFO));
		printf("ReadFile\r\n");

		if(ReadFile(hcan,&readinfo,sizeof(CAN_INFO),&len,NULL)) 
		{
			printf("can id is %x \r\n",readinfo.Id);
			for(int i = 0; i< readinfo.datalen;i++)
				printf("%02x ",readinfo.data[i]);
			printf("\r\n");
		}
		else
			printf("ReadFile fail,error is %d\r\n",GetLastError());
		
	}

	CloseHandle(hcan);
	CloseHandle(hwaitevent);
	return TRUE;
}

DWORD Can_TestWriteThread()
{
	HANDLE hwaitevent = CreateEvent(NULL,FALSE,FALSE,NULL);
	CAN_INFO writeinfo[5];

	DWORD len;
	while(1)
	{
		//DeviceIoControl(hfile,0x12345678,NULL,0,NULL,0,NULL,NULL);
		WaitForSingleObject(hwaitevent,1000);

		for(int i = 0; i < 5;i++)
		{
			writeinfo[i].Id = i;
			writeinfo[i].datalen = 8;
			memset(&writeinfo[i].data,i,8);
		}		
		if(WriteFile(hcan,writeinfo,5*sizeof(CAN_INFO),&len,NULL))
			printf("write ok\r\n");
	}
	//CloseHandle(hfile);
	CloseHandle(hwaitevent);
	return TRUE;
}

BOOL Can_Test()
{
	HANDLE hThread;

	CanBandRate bandrate = BandRate_250kbps;
	hcan =  CreateFile(_T("CAN1:"),GENERIC_READ|GENERIC_WRITE,0,NULL,OPEN_EXISTING,0,NULL);
	if(hcan == INVALID_HANDLE_VALUE)
	{
		printf("OPEN CAN1 device fail,error is %d \r\n",GetLastError());
		return FALSE;
	}

	DeviceIoControl(hcan,CAN_IOCTL_SETBANDRATE,&bandrate,1,NULL,0,NULL,NULL);

	hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)Can_TestReadThread, NULL, 0, NULL);
	if(hThread  == NULL)
		printf("CreateThread fail,error is %d",GetLastError());
	CloseHandle(hThread);
	
	hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)Can_TestWriteThread, NULL, 0, NULL);
	if(hThread  == NULL)
		printf("CreateThread fail,error is %d",GetLastError());
	CloseHandle(hThread);
	return TRUE;
}