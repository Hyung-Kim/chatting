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

char ServerIP[IPSIZE]; //SERVER 주소
char PortNumber[PORTSIZE]; //포트번호
char NickName[NICKSIZE], Temp[NICKSIZE]; //닉네임

						 //대화상자 프로시저
BOOL CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);

//편집 컨트롤 출력 함수
void DisplayText(char *fmt, ...);           // 채팅창에 내용출력
void DisplayNumberOfClient(char *fmt, ...); //클라이언트 수 출력
void DisplayNick(char *fmt, ...);             //현재 사용자 출력
void DisplayNicknameOfClient(char *fmt, ...);   // 현재사용자의 닉네임 출력

//오류 출력 함수
void err_quit(char *msg);
void err_display(char *msg);

//서버와와 데이터 통신
DWORD WINAPI Receiver(LPVOID arg);
DWORD WINAPI Sender(LPVOID arg);

//아이피번호, 포트번호, 닉네임 확인
int CheckPortNumber();
int CheckIPAddress();
int CheckNickname(char * name);

SOCKET sock; //소켓
char buf[BUFSIZE + 1];  // 데이터 송수신 버퍼
HANDLE hReadEvent, hWriteEvent;  //읽기, 쓰기 이벤트
HANDLE hNickEvent;  //채팅 IP와 닉네임 이벤트.
HANDLE hPortNumberEvent, hServerIPEvent;  //포트번호 이벤트
HANDLE hConnectOnEvent;  //중복처리 이벤트
HWND hSendButton;  //보내기 버튼
HWND hEdit1, hEdit2, hEdit3, hEdit4, hEdit5;
CRITICAL_SECTION cs, cs2; //임계 영역
int nChangeOfNick = 0, OverlapNick=0;
char * ImpossibleNick[] = {"닉변","닉네임", "귓속말", "귓속말성공"," "};  //제외할 닉네임

//시간출력을 위한 변수 선언
time_t ltime;
struct tm now = { 0 };

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	//이벤트 생성
	hWriteEvent = CreateEvent(NULL, FALSE, FALSE, NULL);  // 비신호상대, 자동 리셋 이벤트 방식 쓰기를 알림
	hPortNumberEvent = CreateEvent(NULL, TRUE, FALSE, NULL);  //비신호상태, 수동리셋 방식, 포트번호 이벤트
	hConnectOnEvent = CreateEvent(NULL, TRUE, FALSE, NULL);  //비신호상태, 수동리셋 방식, 포트번호 이벤트
	hServerIPEvent = CreateEvent(NULL, TRUE, FALSE, NULL);  //비신호상태, 수동리셋 방식, IP주소 이벤트
	hNickEvent = CreateEvent(NULL, TRUE, FALSE, NULL);  //비신호상태, 수동리셋 방식, 닉네임 이벤트
	InitializeCriticalSection(&cs);
	InitializeCriticalSection(&cs2);
	if (hWriteEvent == NULL || hServerIPEvent==NULL||hPortNumberEvent == NULL || hNickEvent == NULL)
		return 1;

	//소켓 통신 스레드 생성
	CreateThread(NULL, 0, Sender, NULL, 0, NULL);
	CreateThread(NULL, 0, Receiver, NULL, 0, NULL);

	//대화상자 생성
	DialogBox(hInstance, MAKEINTRESOURCE(IDD_DIALOG1), NULL, DlgProc);

	//이벤트 제거
	CloseHandle(hReadEvent);
	CloseHandle(hWriteEvent);
	CloseHandle(hPortNumberEvent);
	CloseHandle(hServerIPEvent);

	//소켓 닫기
	closesocket(sock);

	//윈속 종료
	WSACleanup();
	DeleteCriticalSection(&cs);
	DeleteCriticalSection(&cs2);
	return 0;
}

//대화상자 프로시저
BOOL CALLBACK DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_INITDIALOG:  //다이얼로그 초기화
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
	case WM_COMMAND:  //버튼을 눌렀을 경우
		switch (LOWORD(wParam))
		{
		case IDC_BUTTON3: // 서버주소 확인
			GetDlgItemText(hDlg, IDC_IPADDRESS1, (LPSTR)ServerIP, sizeof(ServerIP));
			if (CheckIPAddress() == 0) 
				MessageBox(NULL, (LPCSTR)"올바른 서버주소를 입력해주십시오.(사용불가 : 224.0.0.0 ~ 255.255.255.255)", (LPCSTR)"Failed", MB_ICONERROR);
			else
			{
				EnableWindow(GetDlgItem(hDlg, IDC_IPADDRESS1), FALSE);  // IP주소 입력란 비활성화
				EnableWindow(GetDlgItem(hDlg, IDC_BUTTON3), FALSE);  //보내기 버튼 비활성화
				SetEvent(hServerIPEvent);
			}
			return TRUE;
		case IDC_BUTTON1:  // 포트번호
			GetDlgItemText(hDlg, IDC_EDIT4, (LPSTR)PortNumber, sizeof(PortNumber));
			if (CheckPortNumber() == 0)
				MessageBox(NULL, (LPCSTR)"올바른 포트번호를 입력해주십시오. (0~1023 port 사용불가능)", (LPCSTR)"Failed", MB_ICONERROR);
			else
			{
				EnableWindow(GetDlgItem(hDlg, IDC_EDIT4), FALSE);  // 포트번호 입력란 비활성화
				EnableWindow(GetDlgItem(hDlg, IDC_BUTTON1), FALSE);  //보내기 버튼 비활성화
				SetEvent(hPortNumberEvent);
			}
			return TRUE;
		case IDC_BUTTON2: //닉네임 설정
			char temp[NICKSIZE];
			GetDlgItemText(hDlg, IDC_EDIT3, (LPSTR)temp, sizeof(temp));

			if (!CheckNickname(temp))
			{
				MessageBox(NULL, (LPCSTR)"사용불가능한 닉네임입니다.", (LPCSTR)"Failed", MB_ICONERROR);
			}
			else if (strlen(temp) > 10)  //10글자 보다 큰 경우
			{
				MessageBox(NULL, (LPCSTR)"닉네임은 (영문기준)10글자 미만으로 사용하십시오.", (LPCSTR)"Failed", MB_ICONERROR);
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
					MessageBox(NULL, (LPCSTR)"사용불가능한 닉네임입니다.", (LPCSTR)"Failed", MB_ICONERROR);
				}
				ltime = time(NULL);
				localtime_s(&now, &ltime);
				DisplayText("[%02d:%02d:%02d][%s]에서 [%s]로 닉네임 변경시도 요청... \r\n", now.tm_hour, now.tm_min, now.tm_sec, NickName, temp);
				ZeroMemory(Temp, sizeof(Temp));
				strcpy_s(Temp, strlen(temp) + 1, temp);
				nChangeOfNick = 1;
				SetEvent(hWriteEvent);
				SetEvent(hNickEvent);
			}
			return TRUE;
		case IDOK:
			EnableWindow(hSendButton, FALSE);  //보내기 버튼 비활성화
			GetDlgItemText(hDlg, IDC_EDIT1, (LPSTR)buf, BUFSIZE + 1);
			if (!strcmp(buf, "help"))
			{
				DisplayText("채팅프로그램 사용법입니다.\r\n");
				DisplayText("귓속말 사용법 : \" 귓속말 닉네임 내용 \" \r\n");
				DisplayText("채팅창 지우기 : \" /clear \" \r\n");
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
			SetEvent(hWriteEvent);  //buf에 쓰기를 완료했다고 알림.
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
	if (name[0]==NULL)  //아무것도 입력하지 않은경우
		return 0;
	//어떤 문자라도 입력한 경우 아래에서 필터링을 함
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
//소켓 함수 오류 출력 후 종료
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



// 클라이언트와 데이터 통신하는 부분
DWORD WINAPI Receiver(LPVOID arg)
{
	WaitForSingleObject(hServerIPEvent, INFINITE);
	WaitForSingleObject(hPortNumberEvent, INFINITE);
	WaitForSingleObject(hNickEvent, INFINITE);
	char Overlap[NICKSIZE];
	
	int retval;

	//윈속 초기화
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return 1;

	//socket()
	sock = socket(AF_INET, SOCK_STREAM, 0);   //TCP사용
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
		if (!strcmp(Overlap, "중복"))  //닉네임 출력 끝
		{
			OverlapNick = 1;
			MessageBox(NULL, (LPCSTR)"중복된 닉네임입니다. 사용할 수 없습니다.1", (LPCSTR)"Failed", MB_ICONERROR);
			ResetEvent(hNickEvent);
		}
		else
		{
			ZeroMemory(NickName, sizeof(NickName));
			strcpy_s(NickName, strlen(Temp) + 1, Temp);
			OverlapNick = 0;
		}
	}while (OverlapNick == 1);      // 중복된 닉네임이라면 변경을 해야함 그전까지는 메시지를 보내거나 받을수 없음.
	SetEvent(hConnectOnEvent);
	//데이터 통신에 사용할 변수
	char buf[BUFSIZE + 1];
	char name[NICKSIZE];
	int numOfClient = 0;
	//자신의 현재닉네임 출력
	SetWindowTextA(hEdit5, "");
	DisplayNicknameOfClient(NickName);
	//데이터 받기
	while (1)
	{
		//데이터 받기, 닉네임 받는것
		retval = recv(sock, name, NICKSIZE, 0);
		if (retval == SOCKET_ERROR)
		{
			err_display("recv() Nick");
			break;
		}
		if (!strcmp(name, "닉네임"))  //닉네임 출력
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
			DisplayNumberOfClient("(%d명)", numOfClient);
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
		}else if(!strcmp(name, "중복"))    //닉네임 출력 끝
		{
			DisplayText("[%02d:%02d:%02d][%s]에서 [%s]로 닉네임 변경 실패 \r\n", now.tm_hour, now.tm_min, now.tm_sec, NickName, Temp);
			MessageBox(NULL, (LPCSTR)"중복된 닉네임입니다. 사용할 수 없습니다.", (LPCSTR)"Failed", MB_ICONERROR);
			continue;
		}
		else if (!strcmp(name, "노중복"))
		{
			DisplayText("[%02d:%02d:%02d][%s]에서 [%s]로 닉네임 변경 성공 \r\n", now.tm_hour, now.tm_min, now.tm_sec, NickName, Temp);
			ZeroMemory(NickName, sizeof(NickName));
			strcpy_s(NickName, strlen(Temp) + 1, Temp);
			//자신의 현재닉네임 수정
			SetWindowTextA(hEdit5, "");
			DisplayNicknameOfClient(NickName);
			continue;
		}
		if (!strncmp(name, "귓속말", strlen("귓속말")))  // 귓속말이 내게 온 것 From.
		{
			if (!strcmp(name,"귓속말"))
			{
				retval = recv(sock, name, NICKSIZE, 0);
				if (retval == SOCKET_ERROR)
				{
					err_display("recv error() 귓속말 닉네임");
					break;
				}
				ltime = time(NULL);
				localtime_s(&now, &ltime);
				DisplayText("[%02d:%02d:%02d][귓속말]From_%s> ",now.tm_hour,now.tm_min,now.tm_sec,name);
				retval = recv(sock, buf, BUFSIZE, 0);
				if (retval == SOCKET_ERROR)
				{
					err_display("recv error() 귓속말 내용");
					break;
				}
				DisplayText("%s\r\n",buf);
				continue;
			}
			else if (!strcmp(name, "귓속말성공"))      //내가 전송한 귓속말이 성공했음을 출력 To.
			{
				retval = recv(sock, name, NICKSIZE, 0);
				if (retval == SOCKET_ERROR)
				{
					err_display("recv error() 귓속말 닉네임");
					break;
				}
				ltime = time(NULL);
				localtime_s(&now, &ltime);
				DisplayText("[%02d:%02d:%02d][귓속말]To_%s> ", now.tm_hour, now.tm_min, now.tm_sec, name);
				retval = recv(sock, buf, BUFSIZE, 0);
				if (retval == SOCKET_ERROR)
				{
					err_display("recv error() 귓속말 내용");
					break;
				}
				DisplayText("%s\r\n", buf);
				continue;
			}
			else if (!strcmp(name, "귓속말에러"))
			{
				DisplayText("귓속말 형식은 \" 귓속말 닉네임 내용 \"입니다.\r\n");
				continue;
			}
			else if (!strcmp(name, "귓속말닉"))
			{
				DisplayText("상대방이 접속중이지 않습니다.\r\n");
				continue;
			}
			else if (!strcmp(name, "귓속말me"))
			{
				DisplayText("자신에게는 귓속말을 보낼 수 없습니다.\r\n");
				continue;
			}
		}
		//데이터 받기, 내용
		retval = recv(sock, buf, BUFSIZE, 0);
		if (retval == SOCKET_ERROR)
		{
			err_display("recv()2 ");
			break;
		}
		//받은 데이터 출력
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

	EnableWindow(hEdit1, TRUE);  //활성화 시킴
	EnableWindow(hEdit2, TRUE);
	EnableWindow(hEdit3, TRUE);
	EnableWindow(hSendButton, TRUE);

	DisplayText(" chatting start!! Server ip : %s. 포트번호 : %s\r\n 채팅에서 지원하는 명령어를 보실면 help를 입력해주세요.\r\n", ServerIP, PortNumber);

	//데이터 보내기(전송)
	while (1)
	{
		WaitForSingleObject(hWriteEvent, INFINITE); // 쓰기 완료 기다리기
													//문자열 길이가 0이면 보내지 않음
		if (nChangeOfNick == 1) //닉네임 갱신을 서버에게 알림
		{
			retval = send(sock, "닉변", NICKSIZE, 0);
			if (retval == SOCKET_ERROR)
			{
				err_display("send() NickName 갱신실패");
				continue;
			}
			retval = send(sock, Temp, NICKSIZE, 0);
			if (retval == SOCKET_ERROR)
			{
				err_display("send() NickName 갱신실패");
				continue;
			}
			nChangeOfNick = 0;
			continue;
		}
		//데이터 보내기
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
		EnableWindow(hSendButton, TRUE); // 보내기 버튼 활성화

	}

	//closesocket()
	closesocket(sock);

	//윈속 종료
	WSACleanup();
}