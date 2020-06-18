#include <winsock2.h>
#include <windows.h>
#include <iostream>
#include <fstream>
#include <ws2tcpip.h>
#include <thread>
#include <vector>
#include <chrono>
#include <time.h>

#pragma comment (lib, "ws2_32.lib")

#define DEFAULT_HTTP_PORT "80"

void handleClient(SOCKET clientSocket, char* stillwork_flag);
void handleMessage(char* buff);
void wait(int* miliseconds, char* is_timeout);

using namespace std;
int main(int argc, char* argv[])
{
	WSADATA wd;
	int k = WSAStartup(MAKEWORD(2, 2), &wd);
	addrinfo hint_adinf;
	addrinfo* res_adinf = NULL;

	if (k != 0)
	{
		cout << "WSAStartup fail" << endl;
		return 0;
	}
	
	//get address information
	ZeroMemory(&hint_adinf, sizeof(hint_adinf));
	hint_adinf.ai_family = AF_INET;
	hint_adinf.ai_socktype = SOCK_STREAM;
	hint_adinf.ai_protocol = IPPROTO_TCP;
	//adinf.ai_flags = AI_PASSIVE;
	k = getaddrinfo(NULL, DEFAULT_HTTP_PORT, &hint_adinf, &res_adinf);
	if (k != 0)
	{
		cout << "Error getting address information" << endl;
		WSACleanup();
		return 0;
	}

	SOCKET listenSocket = NULL;
	listenSocket = socket(res_adinf->ai_family, res_adinf->ai_socktype, res_adinf->ai_protocol);
	if (listenSocket == INVALID_SOCKET)
	{
		cout << "Listen socket fail. Error: " << WSAGetLastError() << endl;
		WSACleanup();
		return 0;
	}
	
	k = bind(listenSocket, res_adinf->ai_addr, (int)res_adinf->ai_addrlen);
	if (k == SOCKET_ERROR)
	{
		cout << "Bind socket with address fail. Error: "<< WSAGetLastError() << endl;
		freeaddrinfo(res_adinf);
		closesocket(listenSocket);
		WSACleanup();
		return 0;
	}
	freeaddrinfo(res_adinf);

	k = listen(listenSocket, SOMAXCONN);
	if (k == SOCKET_ERROR)
	{
		cout << "Listen fail. Error: " << WSAGetLastError() << endl;
		closesocket(listenSocket);
		WSACleanup();
		return 0;
	}
	//vector<SOCKET> clientSockets;
	SOCKET clientSocket;
	//vector<thread*> handleFunctions;
	vector<char*>stillwork_flag_ptr;//0 or 1
	
	//int time_wait = 30000;
	//int tclock = time_wait;
	//char is_timeout = 0;
	//thread do_wait(wait, &tclock, &is_timeout);
	do
	{
		clientSocket = accept(listenSocket, NULL, NULL);
		if (clientSocket == INVALID_SOCKET)
		{
			cout << "Connect request detected! But fail to connect to this client" << endl;
		}
		//clientSockets.push_back(clientSocket);
		char* c_ptr = new char;
		*c_ptr = 1;
		stillwork_flag_ptr.push_back(c_ptr);
		thread(handleClient, clientSocket, c_ptr).detach();
		
		//delete closed thread
		int i = 0;
		for (; i < int(stillwork_flag_ptr.size());)
		{
			if (*stillwork_flag_ptr[i] == 0)//thread i no longer work
			{
				delete stillwork_flag_ptr[i];
				stillwork_flag_ptr.erase(stillwork_flag_ptr.begin() + i);
				break;
			}
			else
			{
				i++;
			}
		}

		/*if (stillwork_flag_ptr.size() == 0)
		{
			if (is_timeout)
			{
				do_wait.join();
				break;
			}
			cout << "tclock: "<<tclock << endl;
		}
		else
		{
			cout << "Number of clients: "<<stillwork_flag_ptr.size() << endl;
			tclock = time_wait;
		}*/

	} while (1);
	
	int i = 0;
	for (; i < int(stillwork_flag_ptr.size());)
	{
		if (*stillwork_flag_ptr[i] == 0)//thread i no longer work
		{
			delete stillwork_flag_ptr[i];
			stillwork_flag_ptr.erase(stillwork_flag_ptr.begin() + i);
			break;
		}
		else
		{
			i++;
		}
	}
	
	cout << "Server stopped!" << endl;
	closesocket(listenSocket);
	WSACleanup();
	return 0;
}

void handleClient(SOCKET clientSocket, char* stillwork_flag)
{
	//transmit file
	int buffsize = 10000;
	char buff[10000];
	int r;
	do {
		r = recv(clientSocket, buff, buffsize, 0);
		if (r > 0)
		{
			cout << "received: " << buff << endl;
			
			handleMessage(buff);

			int v = send(clientSocket, buff, r, 0);
			if (v == SOCKET_ERROR)
			{
				cout << "send failed" << endl;
			}
			else
			{
				cout << "sent: " << buff << endl;
			}
		}
		else if (r == 0)
		{
			cout << "Connection closing" << endl;
		}
		else//r<0
		{
			cout << "receive fail" << endl;
			closesocket(clientSocket);
			*stillwork_flag = false;
			return;
		}
	} while (r > 0);
	
	//done, shutdown connection
	int k = shutdown(clientSocket, SD_SEND);
	if (k == SOCKET_ERROR)
	{
		cout << "Shutdown connection error! force to close..." << endl;
	}

	closesocket(clientSocket);
	*stillwork_flag = 0;
}

void handleMessage(char* buff)
{

	const char filename[] = "../../index.html";
	string messtemp;
	ifstream filetoreturn;
	filetoreturn.open(filename);
	if (!filetoreturn.is_open())
	{
		return;
	}

	
	int i = 0;
	while (!filetoreturn.eof())
	{
		filetoreturn.read(buff + i, 1);
		i++;
	}

	//get time
	char buff_time[100];
	time_t t;
	const time_t* const ct = &t;
	tm tmr;
	tm* const tl = &tmr;
	time(&t);
	localtime_s(tl, ct);
	strftime(buff_time, 100, "Thu, %d %m %Y %H:%M:%S", tl);
	string str_time = "Date: " + string(buff_time) + "\n";

	int len = i--;
	char str_num[10];
	_itoa_s(len, str_num, 10);
	int nline = 8;
	string line[10] = {
		"HTTP/1.1 200 OK\n",
		"Connection: keep-alive\n",
		str_time,
		"Server: PServer/1.0\n",
		"Last-Modified: Thu, 18 June 2020 16:22:29\n",
		"Content-Length: " + string(str_num) +"\n",
		"Content-Type: text/html\n"
		"\n"
	};

	int k = 0;
	for (i = 0; i < nline; i++)
	{
		for (int j = 0; j < int(line[i].size()); j++)
		{
			buff[k] = line[i][j];
			k++;
		}
	}
	filetoreturn.close();
	filetoreturn.open(filename);
	if (!filetoreturn.is_open())
	{
		return;
	}
	for (int i = 0; i < len; i++)
	{
		filetoreturn.read(buff + k, 1);
		k++;
	}

	buff[--k] = 0;
	filetoreturn.close();
}

void wait(int* miliseconds, char* is_timeout)
{
	do {
		std::this_thread::sleep_for(chrono::milliseconds(100));
		miliseconds -= 100;
	} while (*miliseconds <= 0);
	*is_timeout = 1;
}
