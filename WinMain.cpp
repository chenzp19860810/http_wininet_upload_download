#include <stdio.h>
#include <windows.h>
#include <commctrl.h>
#include <Wininet.h>
#include "resource.h"

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "Wininet.lib")

//�ϴ��������ļ��Ļ�������С
#define UPLOAD_BUFFER_SIZE   1024*64
#define DOWNLOAD_BUFFER_SIZE 1024*64

//���ļ��ϴ�ʱ��Ҫ�������
typedef struct _UPLOAD_FILE
{
	char strIP[16]; //������IP��ַ
	char strLocalFile[MAX_PATH]; //Ҫ�ϴ��ı����ļ�
	char strRemoteDir[1024]; //�ϴ�����������Ŀ¼

}UPLOAD_FILE, *PUPLOAD_FILE;

//���ļ�����ʱ��Ҫ�������
typedef struct _DOWNLOAD_FILE
{
	char strIP[16]; //������IP��ַ
	char strRemoteFile[1024]; //Ҫ���صķ������ϵ��ļ�
	char strLocalFile[MAX_PATH]; //���ص��ı����ļ�

}DOWNLOAD_FILE, *PDOWNLOAD_FILE;

char curDir[MAX_PATH] = {0}; //����ĵ�ǰĿ¼
HWND hMainDlg = NULL; //���Ի���ľ��

DOWNLOAD_FILE download_file; //�ļ����ؽṹ
UPLOAD_FILE upload_file; //�ļ��ϴ��ṹ

BOOL CALLBACK MainDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam); //���Ի���Ļص�����
BOOL MainDlgOnInit(HWND hwndDlg, WPARAM wParam, LPARAM lParam); //ִ�����Ի��򴴽�ʱ�ĳ�ʼ������
BOOL MainDlgOnCommand(HWND hwndDlg, WPARAM wParam, LPARAM lParam); //���Ի�����BUTTON���Ϳؼ�����Ϣ��Ӧ����

DWORD WINAPI UploadFileProc(LPVOID lpParameter); //�ļ��ϴ��߳�
DWORD WINAPI DownloadFileProc(LPVOID lpParameter); //�ļ������߳�

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	::DialogBox(hInstance, MAKEINTRESOURCE(IDD_MAIN), NULL, MainDlgProc);
	return 0;
}

BOOL CALLBACK MainDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg)
	{
	case WM_INITDIALOG:
		MainDlgOnInit(hwndDlg, wParam, lParam);
		break;
	case WM_COMMAND:
		MainDlgOnCommand(hwndDlg, wParam, lParam);
		break;
	}
	return FALSE; //���ڶԻ���������ڴ�������Ϣ֮��Ӧ�÷���FALSE����ϵͳ��һ������
}

BOOL MainDlgOnInit(HWND hwndDlg, WPARAM wParam, LPARAM lParam)
{
	hMainDlg = hwndDlg; //�����Ի�����
	::GetCurrentDirectory(sizeof(curDir), curDir); //��ȡ����ĵ�ǰĿ¼�������Ժ󲻻��ٸı�

	HWND hwndIP = ::GetDlgItem(hwndDlg, IDT_IP);
	::SendMessage(hwndIP, WM_SETTEXT, 0, (LPARAM)"139.129.116.15"); //���ó�ʼ��������IP��ַ

	HWND hwndLocalFile = ::GetDlgItem(hwndDlg, IDT_UPLOAD_LOCAL_FILE);
	::SendMessage(hwndLocalFile, WM_SETTEXT, 0, (LPARAM)"C:\\upload.txt"); //����Ҫ�ϴ��ĳ�ʼ�����ļ�

	HWND hwndRemoteDir = ::GetDlgItem(hwndDlg, IDT_UPLOAD_REMOTE_DIR);
	::SendMessage(hwndRemoteDir, WM_SETTEXT, 0, (LPARAM)"\\upload\\"); //����Ҫ�ϴ��ļ��ĳ�ʼԶ��Ŀ¼

	HWND hwndRemoteFile = ::GetDlgItem(hwndDlg, IDT_DOWNLOAD_REMOTE_FILE);
	::SendMessage(hwndRemoteFile, WM_SETTEXT, 0, (LPARAM)"\\index.html"); //����Ҫ�����ļ��ĳ�ʼURL
	
	HWND hwndTransProgress = ::GetDlgItem(hwndDlg, IDP_TRANSFER_RATE); //���������

	::SendMessage(hwndTransProgress, PBM_SETRANGE32, 0, 100); //���ý������ķ�Χ��0--100��
	::SendMessage(hwndTransProgress, PBM_SETPOS, 0, 0); //���ý������ĳ�ʼλ��Ϊ0

	return TRUE;
}

BOOL MainDlgOnCommand(HWND hwndDlg, WPARAM wParam, LPARAM lParam)
{
	int ID = LOWORD(wParam);
	if(ID == IDCANCEL)
	{
		::EndDialog(hwndDlg, 0);
	}
	if(ID == IDB_UPLOAD_SEL_FILE) //ѡ��Ҫ�ϴ��ı����ļ�
	{
		char filename[MAX_PATH] = {0};
		OPENFILENAME open_file;
		memset(&open_file, 0, sizeof(OPENFILENAME));
		open_file.lStructSize = sizeof(OPENFILENAME);
		open_file.lpstrFile = filename; //���ļ��Ի����з��ص��ļ�ȫ��
		open_file.nMaxFile = MAX_PATH;
		int id = ::GetOpenFileName(&open_file); //�������ļ��Ի��򣬲�����ѡ����ļ�ȫ��
		if(id == IDOK)
		{
			::SendMessage(::GetDlgItem(hwndDlg, IDT_UPLOAD_LOCAL_FILE), WM_SETTEXT, 0, (LPARAM)filename);
		}
	}
	if(ID == IDB_UPLOAD_START) //��ʼ�ϴ��ļ�
	{
		memset(&upload_file, 0, sizeof(UPLOAD_FILE));

		::SendMessage(::GetDlgItem(hwndDlg, IDT_IP), WM_GETTEXT, sizeof(upload_file.strIP), (LPARAM)upload_file.strIP);
		::SendMessage(::GetDlgItem(hwndDlg, IDT_UPLOAD_LOCAL_FILE), WM_GETTEXT, sizeof(upload_file.strLocalFile), (LPARAM)upload_file.strLocalFile);
		::SendMessage(::GetDlgItem(hwndDlg, IDT_UPLOAD_REMOTE_DIR), WM_GETTEXT, sizeof(upload_file.strRemoteDir), (LPARAM)upload_file.strRemoteDir);

		::CreateThread(NULL, 0, UploadFileProc, &upload_file, 0, 0); //�����ļ��ϴ��߳�
	}
	if(ID == IDB_DOWNLOAD_START) //��ʼ�����ϴ�
	{
		memset(&download_file, 0, sizeof(DOWNLOAD_FILE));

		::SendMessage(::GetDlgItem(hwndDlg, IDT_IP), WM_GETTEXT, sizeof(download_file.strIP), (LPARAM)download_file.strIP);
		::SendMessage(::GetDlgItem(hwndDlg, IDT_DOWNLOAD_REMOTE_FILE), WM_GETTEXT, sizeof(download_file.strRemoteFile), (LPARAM)download_file.strRemoteFile);
		strcpy(download_file.strLocalFile, curDir), strcat(download_file.strLocalFile, "\\download.txt");
		
		::CreateThread(NULL, 0, DownloadFileProc, &download_file, 0, 0); //�����ļ������߳�
	}
	return TRUE;
}

DWORD WINAPI UploadFileProc(LPVOID lpParameter)
{	
	UPLOAD_FILE* upload_file = (UPLOAD_FILE*)lpParameter; //��ȡ�ļ��ϴ��ṹ

	//��ʼ��һ��Internet�Ự
	HINTERNET hInternet = ::InternetOpen("HTTP_WINNET_UploadDownload", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
	if(!hInternet)
	{
		char errorInfo[128] = {0};
		sprintf(errorInfo,"InternetOpen failed with code %d.", GetLastError());
		MessageBox(hMainDlg, errorInfo, "fail", MB_OK | MB_ICONERROR);
		return 0;
	}

	//����HTTPЭ�����ӷ�����
	HINTERNET hConnect = ::InternetConnect(hInternet, upload_file->strIP, 80, NULL, NULL, INTERNET_SERVICE_HTTP, 0, NULL);
	if(!hConnect)
	{
		char errorInfo[128] = {0};
		sprintf(errorInfo,"InternetConnect failed with code %d.", GetLastError());
		MessageBox(hMainDlg, errorInfo, "fail", MB_OK | MB_ICONERROR);
		::InternetCloseHandle(hInternet);
		return 0;
	}

	//�����ļ��ϴ�����
	HINTERNET hRequest = ::HttpOpenRequest(hConnect, "PUT", upload_file->strRemoteDir, "HTTP/1.1", NULL, NULL, INTERNET_FLAG_KEEP_CONNECTION, NULL);
	if(!hRequest)
	{
		char errorInfo[128] = {0};
		sprintf(errorInfo,"HttpOpenRequest failed with code %d.", GetLastError());
		MessageBox(hMainDlg, errorInfo, "fail", MB_OK | MB_ICONERROR);
		::InternetCloseHandle(hConnect);
		::InternetCloseHandle(hInternet);
		return 0;
	}	

	//�򿪱����Ѵ����ļ�
	HANDLE hFile = ::CreateFile(upload_file->strLocalFile, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	int file_size = ::GetFileSize(hFile, NULL); //��ȡ�����ļ��Ĵ�С

	INTERNET_BUFFERS ib;
	memset(&ib, 0, sizeof(INTERNET_BUFFERS));
	ib.dwStructSize = sizeof(INTERNET_BUFFERS);
	ib.dwBufferTotal = file_size;
	BOOL bSend = ::HttpSendRequestEx(hRequest, &ib, 0, NULL, 0); //�������͵�������
	if(!bSend)
	{
		char errorInfo[128] = {0};
		sprintf(errorInfo,"HttpSendRequestEx failed with code %d.", GetLastError());
		MessageBox(hMainDlg, errorInfo, "fail", MB_OK | MB_ICONERROR);
		::CloseHandle(hFile);
		::InternetCloseHandle(hRequest);
		::InternetCloseHandle(hConnect);
		::InternetCloseHandle(hInternet);	
		return 0;
	}
	
	::ShowWindow(::GetDlgItem(hMainDlg, IDP_TRANSFER_RATE), SW_SHOW);
	::EnableWindow(::GetDlgItem(hMainDlg, IDB_UPLOAD_START), FALSE);

	DWORD dwRead = 0, dwWrite = 0, sumRead = 0, sumWrite = 0, rate =0;
	char *lpBuffer = new char[UPLOAD_BUFFER_SIZE];
	do
	{
		memset(lpBuffer, 0, UPLOAD_BUFFER_SIZE);
		::ReadFile(hFile, lpBuffer, UPLOAD_BUFFER_SIZE, &dwRead, NULL); //��ȡ�����ļ�������

		sumWrite =0;
		do
		{
			::InternetWriteFile(hRequest, lpBuffer+sumWrite, dwRead, &dwWrite); //д�뵽Զ���ļ�
			sumWrite += dwWrite;

		}while(sumWrite< dwRead);

		sumRead += dwRead;
		
		rate = (double)sumRead*100/file_size; //������Ҫ��ʱת����double��������ڴ��ļ���Խ��
		::SendMessage(::GetDlgItem(hMainDlg, IDP_TRANSFER_RATE), PBM_SETPOS, rate, 0);	//���õ�ǰ����Ľ���

	}while(sumRead<file_size);

	delete []lpBuffer;

	::ShowWindow(::GetDlgItem(hMainDlg, IDP_TRANSFER_RATE), SW_HIDE);
	::EnableWindow(::GetDlgItem(hMainDlg, IDB_UPLOAD_START), TRUE);

	::CloseHandle(hFile);
	::InternetCloseHandle(hRequest);
	::InternetCloseHandle(hConnect);
	::InternetCloseHandle(hInternet);	

	MessageBox(hMainDlg, "�ļ��ϴ��ɹ�", "success", MB_OK);
	return 0;
}

DWORD WINAPI DownloadFileProc(LPVOID lpParameter)
{
	DOWNLOAD_FILE* download_file = (DOWNLOAD_FILE*)lpParameter; //��ȡ�ļ����ؽṹ

	//��ʼ��һ��Internet�Ự
	HINTERNET hInternet = ::InternetOpen("HTTP_WINNET_UploadDownload", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
	if(!hInternet)
	{
		char errorInfo[128] = {0};
		sprintf(errorInfo,"InternetOpen failed with code %d.", GetLastError());
		MessageBox(hMainDlg, errorInfo, "fail", MB_OK | MB_ICONERROR);
		return 0;
	}

	//��ȡ��ʱʱ��
	DWORD dwTime = 0, dwLen = 0;
	::InternetQueryOption(hInternet, INTERNET_OPTION_CONNECT_TIMEOUT, &dwTime, &dwLen);
	dwTime = 0;
	::InternetQueryOption(hInternet, INTERNET_OPTION_DATA_RECEIVE_TIMEOUT, &dwTime, &dwLen);
	dwTime = 0;
	::InternetQueryOption(hInternet, INTERNET_OPTION_DATA_SEND_TIMEOUT, &dwTime, &dwLen);
	dwTime = 0;
	::InternetQueryOption(hInternet, INTERNET_OPTION_DISCONNECTED_TIMEOUT, &dwTime, &dwLen);
	dwTime = 0;
	::InternetQueryOption(hInternet, INTERNET_OPTION_RECEIVE_TIMEOUT, &dwTime, &dwLen);
	dwTime = 0;
	::InternetQueryOption(hInternet, INTERNET_OPTION_SEND_TIMEOUT, &dwTime, &dwLen);

	//���ó�ʱʱ��
	dwTime = 10*1000;
	::InternetSetOption(hInternet, INTERNET_OPTION_CONNECT_TIMEOUT, &dwTime, 4);
	dwTime = 10*1000;
	::InternetSetOption(hInternet, INTERNET_OPTION_DATA_RECEIVE_TIMEOUT, &dwTime, 4);
	dwTime = 10*1000;
	::InternetSetOption(hInternet, INTERNET_OPTION_DATA_SEND_TIMEOUT, &dwTime, 4);
	dwTime = 10*1000;
	::InternetSetOption(hInternet, INTERNET_OPTION_DISCONNECTED_TIMEOUT, &dwTime, 4);
	dwTime = 10*1000;
	::InternetSetOption(hInternet, INTERNET_OPTION_RECEIVE_TIMEOUT, &dwTime, 4);
	dwTime = 10*1000;
	::InternetSetOption(hInternet, INTERNET_OPTION_SEND_TIMEOUT, &dwTime, 4);	

	//����HTTPЭ�����ӷ�����
	HINTERNET hConnect = ::InternetConnect(hInternet, download_file->strIP, 80, NULL, NULL, INTERNET_SERVICE_HTTP, 0, NULL);
	if(!hConnect)
	{
		char errorInfo[128] = {0};
		sprintf(errorInfo,"InternetConnect failed with code %d.", GetLastError());
		MessageBox(hMainDlg, errorInfo, "fail", MB_OK | MB_ICONERROR);
		::InternetCloseHandle(hInternet);
		return 0;
	}

	//�����ļ�ͷ����Ŀ����Ҫ��ȡ�����ļ��Ĵ�С
	HINTERNET hRequest = ::HttpOpenRequest(hConnect, "HEAD", download_file->strRemoteFile, "HTTP/1.1", NULL, NULL, INTERNET_FLAG_KEEP_CONNECTION, NULL);
	if(!hRequest)
	{
		char errorInfo[128] = {0};
		sprintf(errorInfo,"HttpOpenRequest failed with code %d.", GetLastError());
		MessageBox(hMainDlg, errorInfo, "fail", MB_OK | MB_ICONERROR);
		::InternetCloseHandle(hConnect);
		::InternetCloseHandle(hInternet);
		return 0;
	}

	BOOL bSend = ::HttpSendRequest(hRequest, NULL, 0, NULL, 0); //�������͵�������
	if(!bSend)
	{
		char errorInfo[128] = {0};
		sprintf(errorInfo,"HttpSendRequest failed with code %d.", GetLastError());
		MessageBox(hMainDlg, errorInfo, "fail", MB_OK | MB_ICONERROR);		
		::InternetCloseHandle(hRequest);
		::InternetCloseHandle(hConnect);
		::InternetCloseHandle(hInternet);	
		return 0;
	}
	//��ѯ�ղŷ�����������Ӧ����Ӧ����Ϣ
	DWORD dwContentLen = 0, dwSize = sizeof(dwContentLen);
	BOOL bQuery = HttpQueryInfo(hRequest, HTTP_QUERY_FLAG_NUMBER | HTTP_QUERY_CONTENT_LENGTH, &dwContentLen, &dwSize, NULL);  
	if(!bQuery)
	{
		char errorInfo[128] = {0};
		sprintf(errorInfo,"HttpQueryInfo failed with code %d.", GetLastError());
		MessageBox(hMainDlg, errorInfo, "fail", MB_OK | MB_ICONERROR);		
		::InternetCloseHandle(hRequest);
		::InternetCloseHandle(hConnect);
		::InternetCloseHandle(hInternet);
		return 0;
	}
	::InternetCloseHandle(hRequest);

	//�����ļ���������
	hRequest = ::HttpOpenRequest(hConnect, "GET", download_file->strRemoteFile, "HTTP/1.1", NULL, NULL, INTERNET_FLAG_KEEP_CONNECTION, NULL);
	if(!hRequest)
	{
		char errorInfo[128] = {0};
		sprintf(errorInfo,"HttpOpenRequest failed with code %d.", GetLastError());
		MessageBox(hMainDlg, errorInfo, "fail", MB_OK | MB_ICONERROR);
		::InternetCloseHandle(hConnect);
		::InternetCloseHandle(hInternet);
		return 0;
	}

	bSend = ::HttpSendRequest(hRequest, NULL, 0, NULL, 0); //�������͵�������
	if(!bSend)
	{
		char errorInfo[128] = {0};
		sprintf(errorInfo,"HttpSendRequest failed with code %d.", GetLastError());
		MessageBox(hMainDlg, errorInfo, "fail", MB_OK | MB_ICONERROR);		
		::InternetCloseHandle(hRequest);
		::InternetCloseHandle(hConnect);
		::InternetCloseHandle(hInternet);	
		return 0;
	}
	
	//�򿪱����Ѵ����ļ�
	HANDLE hFile = ::CreateFile(download_file->strLocalFile, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
	
	::ShowWindow(::GetDlgItem(hMainDlg, IDP_TRANSFER_RATE), SW_SHOW);
	::EnableWindow(::GetDlgItem(hMainDlg, IDB_DOWNLOAD_START), FALSE);

	DWORD dwRead = 0, dwWrite = 0, sumRead = 0, sumWrite = 0, rate = 0;	
	char *lpBuffer = new char[DOWNLOAD_BUFFER_SIZE];
	FILE * file = NULL;
	do
	{		
		memset(lpBuffer, 0, DOWNLOAD_BUFFER_SIZE);
		::InternetReadFile(hRequest, lpBuffer, DOWNLOAD_BUFFER_SIZE, &dwRead); //��ȡԶ���ļ�	

		sumWrite = 0;
		do
		{
			::WriteFile(hFile, lpBuffer+sumWrite, dwRead, &dwWrite, NULL); //д�뵽�����ļ�
			sumWrite += dwWrite;

		}while(sumWrite<dwRead);

		sumRead += dwRead;
		
		rate = (double)sumRead*100/dwContentLen; //������Ҫ��ʱת����double��������ڴ��ļ���Խ��
		::SendMessage(::GetDlgItem(hMainDlg, IDP_TRANSFER_RATE), PBM_SETPOS, rate, 0);	//���õ�ǰ����Ľ���		

	}while(sumRead<dwContentLen);

	delete []lpBuffer;

	::ShowWindow(::GetDlgItem(hMainDlg, IDP_TRANSFER_RATE), SW_HIDE);
	::EnableWindow(::GetDlgItem(hMainDlg, IDB_DOWNLOAD_START), TRUE);

	::CloseHandle(hFile);
	::InternetCloseHandle(hRequest);
	::InternetCloseHandle(hConnect);
	::InternetCloseHandle(hInternet);	

	MessageBox(hMainDlg, "�ļ����سɹ�", "success", MB_OK);
	return 0;
}