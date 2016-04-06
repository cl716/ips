#include "stdafx.h"
#include "OperCom.h"
#include"systemmsg.h"
#include  "eth.h"
#include "RoomInf.h"
#include "database.h"
#include "mmsystem.h"
#include "systemmsg.h"

COperCom	m_Com;

COperCom::COperCom()
{
	m_bOpened = FALSE;
	m_hCom = NULL;
}

COperCom::~COperCom()
{
	CloseCom();
}

BOOL COperCom::OpenComm(wchar_t *wszComName,DWORD dwBAUD_RATE)
{
	DCB dcb;
	wchar_t *p_fname = wszComName;

	m_hCom = CreateFile(p_fname,                       //文件名   
						GENERIC_READ   |   GENERIC_WRITE,   //   允许读和写     
						0,                                                         //   独占方式   
						NULL,     
						OPEN_EXISTING,                                   //打开而不是创建   
						0,
						0   
						);   
    
	if(m_hCom == NULL )   
	{
		printf("Open comm port fail 1\n");
		return FALSE;
	}

	COMMTIMEOUTS TimeOuts;
	TimeOuts.ReadIntervalTimeout = 100;     
	TimeOuts.ReadTotalTimeoutMultiplier = 10;     
	TimeOuts.ReadTotalTimeoutConstant = 100;     
	TimeOuts.WriteTotalTimeoutMultiplier = 0;     
	TimeOuts.WriteTotalTimeoutConstant = 0;   
	SetCommTimeouts(m_hCom, &TimeOuts);

	dcb.DCBlength = sizeof( DCB );
	GetCommState( m_hCom, &dcb );
	dcb.BaudRate = dwBAUD_RATE;   //   波特率
	dcb.ByteSize = 8;   //   每个字符有8位   
	dcb.Parity = NOPARITY;   //无校验   
	dcb.StopBits = ONESTOPBIT;   //一个停止位   
	if( !SetCommState( m_hCom, &dcb ))
	{
		DWORD dwError = GetLastError();
		CloseHandle( m_hCom );
		printf("Open comm port fail 2 : dwError = %d\n", dwError);
		return FALSE;
	}

     m_bOpened = TRUE;
     return m_bOpened;
}

void COperCom::CloseCom()
{
	if(m_hCom && m_bOpened)
	{
		CloseHandle(m_hCom);
		m_hCom = NULL;
		m_bOpened = FALSE;
	}
}

DWORD COperCom::ReadData(BYTE *buffer, DWORD dwBytesRead)
{
	if( !m_bOpened || m_hCom == NULL )
	{
		_tprintf(L"COM was not open\n");
		return 0;
	}
	
	DWORD dwRead;
		
	if(0 == ReadFile( m_hCom, buffer, dwBytesRead, &dwRead,0))
		return -1;
	
	return dwRead;
}

DWORD COperCom::SendData(BYTE *buffer, DWORD dwBytesWritten)
{
	if( !m_bOpened || m_hCom == NULL ) 
	{
		_tprintf(L"COM was not open\n");
		return 0;
	}

	DWORD nWrite;
	if(0 == WriteFile(m_hCom,buffer,dwBytesWritten,&nWrite,NULL))
		return -1;
	//printf("=%x,%x,%x,%x,%x===\n",buffer[0],buffer[1],buffer[2],buffer[3],buffer[4]);
	return nWrite;
}


/*楼道数据帧格式：智能电源数据包*/
/*4bits起始标志(1010)+2bits组包位(11)+2bits数据包方向(00)+2bytes源地址+2bytes目的地址+1bytes数据长度+nbytes数据*/
/*10101100+xxxxxxxx xxxxxxxx+ 11111111 11111111+ 3+command+session+value*/ 
static DWORD OperCom_Thread(HANDLE lparam)
{
	Network_485_DATA OperComData;
	Network_Ipswich_TCP NetTCP485Pak;
	BYTE CheckTemp=0;
	while(1)
	{
		ZeroMemory(&OperComData,sizeof(OperComData));
		WaitForSingleObject(OperComDataThread_event,60000);
		{
			if(m_Com.ReadData((BYTE *)&OperComData,OperCom_PowerStatusDlenth))//MAX_485Lenth
			{
				CheckTemp=~(OperComData.DataId+OperComData.fromaddr+(OperComData.fromaddr>>8)&0xff+OperComData.toaddr+(OperComData.toaddr>>8)&0xff+
						  OperComData.length+OperComData.DATA[0]+OperComData.DATA[1]+OperComData.DATA[2])+1;
				if ((OperComData.CheckData==CheckTemp)&&(OperComData.DATA[OperCom_command]==CODET_POWERStatues))
				{
					NetTCP485Pak.nitcphead.fromaddr=(DWORD)OperComData.fromaddr;
					NetTCP485Pak.nitcphead.toaddr=(DWORD)OperComData.toaddr;
					NetTCP485Pak.nitcphead.headcommand=OperComData.DATA[OperCom_command];
					NetTCP485Pak.nitcphead.headmsgsession=OperComData.DATA[OperCom_session];
					NetTCP485Pak.nitcphead.length=sizeof(Network_Ipswich_TCP_HEAD)+OperComData.length;
					NetTCP485Pak.content[0]=OperComData.length;

					memcpy(&(NetTCP485Pak.content[1]),&(OperComData.DATA),OperComData.length);
					DBGMSG(DPINFO, "OperCom485 ReadData command:0x%x,session:0x%x,value:0x%x\r\n",NetTCP485Pak.nitcphead.headcommand,NetTCP485Pak.nitcphead.headmsgsession,OperComData.DATA[2]);
					if(ETHSend(NetTCP485Pak,NMC_ip,IPS_SERVER_PORT)==FALSE)	
					{
						DBGMSG(DPINFO, "OperCom_Thread OperCom485 fail\r\n");
					}
				}
			}
		}
		
	}
}

BOOL enable_OperCom_RunThread()
{
	HANDLE hThread1;

	m_Com.OpenComm(L"COM2:",115200);
	hThread1 =CreateThread(NULL, 0, OperCom_Thread,0, 0, NULL);
	if(hThread1  == NULL)
		printf("CreateThread fail,error is %d",GetLastError());
	CeSetThreadPriority(hThread1,219);
	CloseHandle(hThread1);
	return TRUE;
}