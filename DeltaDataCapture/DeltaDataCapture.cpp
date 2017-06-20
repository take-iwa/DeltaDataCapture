/*========================================================================*/
/*== DeltaDataCaptute.cpp												==*/
/*==  Author:Hirofumi Ooiwa												==*/
/*==  Date:2017.2/22	���e												==*/
/*========================================================================*/
/*== <���e>																==*/
/*== SI-40LA2�ɐڑ����A�\�ߎw�肵���t�@�C���Ɏ�M�f�[�^�������݂܂��B			==*/
/*== �Ώۃt�@�C��������Ɠ����t�H���_�Ƀr�[���̋L�^�A���Y���|�[�g�A				==*/
/*== �p�^�[���f�[�^���ꂼ���ۑ�����t�H���_���쐬���܂��B						==*/
/*== �f�[�^��������Ǝw�莞�Ԉȏ�ʐM���r�؂��܂�							==*/
/*== ����t�@�C���ɕۑ����܂��B												==*/
/*== <����菇>															==*/
/*== 1.[�Q��]���N���b�N���A�ۑ���t�@�C�����w�肵�܂��B		     			==*/
/*== �@���̃t�@�C����txt�t�@�C�����w�肷��B				     				==*/
/*== 2.�ڑ����鍆�@��I�����܂��B											==*/
/*== �@IP�A�h���X�͎����Ő؂�ւ��A�|�[�g�͌Œ�ł��B						==*/
/*== 3.[�ڑ�]���N���b�N���ASI-40LA2�ɐڑ����܂��B							==*/
/*== 4.SI-40LA2����̃f�[�^����M����Ǝw�肵���t�@�C���ɏ����܂�				==*/
/*==   �J�E���^�[�Ɏ�M�o�C�g�����\������܂��B								==*/
/*==   ��M���I�����w�莞�Ԉȏ�ʐM���r�₦��ƕۑ��t�@�C����ؑւ��܂�			==*/
/*==   �Ώۃt�H���_��txt�t�@�C�����ł��܂��̂ł������̃e�L�X�g�G�f�B�^��			==*/
/*==   �m�F���Ă��������B													==*/
/*== 5.[�ؒf]���N���b�N���ASI-40LA2�Ɛؒf���܂��B							==*/
/*== <����>																==*/
/*== 	���j�b�g�Ƃ̒ʐM�͓����^�ł��B										==*/
/*========================================================================*/
#define STRICT
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

#include "DeltaDataCapture.h"
#include "resource.h"

#include <string.h>
#include <direct.h>
#include <process.h>
#include <iostream>
#include <time.h>
#include <WinSock.h>

#include <shlwapi.h>
#pragma comment(lib, "WSock32.lib")

#pragma comment(lib, "shlwapi.lib")

/*--------------------------------------------------------------------------*/
/* Global variable                                                          */
/*--------------------------------------------------------------------------*/
HINSTANCE	g_hInstance;			// Save hInstance
HANDLE		g_hTCPsockThread;		// Handle of Thread
FILE		*g_pFile;				// File pointer
TCHAR		g_FldPath[MAX_PATH];	// Folder path of save data
TCHAR		g_FileName[MAX_PATH];	// File path of save data
LPSTR		g_ReportFldPath;		// Folder path of save Beam recodes and Day Reports
LPSTR		g_PtnDtFldPath;			// Folder path of save Pattern data
BOOL		g_fConnected;			// Connection status (TRUE = Connect)
DWORD		g_dwCounter;			// Receive data counter
WSADATA		g_wsaData;				// WSADATA data structure
SOCKET		g_sock;					// Soket Descriptor
SOCKADDR_IN g_sockaddr;				// address family
BOOL		g_fDropdown;			// Show Dropdown once

BOOL SwitchFiles(HWND hDlg);

/*--------------------------------------------------------------------------*/
/* Write data to a file														*/
/*--------------------------------------------------------------------------*/

BOOL WriteData( const void *pBuffer, DWORD dwSize )
{
	int nLength;

	if( g_pFile == NULL )
	{
		return FALSE;
	}

	if( 0 == ( nLength = fwrite( pBuffer, sizeof(BYTE), dwSize, g_pFile )) )
	{
		return FALSE;
	}

    return TRUE;
}

/*--------------------------------------------------------------------------*/
/* Read data form the LAN port												*/
/*--------------------------------------------------------------------------*/

int ReadTCPSockData( SOCKET sock, char *pBuff, int nBuffLength )
{
	int nLength;

	nLength = recv( sock, pBuff, nBuffLength, 0 );
	if( nLength == SOCKET_ERROR || nLength == 0 )
	{

		nLength = 0;
	}

	return nLength;

}

/*--------------------------------------------------------------------------*/
/* Thread process															*/
/*--------------------------------------------------------------------------*/

unsigned __stdcall TCPsockThreadProc( LPVOID hDlg )
{
	int nLength;
	static char Buff[ MAXBLOCK ];
	bool bOnTimer = false;

	/* create file */
	errno_t err;
	if (err = (fopen_s(&g_pFile, (const char*)g_FileName, "wb+") != 0))
	{
		return  FALSE;
	}

	while ( g_fConnected )
	{
		if (bOnTimer == false)
		{
			// 5�b�ԃf�[�^�������Ă��Ȃ��ꍇ�A��U�ڑ��f
			SetTimer((HWND)hDlg, TM_TIMEOUT, 5000, NULL);
			bOnTimer = true;
		}

		if( 0 != (nLength = ReadTCPSockData( g_sock, Buff, sizeof(Buff) )) )
		{
			if( !WriteData( Buff, (DWORD)nLength ))
			{
				PostMessage( (HWND)hDlg, WM_USER_MSG, LOWORD(UMSG_WRITEWARNING), 0 );
			}
			g_dwCounter = g_dwCounter + nLength;

			// �ڑ��f�^�C�}�[���Z�b�g
			if (bOnTimer == true)
			{
				KillTimer((HWND)hDlg, TM_TIMEOUT);
				bOnTimer = false;
			}
		}
		else
		{
			if( g_fConnected )
			{	
				PostMessage( (HWND)hDlg, WM_USER_MSG, LOWORD(UMSG_RECEIVEWARNING), 0 );
			}
		}
	}

	/* close a file */
	if (g_pFile != 0)
	{
		fclose(g_pFile);
	}
	g_pFile = NULL;

	/* get rid of thread handle */
	CloseHandle( g_hTCPsockThread );
	g_hTCPsockThread = NULL;

	return TRUE;
}

/*--------------------------------------------------------------------------*/
/* Initialize the Windows socket											*/
/*--------------------------------------------------------------------------*/
BOOL SockInitialize( HWND hDlg, WSADATA wsaData )
{
	WORD wVersionRequested;

	wVersionRequested = MAKEWORD(1,1);

	if( WSAStartup( wVersionRequested, &wsaData ) != 0 ) 
	{
		MessageBox( hDlg, "Failed to initialize WinSock!", "Error", MB_OK|MB_ICONSTOP );
		return FALSE;
	}
	
	return TRUE;
}

/*--------------------------------------------------------------------------*/
/* Disconnect of the TCP socket												*/
/*--------------------------------------------------------------------------*/

BOOL TCPSockDisconnect( HWND hDlg )
{
	shutdown( g_sock, 2 );		/* disables sends or receives on a socket */
	closesocket( g_sock );		/* closes an existing socket */
	g_sock = INVALID_SOCKET;
	return TRUE;
}

/*--------------------------------------------------------------------------*/
/* Connect of the TCP socket												*/
/*--------------------------------------------------------------------------*/

BOOL TCPSockConnect( HWND hDlg )
{
	int nPort;
	char szPort[10];
	char szIPaddr[16];
	char *pIPaddr;

	GetDlgItemText( hDlg, IDC_IPADDRESS, (LPTSTR)szIPaddr, sizeof(szIPaddr) );
	pIPaddr = &szIPaddr[0];
	GetDlgItemText( hDlg, IDC_PORTNUMBER, (LPTSTR)szPort, sizeof(szPort) );
	nPort = atoi( szPort );
	
	if( szIPaddr[0] == '\0' )
	{
		MessageBox( hDlg, "IPaddress is not input!", "Error",MB_OK);
		return FALSE;
	}

	if( nPort == 0 )
	{
		MessageBox( hDlg, "Port number is not input!", "Error",MB_OK);
		return FALSE;
	}

    /* set the IP address and the port number of the server. */
	memset( g_sockaddr.sin_zero, 0, sizeof(g_sockaddr.sin_zero) );  /* initialize a structure */
    g_sockaddr.sin_family		= AF_INET;                          /* Internet */
    g_sockaddr.sin_addr.s_addr  = inet_addr(pIPaddr);				/* IP address */
	g_sockaddr.sin_port			= htons(nPort);						/* Port number */
	

    g_sock = INVALID_SOCKET;
	/* Create the TCP socket */
    g_sock = socket( AF_INET, SOCK_STREAM, 0 );
    if( g_sock == INVALID_SOCKET )
	{
		MessageBox( hDlg, "Failed to create socket!", "Error",MB_ICONEXCLAMATION );
        return FALSE;
    }

    /* establishes a connection to the TCP socket */
    if( connect( g_sock, (LPSOCKADDR)&g_sockaddr, sizeof(g_sockaddr)) == SOCKET_ERROR )
	{
		MessageBox( hDlg, "Connect failure", "Error", MB_ICONEXCLAMATION | MB_OK);
		TCPSockDisconnect( hDlg );
		return FALSE;
    }

    return TRUE;
}

/*--------------------------------------------------------------------------*/
/* Counter process							                                */
/*--------------------------------------------------------------------------*/

void CounterDisplay( HWND hDlg )
{
	char szText[10];

	/* display of counter */
	wsprintf(szText, (LPCSTR)"%d",g_dwCounter);
	SetDlgItemText( hDlg, IDC_COUNTER, szText );
}

/*--------------------------------------------------------------------------*/
/* Connection process														*/
/*--------------------------------------------------------------------------*/

BOOL OpenConnection( HWND hDlg )
{
	BOOL fResult;
	unsigned int ThreadID;

	/* file name check */
	if( g_FileName == NULL )
	{
		return FALSE;
	}

	fResult = TCPSockConnect( hDlg );

	if (fResult)
	{
		// ����ڑ��m�F
		SetTimer((HWND)hDlg, TM_RECONECT, (3*60*60*1000), NULL);

		/* set the connection status (Connect) */
		g_fConnected = TRUE;

		/* create a secondary thread */
		if (NULL == ( g_hTCPsockThread = (HANDLE)_beginthreadex(NULL,
															0,
															&TCPsockThreadProc,
															(LPVOID)hDlg,
															0,
															&ThreadID)))
		{
			/* set the connection status (Disconnect) */
			g_fConnected = FALSE ;
		
			/* close a file */
			fclose( g_pFile );
			g_pFile = NULL;

			fResult = FALSE;
		}
		else
		{

			/* change the control status */
			EnableWindow( GetDlgItem( hDlg, IDC_CHANGE ), FALSE );
			EnableWindow( GetDlgItem( hDlg, IDC_SELECT ), FALSE );
			EnableWindow( GetDlgItem( hDlg, IDC_IPADDRESS ), FALSE );
			EnableWindow( GetDlgItem( hDlg, IDC_PORTNUMBER ), FALSE );
			EnableWindow( GetDlgItem( hDlg, IDC_CONNECT), FALSE );
			EnableWindow( GetDlgItem( hDlg, IDC_DISCONNECT ), TRUE );
	
			/* clear the display counter */
			g_dwCounter = 0;
			CounterDisplay( hDlg );
			/* start the update timer for counter display (0.5s intervals) */
			SetTimer( hDlg , TM_COUNTER, 500 , NULL );
		}
	}
	else
	{
		/* set the connection status (Disconnect) */
		g_fConnected = FALSE;

		/* close a file */
		fclose( g_pFile );
		g_pFile = NULL;
	}

	return fResult;

}

/*--------------------------------------------------------------------------*/
/* Disconnection process			                                        */
/*--------------------------------------------------------------------------*/

BOOL CloseConnection( HWND hDlg )
{
	DWORD dwExCode;
	
	/* set the connection status (Disconnect) */
	g_fConnected = FALSE;

	TCPSockDisconnect( hDlg );
	
	/* retrieves the termination status of the thread */
	GetExitCodeThread( g_hTCPsockThread , &dwExCode );
	if (dwExCode == STILL_ACTIVE)
	{
		WaitForSingleObject( g_hTCPsockThread,INFINITE ); /* wait until the thread is over */
	}

	if( g_pFile != NULL )
	{
		/* close a file */
		fclose( g_pFile );
		g_pFile = NULL;
	}

	/* change the control status */
	EnableWindow( GetDlgItem( hDlg, IDC_CHANGE ), TRUE );
	EnableWindow( GetDlgItem( hDlg, IDC_SELECT ), TRUE );
	EnableWindow( GetDlgItem( hDlg, IDC_IPADDRESS ), TRUE );
	EnableWindow( GetDlgItem( hDlg, IDC_PORTNUMBER ), TRUE );
	EnableWindow( GetDlgItem( hDlg, IDC_CONNECT ), TRUE );
	EnableWindow( GetDlgItem( hDlg, IDC_DISCONNECT ), FALSE );

	/* stop the update timer for counter display */
	KillTimer( hDlg , TM_COUNTER);
	CounterDisplay( hDlg );
	
	// ����ڑ��m�F�^�C�}�[OFF
	KillTimer( hDlg, TM_RECONECT);

	return TRUE;

}

/*--------------------------------------------------------------------------*/
/* Save As dialog box														*/
/*--------------------------------------------------------------------------*/

BOOL SaveFilesDlg(HWND hDlg)
{
    OPENFILENAME ofn;
    memset( &ofn, 0, sizeof(OPENFILENAME) );
    
	ofn.lStructSize		= sizeof( OPENFILENAME );
    ofn.hwndOwner		= hDlg;
    ofn.lpstrFilter		= "*.txt";
    ofn.lpstrFile		= g_FileName;
    ofn.nMaxFile		= MAX_PATH;
    ofn.Flags			= OFN_FILEMUSTEXIST | OFN_OVERWRITEPROMPT| OFN_HIDEREADONLY;
    ofn.lpstrDefExt		= "txt";
    ofn.nMaxFileTitle	= 64;

	if( GetSaveFileName( &ofn ) == 0 ) 
	{
		return FALSE;
	}
	else
	{
        InvalidateRect( hDlg, NULL, TRUE );
    }

	// �t�H���_�p�X�擾
	TCHAR cFldPath1[MAX_PATH];
	TCHAR cFldPath2[MAX_PATH];
	strncpy_s(cFldPath1, sizeof(cFldPath1), g_FileName, _TRUNCATE);
	strncpy_s(cFldPath2, sizeof(cFldPath2), g_FileName, _TRUNCATE);

	PathRemoveFileSpec(cFldPath1);
	PathRemoveFileSpec(cFldPath2);

	/* display save Folder path the on the textbox */
	SetDlgItemText( hDlg,IDC_PATH, g_FileName);
	
	/* enable connect button */
	EnableWindow( GetDlgItem( hDlg, IDC_CONNECT ), TRUE );
    
	// �ۑ���t�H���_�쐬
	// �r�[���̋L�^+���Y���|�[�g
	TCHAR cBeamFldName[] = TEXT("/Report");
	g_ReportFldPath = (LPSTR)lstrcat(cFldPath1, cBeamFldName);
	if (PathFileExists(g_ReportFldPath) == false)
	{
		_mkdir(g_ReportFldPath);
	}

	// �������ݐ}
	TCHAR cPtnDtFldName[] = TEXT("/PatternData");
	g_PtnDtFldPath = (LPSTR)lstrcat(cFldPath2, cPtnDtFldName);
	if (PathFileExists(g_PtnDtFldPath) == false)
	{
		_mkdir(g_PtnDtFldPath);
	}

	return TRUE;
}

/*--------------------------------------------------------------------------*/
/* Change Select Unit Number								                */
/*--------------------------------------------------------------------------*/

BOOL ChangeSelectUnit(HWND hDlg, WPARAM wParam)
{
	char szUint[16];

	// �E�C���h�E�n���h���擾
	HWND hwndComb = GetDlgItem(hDlg, IDC_SELECT);

	if( HIWORD(wParam) == CBN_DROPDOWN )
	{
		if (!g_fDropdown)
		{
			// �\�����郊�X�g
			SendMessage(hwndComb, CB_INSERTSTRING, 0, (LPARAM)"1���@");
			SendMessage(hwndComb, CB_INSERTSTRING, 1, (LPARAM)"2���@");
			SendMessage(hwndComb, CB_INSERTSTRING, 2, (LPARAM)"5���@");
			SendMessage(hwndComb, CB_INSERTSTRING, 3, (LPARAM)"6���@");
			g_fDropdown = TRUE;
		}
	}
	else if (HIWORD(wParam) == CBN_SELCHANGE)
	{
		// ���@�I��ύX
		// ���@��I���������AIP�A�h���X�ƃ|�[�g�������Őݒ�
		GetDlgItemText(hDlg, IDC_SELECT, (LPTSTR)szUint, sizeof(szUint));
		switch(SendMessage(hwndComb, CB_GETCURSEL, 0, 0 ))
		{
			case 0:		// 1���@
				SetDlgItemText(hDlg, IDC_IPADDRESS, (LPCSTR)"192.168.122.21");
				break;
			case 1:		// 2���@
				SetDlgItemText(hDlg, IDC_IPADDRESS, (LPCSTR)"192.168.122.22");
				break;
			case 2:		// 5���@
				SetDlgItemText(hDlg, IDC_IPADDRESS, (LPCSTR)"192.168.122.25");
				break;
			case 3:		// 6���@
				SetDlgItemText(hDlg, IDC_IPADDRESS, (LPCSTR)"192.168.122.26");
				break;
			default:
				// none
				break;
		}
	}
	else
	{
		// none
	}

	return TRUE;
}

/*--------------------------------------------------------------------------*/
/* Categorize + Copy File									                */
/*--------------------------------------------------------------------------*/

BOOL SwitchFiles(HWND hDlg)
{
	int i = 0;
	bool bIsBeamRec = false;
	bool bIsDayRepo = false;
	char cKeyWordBeam[] = "&%`$N5-O?";			// �r�[���̋L�^�̎��ʎq
	char cKeyWordBeam2[] = "&%S!<%`$N5-O?";		// �r�[���̋L�^�̎��ʎq�Q
	char cKeyWordRepo[] = "&%+%&%s%?";			// ���Y���|�[�g�̎��ʎq
	char cKeyPaperSend[] = ",";					// ������M��
	char str[0xFF] = {0};						// �����p

	// Categorize(���Y���|�[�g or �r�[���̋L�^ or �p�^�[���f�[�^ or ������M��)
	errno_t err;
	if (g_FileName != NULL)
	{
		if (err = (fopen_s(&g_pFile, (const char*)g_FileName, "rb") != 0))
		{
			MessageBox(hDlg, "Failed to open file.", "Error", MB_ICONEXCLAMATION | MB_OK);
			return  FALSE;
		}

		// �e�����񎯕ʎq�ɂĕ���(10��܂Ŋm�F)
		for (i; i < 10; i++)
		{
			fgets(str, sizeof(str), g_pFile);
			//�r�[���̋L�^�̎��ʎq
			if (NULL != strstr(str, cKeyWordBeam))
			{
				bIsBeamRec = true;
				break;
			}
			//�r�[���̋L�^�̎��ʎq�Q
			else if (NULL != strstr(str, cKeyWordBeam2))
			{
				bIsBeamRec = true;
				break;
			}
			//���Y���|�[�g�̎��ʎq
			else if (NULL != strstr(str, cKeyWordRepo))
			{
				bIsDayRepo = true;
				break;
			}
			//������M�������̏ꍇ�A�j��
			else if ((i < 1) && (NULL != strstr(str, cKeyPaperSend)))
			{
				// ������M��
				return FALSE;
			}
		}

		fclose(g_pFile);
	}
	g_pFile = NULL;

	// ���Ԏ擾
	time_t timer;
	struct tm* tm;
	TCHAR datetime[80];
	timer = time(NULL);
	tm = localtime(&timer);

	// �t�H���_�p�X�擾
	strncpy_s(g_FldPath, sizeof(g_FldPath), g_FileName, _TRUNCATE);
	PathRemoveFileSpec(g_FldPath);

	// Copy
	// �r�[���̋L�^�Ɛ��Y���|�[�g�͓����t�H���_�ɕۑ�
	if (bIsBeamRec)
	{
		// �r�[���̋L�^
		strftime((char*)datetime, 80, "\\Report\\BeamRecode_%Y%m%d%H%M%S.txt", tm);
		LPWSTR sBeamFilePath = (LPWSTR)lstrcat(g_FldPath, datetime);
		CopyFile(g_FileName, (LPCSTR)sBeamFilePath, false);
	}
	else if (bIsDayRepo)
	{
		// ���Y���|�[�g
		strftime((char*)datetime, 80, "\\Report\\DayReport_%Y%m%d%H%M%S.txt", tm);
		LPWSTR sDayRepFilePath = (LPWSTR)lstrcat(g_FldPath, datetime);
		CopyFile(g_FileName, (LPCSTR)sDayRepFilePath, false);
	}
	else
	{
		// �������ݐ}�Ȃ�
		strftime((char*)datetime, 80, "\\etc\\PatternData_%Y-%m-%d-%H%M%S.txt", tm);
		LPWSTR sPtnDtFilePath = (LPWSTR)lstrcat(g_FldPath, datetime);
		CopyFile(g_FileName, (LPCSTR)sPtnDtFilePath, false);
	}

	return TRUE;
}

/*--------------------------------------------------------------------------*/
/* Process of WM_TIMER message								                */
/*--------------------------------------------------------------------------*/

BOOL WmTimer(HWND hDlg, WPARAM wParam, LPARAM lParam)
{
	// �f�[�^��M
	if (LOWORD(wParam) == TM_COUNTER)
	{
		// ��M�f�[�^���X�V
		CounterDisplay( hDlg );
	}
	// �^�C���A�E�g
	else if (LOWORD(wParam) == TM_TIMEOUT)
	{
		// �f�[�^��M���Ă����ꍇ
		if (0 != g_dwCounter)
		{
			// �ڑ��f
			CloseConnection(hDlg);

			// �t�@�C�������[�h
			SwitchFiles(hDlg);

			// �Đڑ�
			if (OpenConnection(hDlg)) {
				// �ł��Ȃ���΂P�b���ƂɃ��g���C
				SetTimer((HWND)hDlg, TM_TIMEOUT, 1000, NULL);
			}
		}
		// �f�[�^��M�I��
		else
		{
			// �^�C���A�E�g�^�C�}�[�Đݒ�@5�b�ԃf�[�^�������Ă��Ȃ��ꍇ�A��U�ڑ��f
			SetTimer((HWND)hDlg, TM_TIMEOUT, 5000, NULL);
		}
	}
	else // TM_RECONECT ����ڑ��m�F
	{
		if ((g_fConnected == TRUE) && (g_dwCounter == 0))
		{
			// �ڑ��f
			CloseConnection(hDlg);

			// �Đڑ�
			if (OpenConnection(hDlg)) {
				// �ł��Ȃ���΂P�b���ƂɃ��g���C
				SetTimer((HWND)hDlg, TM_TIMEOUT, 1000, NULL);
			}
		}
	}
	return TRUE;
}

/*--------------------------------------------------------------------------*/
/* Process of WM_INITDIALOG message (Initialize dialog box)                 */
/*--------------------------------------------------------------------------*/

BOOL WmInitDialog( HWND hDlg, WPARAM wParam, LPARAM lParam )
{
	HICON hIcon;

	// �����l�ݒ�
	SetDlgItemText(hDlg, IDC_PATH, (LPCSTR)"C:\\Users");
	SetDlgItemText(hDlg, IDC_SELECT, (LPCSTR)"1���@");
	SetDlgItemText(hDlg, IDC_IPADDRESS, (LPCSTR)"192.168.122.21");
	SetDlgItemText(hDlg, IDC_PORTNUMBER, (LPCSTR)"10001");

	/* disable connect and disconnect button */
	EnableWindow( GetDlgItem( hDlg, IDC_CONNECT ), FALSE );
	EnableWindow( GetDlgItem( hDlg, IDC_DISCONNECT ), FALSE );

	/* Initialize global variable */
	g_pFile = NULL;
	g_hTCPsockThread = NULL;
	g_fDropdown = FALSE;

	SockInitialize( hDlg, g_wsaData );

	// �A�C�R���̃Z�b�g
	hIcon = (HICON)LoadImage(g_hInstance, MAKEINTRESOURCE(IDI_ICON1), IMAGE_ICON, 16, 16, 0);
	SendMessage(hDlg, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);

	return TRUE;
}

/*--------------------------------------------------------------------------*/
/* Process of WM_CLOSE message								                */
/*--------------------------------------------------------------------------*/

BOOL WmCloseMain( HWND hDlg )
{	
	if( g_fConnected )
	{
		CloseConnection( hDlg );
	}

	WSACleanup();

    EndDialog( hDlg, 0 );

    return TRUE;
}

/*--------------------------------------------------------------------------*/
/* Process of WM_USER_MSG message							                */
/*--------------------------------------------------------------------------*/

BOOL WmUserMsg( HWND hDlg, WPARAM wParam, LPARAM lParam )
{
	switch( LOWORD( wParam ) )
	{
	case UMSG_WRITEWARNING:
		MessageBox( hDlg, (LPCSTR)"Failed to write data.", (LPCSTR)"Error",MB_ICONEXCLAMATION | MB_OK);
		SendMessage( hDlg, WM_COMMAND, LOWORD(IDC_DISCONNECT), 0 );
		return TRUE;

	case UMSG_RECEIVEWARNING:
		MessageBox( hDlg, (LPCSTR)"Failed to receive data.", (LPCSTR)"Error",MB_ICONEXCLAMATION | MB_OK);
		SendMessage( hDlg, WM_COMMAND, LOWORD(IDC_DISCONNECT), 0 );
		return TRUE;
	}
	return FALSE;
}

/*--------------------------------------------------------------------------*/
/* Process of WM_COMMAND message							                */
/*--------------------------------------------------------------------------*/

BOOL WmCommand( HWND hDlg, WPARAM wParam, LPARAM lParam )
{	
	switch( LOWORD( wParam ) )
	{
	case IDC_CHANGE:
		return SaveFilesDlg( hDlg );

	case IDC_SELECT:	// �h���b�v�_�E�����X�g
		return ChangeSelectUnit(hDlg, wParam);

	case IDC_CONNECT:
		return OpenConnection( hDlg );

	case IDC_DISCONNECT:
		return CloseConnection( hDlg );
		
	case IDC_EXIT:
		return WmCloseMain( hDlg );
	}
	return FALSE;
}

/*--------------------------------------------------------------------------*/
/* Dialog box (main window) procedure                                       */
/*--------------------------------------------------------------------------*/

BOOL CALLBACK DlgProcMain( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    switch( uMsg )
    {
	case WM_INITDIALOG:
		return WmInitDialog( hDlg, wParam, lParam );

    case WM_COMMAND:
        return WmCommand( hDlg, wParam, lParam );

    case WM_CLOSE:
        return WmCloseMain( hDlg );

	case WM_TIMER:
		return WmTimer( hDlg, wParam, lParam );

	case WM_USER_MSG:
		return WmUserMsg( hDlg, wParam, lParam );

    }

    return FALSE;
}

/*--------------------------------------------------------------------------*/
/* Application program entry                                                */
/*--------------------------------------------------------------------------*/

int CALLBACK WinMain( HINSTANCE hInstance, HINSTANCE, LPSTR, int )
{
    g_hInstance = hInstance;

    DialogBox( g_hInstance, MAKEINTRESOURCE( IDD_MAIN ), NULL, DlgProcMain );
 
    return 0;
}