#pragma comment(lib, "ws2_32")
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#include "lish.h"
#define SERVERPORT 9000
#define BUFSIZE    512

// ������ ���ν���
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
// ���� ��Ʈ�� ��� �Լ�
void DisplayText(char *fmt, ...);
void RenewalClientList(void);
// ���� ��� �Լ�
void err_quit(char *msg);
void err_display(char *msg);
// ���� ��� ������ �Լ�
DWORD WINAPI ServerMain(LPVOID arg);
DWORD WINAPI ProcessClient(LPVOID arg);
HINSTANCE hInst; // �ν��Ͻ� �ڵ�
HWND hEdit; // ���� ��Ʈ��
CRITICAL_SECTION cs; // �Ӱ� ����

//�ð� ��� ����
time_t ltime;
struct tm now = { 0 };

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
	LPSTR lpCmdLine, int nCmdShow)
{
	hInst = hInstance;
	InitializeCriticalSection(&cs);

	// ������ Ŭ���� ���
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

	// ������ ����
	HWND hWnd = CreateWindow("MyWndClass", "TCP ����", WS_OVERLAPPEDWINDOW,
		0, 0, 800, 500, NULL, NULL, hInstance, NULL);
	if (hWnd == NULL) return 1;
	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	// ���� ��� ������ ����
	CreateThread(NULL, 0, ServerMain, NULL, 0, NULL);

	// �޽��� ����
	MSG msg;
	while (GetMessage(&msg, 0, 0, 0) > 0) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	DeleteCriticalSection(&cs);
	return msg.wParam;
}

// ������ ���ν���
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

// ���� ��Ʈ�� ��� �Լ�
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

// ���� �Լ� ���� ��� �� ����
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

// ���� �Լ� ���� ���
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
void RenewalClientList()   // ����� ��� �ֽ�ȭ
{
	NODE * tmp1;
	NODE * tmp2;
	int retval;
	char name1[NICKSIZE]="�г���", name2[NICKSIZE];
	
	tmp1 = list->head;
	while (1)              // ����� ��� ����
	{
		tmp2 = list->head;
		retval = send(tmp1->data, name1, NICKSIZE, 0);  //�г����̶� �̸� ����
		if (retval == SOCKET_ERROR)
		{
			err_display("send() NickName");
			break;
		}
		_itoa_s(list->numOfClient, name2, 10);  //Ŭ���̾�Ʈ ���� ���ڿ��� �ٲپ �����ϴ� �κ�
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
	} //����� ��� ��� ��
}

// TCP ���� ���� �κ�
DWORD WINAPI ServerMain(LPVOID arg)
{
	int retval;
	InitList();
	// ���� �ʱ�ȭ
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

	// ������ ��ſ� ����� ����
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
		// ������ Ŭ���̾�Ʈ ���� ���
		DisplayText("\r\n[TCP ����] Ŭ���̾�Ʈ ����: IP �ּ�=%s, ��Ʈ ��ȣ=%d\r\n",
			inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));

		// ������ ����
		hThread = CreateThread(NULL, 0, ProcessClient,
			(LPVOID)client_sock, 0, NULL);
		if (hThread == NULL) { closesocket(client_sock); }
		else { CloseHandle(hThread); }
	}
	// closesocket()
	closesocket(listen_sock);
	free(list);
	// ���� ����
	WSACleanup();
	return 0;
}

// Ŭ���̾�Ʈ�� ������ ���
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
	
	// Ŭ���̾�Ʈ ���� ���
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
			retval = send(client_sock, "���ߺ�", NICKSIZE, 0);
			if (retval == SOCKET_ERROR)
			{
				err_display("�ߺ�üũ recv()");
				break;
			}
			break;
		}
		else
		{
			retval = send(client_sock, "�ߺ�", NICKSIZE, 0);
			if (retval == SOCKET_ERROR)
			{
				err_display("�ߺ�üũ recv()");
				break;
			}
		}
	}
	
	Insert(client_sock, name);
	list->numOfClient++;
	RenewalClientList();
	
	while (1)
	{
		//������ �ޱ�, �г���
		while (1)
		{
			ZeroMemory(name, sizeof(name));
			retval = recv(client_sock, name, NICKSIZE, 0);
			if (retval == SOCKET_ERROR)
			{
				err_display("recv() Error");
				break;
			}
			if (!strcmp(name, "�к�"))  //�к� ���� ���(����� ��� ����)
			{
				ZeroMemory(name, sizeof(name));
				retval = recv(client_sock, name, NICKSIZE, 0);  //���ο� �г����� �޾ƿ´�.
				if (retval == SOCKET_ERROR)
				{
					err_display("recv() �к�");
					break;
				}

				if (Search(list, name) == 1)                     //�޾ƿ� �г����� �ߺ��� �Ǵ��� üũ�Ѵ�.
				{
					retval = send(client_sock, "���ߺ�", NICKSIZE, 0);
					if (retval == SOCKET_ERROR)
					{
						err_display("�ߺ�üũ recv()");
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
					RenewalClientList();   //����� ��� ����
					continue;
				}
				else
				{
					retval = send(client_sock, "�ߺ�", NICKSIZE, 0);
					if (retval == SOCKET_ERROR)
					{
						err_display("�ߺ�üũ recv()");
						break;
					}
				}
				continue;
			}  //�к� ��
			break;
		}
		if (retval == SOCKET_ERROR)  //������ ������ ��� �ؿ����� �ƿ� ���� �ʵ��� �Ѵ�.
			break;
		//������ �ޱ�, ����
		ZeroMemory(buf, sizeof(buf));
		retval = recv(client_sock, buf, BUFSIZE, 0);
		if (retval == SOCKET_ERROR)
		{
			err_display("recv() Error");
			break;
		}
		buf[retval] = '\0';
		if (!strncmp(buf, "�ӼӸ�", strlen("�ӼӸ�")))           //�ӼӸ��� ������ ��� 
		{
			if (buf[6] == ' ')
			{
				int i = 0;
				while (retval>7+i)                 // �ӼӸ� ����� �г����� recvname�� ����.
				{
					
					if (buf[7 + i] == ' ')
						break;
					recvname[i] = buf[7 + i];
					i++;
				}
				recvname[i] = '\0';
				if (!strcmp(recvname, name))
				{
					retval = send(client_sock, "�ӼӸ�me", NICKSIZE, 0);
					if (retval == SOCKET_ERROR)
					{
						err_display("send() Error for �ڱ��ڽ�");
						break;
					}
					continue;
				}
				if (buf[7+i] == ' ')
				{
					i++;
					temp = 0;
					while (retval > 7+i)   //�ӼӸ��� ������ buf�� �Ű� ����.
					{
						buf[temp] = buf[7+i];
						temp++;
						i++;
					}
					buf[temp] = '\0';
					
					//�ӼӸ� ������
					tmp = list->head;
					while (tmp)
					{
						if (!strcmp(recvname, tmp->name))
							break;
						tmp = tmp->next;
					}
					// ������ ���������� ���� ���
					if (tmp == NULL) 
					{
						retval = send(client_sock, "�ӼӸ���", NICKSIZE, 0);
						if (retval == SOCKET_ERROR)
						{
							err_display("send() Error for �г��� ����");
							break;
						}
					}
					// ������ �������� ���
					else
					{
						//�߽��ڿ��� �޽��� ����(echo���� ���)
						retval = send(client_sock, "�ӼӸ�����", NICKSIZE, 0);
						if (retval == SOCKET_ERROR)
						{
							err_display("send() Error for �ӼӸ� �˸��� ���� To.");
							break;
						}
						retval = send(client_sock, recvname, NICKSIZE, 0);
						if (retval == SOCKET_ERROR)
						{
							err_display("send() Error for �ӼӸ� �г��� ���ۿ��� To.");
							break;
						}
						retval = send(client_sock, buf, BUFSIZE, 0);
						if (retval == SOCKET_ERROR)
						{
							err_display("send() Error for �ӼӸ� ������ ���ۿ��� To.");
							break;
						}
						ltime = time(NULL);
						localtime_s(&now, &ltime);
						DisplayText("[%02d:%02d:%02d][�ӼӸ�]To_%s> %s\r\n", now.tm_hour, now.tm_min, now.tm_sec, recvname, buf);
						//�����ڿ��� �޽��� ����
						retval = send(tmp->data, "�ӼӸ�", NICKSIZE, 0);
						if (retval == SOCKET_ERROR)
						{
							err_display("send() Error for �ӼӸ� �˸��� ���� From.");
							break;
						}
						retval = send(tmp->data, name, NICKSIZE, 0);
						if (retval == SOCKET_ERROR)
						{
							err_display("send() Error for �ӼӸ� �г��� ���ۿ��� From.");
							break;
						}
						retval = send(tmp->data, buf, BUFSIZE, 0);
						if (retval == SOCKET_ERROR)
						{
							err_display("send() Error for �ӼӸ� ������ ���ۿ��� From.");
							break;
						}
						ltime = time(NULL);
						localtime_s(&now, &ltime);
						DisplayText("[%02d:%02d:%02d][�ӼӸ�]From_%s> %s\r\n", now.tm_hour, now.tm_min, now.tm_sec, name, buf);
					}
				}
				else
				{
					retval = send(client_sock, "�ӼӸ�����", NICKSIZE, 0);
					if (retval == SOCKET_ERROR)
					{
						err_display("send() Error for �ӼӸ� ����");
						break;
					}
				}

			}
			else
			{
				retval = send(client_sock, "�ӼӸ�����", NICKSIZE, 0);
				if (retval == SOCKET_ERROR)
				{
					err_display("send() Error for �ӼӸ� ����");
					break;
				}
			}
			continue;
		}
		//���� ������ ���
		ltime = time(NULL);
		localtime_s(&now, &ltime);
		DisplayText("[%s][%02d:%02d:%02d]%s \r\n", name, now.tm_hour, now.tm_min, now.tm_sec, buf);

		// ������ ������
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
	DisplayText("[TCP ����] Ŭ���̾�Ʈ ����: IP �ּ�=%s, ��Ʈ ��ȣ=%d\r\n",
		inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
	if (list->numOfClient != 0)
	{
		RenewalClientList();
	}
	return 0;
}