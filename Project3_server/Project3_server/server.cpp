#pragma comment(lib, "ws2_32")
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#include "lish.h"
#define SERVERPORT 9000
#define BUFSIZE    512

// 윈도우 프로시저
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
// 편집 컨트롤 출력 함수
void DisplayText(char *fmt, ...);
void RenewalClientList(void);
// 오류 출력 함수
void err_quit(char *msg);
void err_display(char *msg);
// 소켓 통신 스레드 함수
DWORD WINAPI ServerMain(LPVOID arg);
DWORD WINAPI ProcessClient(LPVOID arg);
HINSTANCE hInst; // 인스턴스 핸들
HWND hEdit; // 편집 컨트롤
CRITICAL_SECTION cs; // 임계 영역

//시간 출력 변수
time_t ltime;
struct tm now = { 0 };

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
	LPSTR lpCmdLine, int nCmdShow)
{
	hInst = hInstance;
	InitializeCriticalSection(&cs);

	// 윈도우 클래스 등록
	WNDCLASS wndclass;
	wndclass.style = CS_HREDRAW | CS_VREDRAW;
	wndclass.lpfnWndProc = WndProc;
	wndclass.cbClsExtra = 0;
	wndclass.cbWndExtra = 0;
	wndclass.hInstance = hInstance;
	wndclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndclass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wndclass.lpszMenuName = NULL;
	wndclass.lpszClassName = "MyWndClass";
	if (!RegisterClass(&wndclass)) return 1;

	// 윈도우 생성
	HWND hWnd = CreateWindow("MyWndClass", "TCP 서버", WS_OVERLAPPEDWINDOW,
		0, 0, 800, 500, NULL, NULL, hInstance, NULL);
	if (hWnd == NULL) return 1;
	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	// 소켓 통신 스레드 생성
	CreateThread(NULL, 0, ServerMain, NULL, 0, NULL);

	// 메시지 루프
	MSG msg;
	while (GetMessage(&msg, 0, 0, 0) > 0) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	DeleteCriticalSection(&cs);
	return msg.wParam;
}

// 윈도우 프로시저
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		hEdit = CreateWindow("edit", NULL,
			WS_CHILD | WS_VISIBLE | WS_HSCROLL |
			WS_VSCROLL | ES_AUTOHSCROLL |
			ES_AUTOVSCROLL | ES_MULTILINE | ES_READONLY,
			0, 0, 0, 0, hWnd, (HMENU)100, hInst, NULL);
		return 0;
	case WM_SIZE:
		MoveWindow(hEdit, 0, 0, LOWORD(lParam), HIWORD(lParam), TRUE);
		return 0;
	case WM_SETFOCUS:
		SetFocus(hEdit);
		return 0;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

// 편집 컨트롤 출력 함수
void DisplayText(char *fmt, ...)
{
	va_list arg;
	va_start(arg, fmt);

	char cbuf[BUFSIZE + 256];
	vsprintf_s(cbuf, fmt, arg);

	EnterCriticalSection(&cs);
	int nLength = GetWindowTextLength(hEdit);
	SendMessage(hEdit, EM_SETSEL, nLength, nLength);
	SendMessage(hEdit, EM_REPLACESEL, FALSE, (LPARAM)cbuf);
	LeaveCriticalSection(&cs);

	va_end(arg);
}

// 소켓 함수 오류 출력 후 종료
void err_quit(char *msg)
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	MessageBox(NULL, (LPCTSTR)lpMsgBuf, msg, MB_ICONERROR);
	LocalFree(lpMsgBuf);
	exit(1);
}

// 소켓 함수 오류 출력
void err_display(char *msg)
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	DisplayText("[%s] %s", msg, (char *)lpMsgBuf);
	LocalFree(lpMsgBuf);
}
void RenewalClientList()   // 사용자 목록 최신화
{
	NODE * tmp1;
	NODE * tmp2;
	int retval;
	char name1[NICKSIZE]="닉네임", name2[NICKSIZE];
	
	tmp1 = list->head;
	while (1)              // 사용자 목록 전송
	{
		tmp2 = list->head;
		retval = send(tmp1->data, name1, NICKSIZE, 0);  //닉네임이란 이름 전송
		if (retval == SOCKET_ERROR)
		{
			err_display("send() NickName");
			break;
		}
		_itoa_s(list->numOfClient, name2, 10);  //클라이언트 수를 문자열로 바꾸어서 전송하는 부분
		retval = send(tmp1->data, name2, NICKSIZE, 0);
		if (retval == SOCKET_ERROR)
		{
			err_display("send() Buffer");
			break;
		}
		for (int i = 0; i < list->numOfClient; i++)
		{
			retval = send(tmp1->data, tmp2->name, NICKSIZE, 0);
			if (retval == SOCKET_ERROR)
			{
				err_display("send() NickName");
				break;
			}
			tmp2 = tmp2->next;
		}
		if (tmp1->next == NULL)
			break;
		else
		{
			tmp2 = list->head;
			tmp1 = tmp1->next;
		}
	} //사용자 목록 출력 끝
}

// TCP 서버 시작 부분
DWORD WINAPI ServerMain(LPVOID arg)
{
	int retval;
	InitList();
	// 윈속 초기화
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return 1;
	list->head = NULL;
	// socket()
	SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_sock == INVALID_SOCKET) err_quit("socket()");

	// bind()
	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(SERVERPORT);
	retval = bind(listen_sock, (SOCKADDR *)&serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR) err_quit("bind()");

	// listen()
	retval = listen(listen_sock, SOMAXCONN);
	if (retval == SOCKET_ERROR) err_quit("listen()");

	// 데이터 통신에 사용할 변수
	SOCKET client_sock;
	SOCKADDR_IN clientaddr;
	int addrlen;
	HANDLE hThread;

	while (1) {
		// accept()
		addrlen = sizeof(clientaddr);
		client_sock = accept(listen_sock, (SOCKADDR *)&clientaddr, &addrlen);
		if (client_sock == INVALID_SOCKET) {
			err_display("accept()");
			break;
		}
		// 접속한 클라이언트 정보 출력
		DisplayText("\r\n[TCP 서버] 클라이언트 접속: IP 주소=%s, 포트 번호=%d\r\n",
			inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));

		// 스레드 생성
		hThread = CreateThread(NULL, 0, ProcessClient,
			(LPVOID)client_sock, 0, NULL);
		if (hThread == NULL) { closesocket(client_sock); }
		else { CloseHandle(hThread); }
	}
	// closesocket()
	closesocket(listen_sock);
	free(list);
	// 윈속 종료
	WSACleanup();
	return 0;
}

// 클라이언트와 데이터 통신
DWORD WINAPI ProcessClient(LPVOID arg)
{
	SOCKET client_sock = (SOCKET)arg;
	int retval;
	SOCKADDR_IN clientaddr;
	int addrlen;
	int temp;
	char buf[BUFSIZE + 1];
	char name[NICKSIZE],recvname[NICKSIZE];
	NODE * tmp;
	
	// 클라이언트 정보 얻기
	addrlen = sizeof(clientaddr);

	getpeername(client_sock, (SOCKADDR *)&clientaddr, &addrlen);
	while(1)
	{
		ZeroMemory(name, sizeof(name));
		retval = recv(client_sock, name, NICKSIZE, 0);
		if (retval == SOCKET_ERROR)
		{
			err_display("Nick Recv Error");
			exit(1);
		}
		if (Search(list, name) == 1)
		{
			retval = send(client_sock, "노중복", NICKSIZE, 0);
			if (retval == SOCKET_ERROR)
			{
				err_display("중복체크 recv()");
				break;
			}
			break;
		}
		else
		{
			retval = send(client_sock, "중복", NICKSIZE, 0);
			if (retval == SOCKET_ERROR)
			{
				err_display("중복체크 recv()");
				break;
			}
		}
	}
	
	Insert(client_sock, name);
	list->numOfClient++;
	RenewalClientList();
	
	while (1)
	{
		//데이터 받기, 닉네임
		while (1)
		{
			ZeroMemory(name, sizeof(name));
			retval = recv(client_sock, name, NICKSIZE, 0);
			if (retval == SOCKET_ERROR)
			{
				err_display("recv() Error");
				break;
			}
			if (!strcmp(name, "닉변"))  //닉변 했을 경우(사용자 목록 갱신)
			{
				ZeroMemory(name, sizeof(name));
				retval = recv(client_sock, name, NICKSIZE, 0);  //새로운 닉네임을 받아온다.
				if (retval == SOCKET_ERROR)
				{
					err_display("recv() 닉변");
					break;
				}

				if (Search(list, name) == 1)                     //받아온 닉네임이 중복이 되는지 체크한다.
				{
					retval = send(client_sock, "노중복", NICKSIZE, 0);
					if (retval == SOCKET_ERROR)
					{
						err_display("중복체크 recv()");
						break;
					}
					tmp = list->head;
					while (1)
					{
						if (tmp->data == client_sock)
						{
							ZeroMemory(tmp->name, sizeof(tmp->name));
							strcpy_s(tmp->name, strlen(name)+1,name);
							break;
						}
						else
							tmp = tmp->next;
					}
					RenewalClientList();   //사용자 목록 갱신
					continue;
				}
				else
				{
					retval = send(client_sock, "중복", NICKSIZE, 0);
					if (retval == SOCKET_ERROR)
					{
						err_display("중복체크 recv()");
						break;
					}
				}
				continue;
			}  //닉변 끝
			break;
		}
		if (retval == SOCKET_ERROR)  //연결이 끊겼을 경우 밑에것은 아예 읽지 않도록 한다.
			break;
		//데이터 받기, 내용
		ZeroMemory(buf, sizeof(buf));
		retval = recv(client_sock, buf, BUFSIZE, 0);
		if (retval == SOCKET_ERROR)
		{
			err_display("recv() Error");
			break;
		}
		buf[retval] = '\0';
		if (!strncmp(buf, "귓속말", strlen("귓속말")))           //귓속말을 보내는 경우 
		{
			if (buf[6] == ' ')
			{
				int i = 0;
				while (retval>7+i)                 // 귓속말 대상의 닉네임을 recvname에 복사.
				{
					
					if (buf[7 + i] == ' ')
						break;
					recvname[i] = buf[7 + i];
					i++;
				}
				recvname[i] = '\0';
				if (!strcmp(recvname, name))
				{
					retval = send(client_sock, "귓속말me", NICKSIZE, 0);
					if (retval == SOCKET_ERROR)
					{
						err_display("send() Error for 자기자신");
						break;
					}
					continue;
				}
				if (buf[7+i] == ' ')
				{
					i++;
					temp = 0;
					while (retval > 7+i)   //귓속말의 내용을 buf에 옮겨 담음.
					{
						buf[temp] = buf[7+i];
						temp++;
						i++;
					}
					buf[temp] = '\0';
					
					//귓속말 보내기
					tmp = list->head;
					while (tmp)
					{
						if (!strcmp(recvname, tmp->name))
							break;
						tmp = tmp->next;
					}
					// 상대방이 접속중이지 않을 경우
					if (tmp == NULL) 
					{
						retval = send(client_sock, "귓속말닉", NICKSIZE, 0);
						if (retval == SOCKET_ERROR)
						{
							err_display("send() Error for 닉네임 오류");
							break;
						}
					}
					// 상대방이 접속중일 경우
					else
					{
						//발신자에게 메시지 전송(echo서버 기능)
						retval = send(client_sock, "귓속말성공", NICKSIZE, 0);
						if (retval == SOCKET_ERROR)
						{
							err_display("send() Error for 귓속말 알리기 오류 To.");
							break;
						}
						retval = send(client_sock, recvname, NICKSIZE, 0);
						if (retval == SOCKET_ERROR)
						{
							err_display("send() Error for 귓속말 닉네임 전송오류 To.");
							break;
						}
						retval = send(client_sock, buf, BUFSIZE, 0);
						if (retval == SOCKET_ERROR)
						{
							err_display("send() Error for 귓속말 데이터 전송오류 To.");
							break;
						}
						ltime = time(NULL);
						localtime_s(&now, &ltime);
						DisplayText("[%02d:%02d:%02d][귓속말]To_%s> %s\r\n", now.tm_hour, now.tm_min, now.tm_sec, recvname, buf);
						//수신자에게 메시지 전송
						retval = send(tmp->data, "귓속말", NICKSIZE, 0);
						if (retval == SOCKET_ERROR)
						{
							err_display("send() Error for 귓속말 알리기 오류 From.");
							break;
						}
						retval = send(tmp->data, name, NICKSIZE, 0);
						if (retval == SOCKET_ERROR)
						{
							err_display("send() Error for 귓속말 닉네임 전송오류 From.");
							break;
						}
						retval = send(tmp->data, buf, BUFSIZE, 0);
						if (retval == SOCKET_ERROR)
						{
							err_display("send() Error for 귓속말 데이터 전송오류 From.");
							break;
						}
						ltime = time(NULL);
						localtime_s(&now, &ltime);
						DisplayText("[%02d:%02d:%02d][귓속말]From_%s> %s\r\n", now.tm_hour, now.tm_min, now.tm_sec, name, buf);
					}
				}
				else
				{
					retval = send(client_sock, "귓속말에러", NICKSIZE, 0);
					if (retval == SOCKET_ERROR)
					{
						err_display("send() Error for 귓속말 에러");
						break;
					}
				}

			}
			else
			{
				retval = send(client_sock, "귓속말에러", NICKSIZE, 0);
				if (retval == SOCKET_ERROR)
				{
					err_display("send() Error for 귓속말 에러");
					break;
				}
			}
			continue;
		}
		//받은 데이터 출력
		ltime = time(NULL);
		localtime_s(&now, &ltime);
		DisplayText("[%s][%02d:%02d:%02d]%s \r\n", name, now.tm_hour, now.tm_min, now.tm_sec, buf);

		// 데이터 보내기
		tmp = list->head;
		while (1)
		{
			retval = send(tmp->data, name, NICKSIZE, 0);
			if (retval == SOCKET_ERROR)
			{
				err_display("send() NickName");
				break;
			}
			retval = send(tmp->data, buf, BUFSIZE, 0);
			if (retval == SOCKET_ERROR)
			{
				err_display("send() Buf");
				break;
			}
			if (tmp->next == NULL)
				break;
			else
				tmp = tmp->next;
		}
	}

	// closesocket()
	Delete(client_sock);
	list->numOfClient--;
	closesocket(client_sock);
	DisplayText("[TCP 서버] 클라이언트 종료: IP 주소=%s, 포트 번호=%d\r\n",
		inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
	if (list->numOfClient != 0)
	{
		RenewalClientList();
	}
	return 0;
}