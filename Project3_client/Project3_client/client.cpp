#pragma comment(lib,"ws2_32")
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <WinSock2.h>
#include <stdlib.h>
#include <WS2tcpip.h>
#include <stdio.h>
#include <time.h>
#include "resource.h"

#define BUFSIZE 512
#define NICKSIZE 12
#define PORTSIZE 7
#define IPSIZE 16

char ServerIP[IPSIZE]; //SERVER �ּ�
char PortNumber[PORTSIZE]; //��Ʈ��ȣ
char NickName[NICKSIZE], Temp[NICKSIZE]; //�г���

						 //��ȭ���� ���ν���
BOOL CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);

//���� ��Ʈ�� ��� �Լ�
void DisplayText(char *fmt, ...);           // ä��â�� �������
void DisplayNumberOfClient(char *fmt, ...); //Ŭ���̾�Ʈ �� ���
void DisplayNick(char *fmt, ...);             //���� ����� ���
void DisplayNicknameOfClient(char *fmt, ...);   // ���������� �г��� ���

//���� ��� �Լ�
void err_quit(char *msg);
void err_display(char *msg);

//�����Ϳ� ������ ���
DWORD WINAPI Receiver(LPVOID arg);
DWORD WINAPI Sender(LPVOID arg);

//�����ǹ�ȣ, ��Ʈ��ȣ, �г��� Ȯ��
int CheckPortNumber();
int CheckIPAddress();
int CheckNickname(char * name);

SOCKET sock; //����
char buf[BUFSIZE + 1];  // ������ �ۼ��� ����
HANDLE hReadEvent, hWriteEvent;  //�б�, ���� �̺�Ʈ
HANDLE hNickEvent;  //ä�� IP�� �г��� �̺�Ʈ.
HANDLE hPortNumberEvent, hServerIPEvent;  //��Ʈ��ȣ �̺�Ʈ
HANDLE hConnectOnEvent;  //�ߺ�ó�� �̺�Ʈ
HWND hSendButton;  //������ ��ư
HWND hEdit1, hEdit2, hEdit3, hEdit4, hEdit5;
CRITICAL_SECTION cs, cs2; //�Ӱ� ����
int nChangeOfNick = 0, OverlapNick=0;
char * ImpossibleNick[] = {"�к�","�г���", "�ӼӸ�", "�ӼӸ�����"," "};  //������ �г���

//�ð������ ���� ���� ����
time_t ltime;
struct tm now = { 0 };

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	//�̺�Ʈ ����
	hWriteEvent = CreateEvent(NULL, FALSE, FALSE, NULL);  // ���ȣ���, �ڵ� ���� �̺�Ʈ ��� ���⸦ �˸�
	hPortNumberEvent = CreateEvent(NULL, TRUE, FALSE, NULL);  //���ȣ����, �������� ���, ��Ʈ��ȣ �̺�Ʈ
	hConnectOnEvent = CreateEvent(NULL, TRUE, FALSE, NULL);  //���ȣ����, �������� ���, ��Ʈ��ȣ �̺�Ʈ
	hServerIPEvent = CreateEvent(NULL, TRUE, FALSE, NULL);  //���ȣ����, �������� ���, IP�ּ� �̺�Ʈ
	hNickEvent = CreateEvent(NULL, TRUE, FALSE, NULL);  //���ȣ����, �������� ���, �г��� �̺�Ʈ
	InitializeCriticalSection(&cs);
	InitializeCriticalSection(&cs2);
	if (hWriteEvent == NULL || hServerIPEvent==NULL||hPortNumberEvent == NULL || hNickEvent == NULL)
		return 1;

	//���� ��� ������ ����
	CreateThread(NULL, 0, Sender, NULL, 0, NULL);
	CreateThread(NULL, 0, Receiver, NULL, 0, NULL);

	//��ȭ���� ����
	DialogBox(hInstance, MAKEINTRESOURCE(IDD_DIALOG1), NULL, DlgProc);

	//�̺�Ʈ ����
	CloseHandle(hReadEvent);
	CloseHandle(hWriteEvent);
	CloseHandle(hPortNumberEvent);
	CloseHandle(hServerIPEvent);

	//���� �ݱ�
	closesocket(sock);

	//���� ����
	WSACleanup();
	DeleteCriticalSection(&cs);
	DeleteCriticalSection(&cs2);
	return 0;
}

//��ȭ���� ���ν���
BOOL CALLBACK DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_INITDIALOG:  //���̾�α� �ʱ�ȭ
		hEdit1 = GetDlgItem(hDlg, IDC_EDIT1);
		hEdit2 = GetDlgItem(hDlg, IDC_EDIT2);
		hEdit3 = GetDlgItem(hDlg, IDC_EDIT5);
		hEdit4 = GetDlgItem(hDlg, IDC_EDIT6);
		hEdit5 = GetDlgItem(hDlg, IDC_EDIT7);
		hSendButton = GetDlgItem(hDlg, IDOK);
		SendMessage(hEdit1, EM_SETLIMITTEXT, BUFSIZE, 0);
		EnableWindow(hSendButton, FALSE);
		EnableWindow(hEdit1, FALSE);
		EnableWindow(hEdit2, FALSE);
		EnableWindow(hEdit3, FALSE);
		EnableWindow(hEdit4, FALSE);
		EnableWindow(hEdit5, FALSE);
		return TRUE;
	case WM_COMMAND:  //��ư�� ������ ���
		switch (LOWORD(wParam))
		{
		case IDC_BUTTON3: // �����ּ� Ȯ��
			GetDlgItemText(hDlg, IDC_IPADDRESS1, (LPSTR)ServerIP, sizeof(ServerIP));
			if (CheckIPAddress() == 0) 
				MessageBox(NULL, (LPCSTR)"�ùٸ� �����ּҸ� �Է����ֽʽÿ�.(���Ұ� : 224.0.0.0 ~ 255.255.255.255)", (LPCSTR)"Failed", MB_ICONERROR);
			else
			{
				EnableWindow(GetDlgItem(hDlg, IDC_IPADDRESS1), FALSE);  // IP�ּ� �Է¶� ��Ȱ��ȭ
				EnableWindow(GetDlgItem(hDlg, IDC_BUTTON3), FALSE);  //������ ��ư ��Ȱ��ȭ
				SetEvent(hServerIPEvent);
			}
			return TRUE;
		case IDC_BUTTON1:  // ��Ʈ��ȣ
			GetDlgItemText(hDlg, IDC_EDIT4, (LPSTR)PortNumber, sizeof(PortNumber));
			if (CheckPortNumber() == 0)
				MessageBox(NULL, (LPCSTR)"�ùٸ� ��Ʈ��ȣ�� �Է����ֽʽÿ�. (0~1023 port ���Ұ���)", (LPCSTR)"Failed", MB_ICONERROR);
			else
			{
				EnableWindow(GetDlgItem(hDlg, IDC_EDIT4), FALSE);  // ��Ʈ��ȣ �Է¶� ��Ȱ��ȭ
				EnableWindow(GetDlgItem(hDlg, IDC_BUTTON1), FALSE);  //������ ��ư ��Ȱ��ȭ
				SetEvent(hPortNumberEvent);
			}
			return TRUE;
		case IDC_BUTTON2: //�г��� ����
			char temp[NICKSIZE];
			GetDlgItemText(hDlg, IDC_EDIT3, (LPSTR)temp, sizeof(temp));

			if (!CheckNickname(temp))
			{
				MessageBox(NULL, (LPCSTR)"���Ұ����� �г����Դϴ�.", (LPCSTR)"Failed", MB_ICONERROR);
			}
			else if (strlen(temp) > 10)  //10���� ���� ū ���
			{
				MessageBox(NULL, (LPCSTR)"�г����� (��������)10���� �̸����� ����Ͻʽÿ�.", (LPCSTR)"Failed", MB_ICONERROR);
			}
			else if (!strcmp(NickName, ""))
			{
				ZeroMemory(Temp, sizeof(Temp));
				strcpy_s(Temp, strlen(temp) + 1, temp);
				SetEvent(hNickEvent);
			}
			else if (strcmp(NickName, temp) != 0)
			{
				if (!CheckNickname(temp))
				{
					MessageBox(NULL, (LPCSTR)"���Ұ����� �г����Դϴ�.", (LPCSTR)"Failed", MB_ICONERROR);
				}
				ltime = time(NULL);
				localtime_s(&now, &ltime);
				DisplayText("[%02d:%02d:%02d][%s]���� [%s]�� �г��� ����õ� ��û... \r\n", now.tm_hour, now.tm_min, now.tm_sec, NickName, temp);
				ZeroMemory(Temp, sizeof(Temp));
				strcpy_s(Temp, strlen(temp) + 1, temp);
				nChangeOfNick = 1;
				SetEvent(hWriteEvent);
				SetEvent(hNickEvent);
			}
			return TRUE;
		case IDOK:
			EnableWindow(hSendButton, FALSE);  //������ ��ư ��Ȱ��ȭ
			GetDlgItemText(hDlg, IDC_EDIT1, (LPSTR)buf, BUFSIZE + 1);
			if (!strcmp(buf, "help"))
			{
				DisplayText("ä�����α׷� �����Դϴ�.\r\n");
				DisplayText("�ӼӸ� ���� : \" �ӼӸ� �г��� ���� \" \r\n");
				DisplayText("ä��â ����� : \" /clear \" \r\n");
				SetDlgItemText(hDlg, IDC_EDIT1, NULL);
				SetFocus(hEdit1);
				Sleep(500);
				EnableWindow(hSendButton, TRUE);
				return TRUE;
			}else if(!strcmp(buf,"/clear"))
			{
				SetDlgItemText(hDlg, IDC_EDIT2, NULL);
				SetDlgItemText(hDlg, IDC_EDIT1, NULL);
				SetFocus(hEdit1);
				Sleep(500);
				EnableWindow(hSendButton, TRUE);
				return TRUE;
			}
			SetFocus(hEdit1);
			SetEvent(hWriteEvent);  //buf�� ���⸦ �Ϸ��ߴٰ� �˸�.
			SetDlgItemText(hDlg, IDC_EDIT1, NULL);
			return TRUE;
		case IDCANCEL:
			EndDialog(hDlg, IDCANCEL);
			return TRUE;
		}
		return FALSE;
	}
	return FALSE;
}
int CheckIPAddress()
{
	char string[IPSIZE];
	char *tmp;
	char *ptmp;
	int result=0;
	ZeroMemory(string, sizeof(string));
	strcpy_s(string, IPSIZE,ServerIP);
	tmp=strtok_s(string, ".", &ptmp);
	result = atoi(tmp);
	
	if (result >= 224 && result <= 255)
		return 0;
	else
		return 1;
}
int CheckPortNumber()
{
	int result = atoi(PortNumber);
	if (result > 1023 && result<=65535)
		return 1;
	else
		return 0;
}
int CheckNickname(char * name)
{
	if (name[0]==NULL)  //�ƹ��͵� �Է����� �������
		return 0;
	//� ���ڶ� �Է��� ��� �Ʒ����� ���͸��� ��
	int i = 0;
	int arr_strlen = sizeof(ImpossibleNick) / sizeof(ImpossibleNick[0]);
	
	while (arr_strlen>i)
	{
		if (!strncmp(name, ImpossibleNick[i], strlen(ImpossibleNick[i])))
			return 0;
		i++;
	}
	return 1;
}
void DisplayText(char *fmt, ...) 
{
	va_list arg;
	va_start(arg, fmt);

	char cbuf[BUFSIZE + 256];
	vsprintf_s(cbuf, fmt, arg);

	EnterCriticalSection(&cs);
	int nLength = GetWindowTextLength(hEdit2);
	SendMessage(hEdit2, EM_SETSEL, nLength, nLength);
	SendMessage(hEdit2, EM_REPLACESEL, FALSE, (LPARAM)cbuf);
	LeaveCriticalSection(&cs);

	va_end(arg);
}
void DisplayNick(char *fmt, ...)
{
	va_list arg;
	va_start(arg, fmt);

	char cbuf[NICKSIZE + 256];
	vsprintf_s(cbuf, fmt, arg);
	EnterCriticalSection(&cs2);
	int nLength = GetWindowTextLength(hEdit3);
	SendMessage(hEdit3, EM_SETSEL, nLength, nLength);
	SendMessage(hEdit3, EM_REPLACESEL, FALSE, (LPARAM)cbuf);
	LeaveCriticalSection(&cs2);

	va_end(arg);
}
void DisplayNicknameOfClient(char *fmt, ...)
{
	va_list arg;
	va_start(arg, fmt);

	char cbuf[NICKSIZE + 256];
	vsprintf_s(cbuf, fmt, arg);
	EnterCriticalSection(&cs2);
	int nLength = GetWindowTextLength(hEdit5);
	SendMessage(hEdit5, EM_SETSEL, nLength, nLength);
	SendMessage(hEdit5, EM_REPLACESEL, FALSE, (LPARAM)cbuf);
	LeaveCriticalSection(&cs2);

	va_end(arg);
}
void DisplayNumberOfClient(char *fmt, ...)
{
	va_list arg;
	va_start(arg, fmt);

	char cbuf[NICKSIZE + 256];
	vsprintf_s(cbuf, fmt, arg);
	EnterCriticalSection(&cs2);
	int nLength = GetWindowTextLength(hEdit4);
	SendMessage(hEdit4, EM_SETSEL, nLength, nLength);
	SendMessage(hEdit4, EM_REPLACESEL, FALSE, (LPARAM)cbuf);
	LeaveCriticalSection(&cs2);

	va_end(arg);
}
//���� �Լ� ���� ��� �� ����
void err_quit(char *msg)
{
	LPVOID lpMsgBuf;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, WSAGetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	MessageBox(NULL, (LPCSTR)lpMsgBuf, (LPCSTR)msg, MB_ICONERROR);
	LocalFree(lpMsgBuf);
	exit(1);
}

void err_display(char *msg)
{
	LPVOID lpMsgBuf;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, WSAGetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	DisplayText("[%s] %s", msg, (char*)lpMsgBuf);
	LocalFree(lpMsgBuf);
}



// Ŭ���̾�Ʈ�� ������ ����ϴ� �κ�
DWORD WINAPI Receiver(LPVOID arg)
{
	WaitForSingleObject(hServerIPEvent, INFINITE);
	WaitForSingleObject(hPortNumberEvent, INFINITE);
	WaitForSingleObject(hNickEvent, INFINITE);
	char Overlap[NICKSIZE];
	
	int retval;

	//���� �ʱ�ȭ
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return 1;

	//socket()
	sock = socket(AF_INET, SOCK_STREAM, 0);   //TCP���
	if (sock == INVALID_SOCKET)
		err_quit("socket()");


	// connect()
	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.S_un.S_addr = inet_addr(ServerIP);
	serveraddr.sin_port = htons(atoi(PortNumber));
	retval = connect(sock, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR)
		err_quit("connect()");
	
	do
	{
		WaitForSingleObject(hNickEvent, INFINITE);
			retval = send(sock, Temp, NICKSIZE, 0);
			if (retval == SOCKET_ERROR)
			{
				err_display("send Nick Name Error!!");
				exit(1);
			}
		retval = recv(sock, Overlap, NICKSIZE, 0);

		if (retval == SOCKET_ERROR)
		{
			err_display("recv Nick Name Error!!");
			exit(1);
		}
		if (!strcmp(Overlap, "�ߺ�"))  //�г��� ��� ��
		{
			OverlapNick = 1;
			MessageBox(NULL, (LPCSTR)"�ߺ��� �г����Դϴ�. ����� �� �����ϴ�.1", (LPCSTR)"Failed", MB_ICONERROR);
			ResetEvent(hNickEvent);
		}
		else
		{
			ZeroMemory(NickName, sizeof(NickName));
			strcpy_s(NickName, strlen(Temp) + 1, Temp);
			OverlapNick = 0;
		}
	}while (OverlapNick == 1);      // �ߺ��� �г����̶�� ������ �ؾ��� ���������� �޽����� �����ų� ������ ����.
	SetEvent(hConnectOnEvent);
	//������ ��ſ� ����� ����
	char buf[BUFSIZE + 1];
	char name[NICKSIZE];
	int numOfClient = 0;
	//�ڽ��� ����г��� ���
	SetWindowTextA(hEdit5, "");
	DisplayNicknameOfClient(NickName);
	//������ �ޱ�
	while (1)
	{
		//������ �ޱ�, �г��� �޴°�
		retval = recv(sock, name, NICKSIZE, 0);
		if (retval == SOCKET_ERROR)
		{
			err_display("recv() Nick");
			break;
		}
		if (!strcmp(name, "�г���"))  //�г��� ���
		{
			SetWindowTextA(hEdit4, "");
			SetWindowTextA(hEdit3, "");
			retval=recv(sock, name, NICKSIZE, 0);
			if (retval == SOCKET_ERROR)
			{
				err_display("recv() Nick");
				break;
			}
			numOfClient = atoi(name);
			DisplayNumberOfClient("(%d��)", numOfClient);
			for (int i = 0; i < numOfClient; i++)
			{
				retval = recv(sock, name, NICKSIZE, 0);
				if (retval == SOCKET_ERROR)
				{
					err_display("recv() Nick");
					break;
				}
				DisplayNick("%s\r\n",name);
			}
			continue;
		}else if(!strcmp(name, "�ߺ�"))    //�г��� ��� ��
		{
			DisplayText("[%02d:%02d:%02d][%s]���� [%s]�� �г��� ���� ���� \r\n", now.tm_hour, now.tm_min, now.tm_sec, NickName, Temp);
			MessageBox(NULL, (LPCSTR)"�ߺ��� �г����Դϴ�. ����� �� �����ϴ�.", (LPCSTR)"Failed", MB_ICONERROR);
			continue;
		}
		else if (!strcmp(name, "���ߺ�"))
		{
			DisplayText("[%02d:%02d:%02d][%s]���� [%s]�� �г��� ���� ���� \r\n", now.tm_hour, now.tm_min, now.tm_sec, NickName, Temp);
			ZeroMemory(NickName, sizeof(NickName));
			strcpy_s(NickName, strlen(Temp) + 1, Temp);
			//�ڽ��� ����г��� ����
			SetWindowTextA(hEdit5, "");
			DisplayNicknameOfClient(NickName);
			continue;
		}
		if (!strncmp(name, "�ӼӸ�", strlen("�ӼӸ�")))  // �ӼӸ��� ���� �� �� From.
		{
			if (!strcmp(name,"�ӼӸ�"))
			{
				retval = recv(sock, name, NICKSIZE, 0);
				if (retval == SOCKET_ERROR)
				{
					err_display("recv error() �ӼӸ� �г���");
					break;
				}
				ltime = time(NULL);
				localtime_s(&now, &ltime);
				DisplayText("[%02d:%02d:%02d][�ӼӸ�]From_%s> ",now.tm_hour,now.tm_min,now.tm_sec,name);
				retval = recv(sock, buf, BUFSIZE, 0);
				if (retval == SOCKET_ERROR)
				{
					err_display("recv error() �ӼӸ� ����");
					break;
				}
				DisplayText("%s\r\n",buf);
				continue;
			}
			else if (!strcmp(name, "�ӼӸ�����"))      //���� ������ �ӼӸ��� ���������� ��� To.
			{
				retval = recv(sock, name, NICKSIZE, 0);
				if (retval == SOCKET_ERROR)
				{
					err_display("recv error() �ӼӸ� �г���");
					break;
				}
				ltime = time(NULL);
				localtime_s(&now, &ltime);
				DisplayText("[%02d:%02d:%02d][�ӼӸ�]To_%s> ", now.tm_hour, now.tm_min, now.tm_sec, name);
				retval = recv(sock, buf, BUFSIZE, 0);
				if (retval == SOCKET_ERROR)
				{
					err_display("recv error() �ӼӸ� ����");
					break;
				}
				DisplayText("%s\r\n", buf);
				continue;
			}
			else if (!strcmp(name, "�ӼӸ�����"))
			{
				DisplayText("�ӼӸ� ������ \" �ӼӸ� �г��� ���� \"�Դϴ�.\r\n");
				continue;
			}
			else if (!strcmp(name, "�ӼӸ���"))
			{
				DisplayText("������ ���������� �ʽ��ϴ�.\r\n");
				continue;
			}
			else if (!strcmp(name, "�ӼӸ�me"))
			{
				DisplayText("�ڽſ��Դ� �ӼӸ��� ���� �� �����ϴ�.\r\n");
				continue;
			}
		}
		//������ �ޱ�, ����
		retval = recv(sock, buf, BUFSIZE, 0);
		if (retval == SOCKET_ERROR)
		{
			err_display("recv()2 ");
			break;
		}
		//���� ������ ���
		buf[retval] = '\0';
		ltime = time(NULL);
		localtime_s(&now, &ltime);
		DisplayText("[%s][%02d:%02d:%02d]%s \r\n", name, now.tm_hour, now.tm_min, now.tm_sec, buf);
	}
	return 0;
}
DWORD WINAPI Sender(LPVOID arg)
{
	WaitForSingleObject(hServerIPEvent, INFINITE);
	WaitForSingleObject(hPortNumberEvent, INFINITE);
	WaitForSingleObject(hNickEvent, INFINITE);
	WaitForSingleObject(hConnectOnEvent, INFINITE);
	int retval;

	EnableWindow(hEdit1, TRUE);  //Ȱ��ȭ ��Ŵ
	EnableWindow(hEdit2, TRUE);
	EnableWindow(hEdit3, TRUE);
	EnableWindow(hSendButton, TRUE);

	DisplayText(" chatting start!! Server ip : %s. ��Ʈ��ȣ : %s\r\n ä�ÿ��� �����ϴ� ��ɾ ���Ǹ� help�� �Է����ּ���.\r\n", ServerIP, PortNumber);

	//������ ������(����)
	while (1)
	{
		WaitForSingleObject(hWriteEvent, INFINITE); // ���� �Ϸ� ��ٸ���
													//���ڿ� ���̰� 0�̸� ������ ����
		if (nChangeOfNick == 1) //�г��� ������ �������� �˸�
		{
			retval = send(sock, "�к�", NICKSIZE, 0);
			if (retval == SOCKET_ERROR)
			{
				err_display("send() NickName ���Ž���");
				continue;
			}
			retval = send(sock, Temp, NICKSIZE, 0);
			if (retval == SOCKET_ERROR)
			{
				err_display("send() NickName ���Ž���");
				continue;
			}
			nChangeOfNick = 0;
			continue;
		}
		//������ ������
		retval = send(sock, NickName, strlen(NickName), 0);
		if (retval == SOCKET_ERROR)
		{
			err_display("send() NickName");
			continue;
		}
		retval = send(sock, buf, strlen(buf), 0);
		if (retval == SOCKET_ERROR)
		{
			err_display("send() Buf");
			continue;
		}
		Sleep(500);
		EnableWindow(hSendButton, TRUE); // ������ ��ư Ȱ��ȭ

	}

	//closesocket()
	closesocket(sock);

	//���� ����
	WSACleanup();
}