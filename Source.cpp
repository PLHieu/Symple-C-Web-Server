﻿#include <winsock2.h>
#include <windows.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <ws2tcpip.h>
#include <thread>
#include <vector>
#include <chrono>
#include <time.h>
#include <string>

#pragma comment (lib, "ws2_32.lib")

#define DEFAULT_HTTP_PORT "80"

using namespace std;

//void handleClient(SOCKET clientSocket, char* stillwork_flag);
//
//void handleMessage(char* buff);


void handleMessage(char *&buff)
{

	istringstream data(buff);
	vector<string> words((istream_iterator<string>(data)), istream_iterator<string>());

	// chua response
	ostringstream resp;
	// htmlfile_name chua ten cua file tra ve , dung de luu trong response 
	// htmlfile_content chua noi dung
	string htmlfile_name = "";
	string htmlfile_content = "";
	int ReturnCode;

	// lay ten file
	if (htmlfile_name == "/")
	{
		htmlfile_name = "../index.html";
	}
	else {
		htmlfile_name = words[1];
	}

	// can phai xoa dong nay khi hoan thanh xong project
	htmlfile_name.insert(0, "..");

	ifstream f;
	f.open(htmlfile_name);

	// neu nhu mo file khong duoc thi html -> 404.html, mo lai file do
	if (!f.is_open()) {
		htmlfile_name = "../404.html";
		f.open(htmlfile_name);
	}

	// nap du lieu tu trong htmlfile vao trong htmlfile_content
	string str((istreambuf_iterator<char>(f)), istreambuf_iterator<char>());
	htmlfile_content = str;
	ReturnCode = 200;
	f.close();

	if (words[0] == "GET") {
		// response cho phuong thuc GET
		resp << "HTTP/1.1 " << ReturnCode << " OK\r\n";
		resp << "Cache-Control: no-cache, private\r\n";
		//resp << "Last-Modified: " << ;
		resp << "Content-Length: " << htmlfile_content.size() << "\r\n";
		resp << "Content-Type: text/html\r\n";
		resp << "\r\n";
		resp << htmlfile_content;
	}
	else if (words[0] == "POST") {
		// respond cho phuong thuc POST
		resp << "HTTP/1.1 " << ReturnCode << " OK\r\n";
		resp << "Cache-Control: no-cache, private\r\n";
		resp << "Content-Type: text/html\r\n";
		resp << "Content-Length: " << htmlfile_content.size() << "\r\n";
		resp << "\r\n";
		resp << htmlfile_content;
	}
	else {
		// tra lai bad resquest
	}

	cout << resp.str();
	// dua lai response vao trong buff
	strcpy(buff, resp.str().c_str());
}

void handleClient(SOCKET clientSocket, char* stillwork_flag)
{
	//transmit file
	int buffsize = 10000;
	char* buff = new char[10000];
	int r;
	do {
		r = recv(clientSocket, buff, buffsize, 0);
		if (r > 0)
		{
			cout << "received: " << buff << endl;
			handleMessage(buff);

			int v = send(clientSocket, buff, strlen(buff), 0);
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

//void wait(int* miliseconds, char* is_timeout);
void wait(int* miliseconds, char* is_timeout)
{
	do {
		std::this_thread::sleep_for(chrono::milliseconds(100));
		miliseconds -= 100;
	} while (*miliseconds <= 0);
	*is_timeout = 1;
}

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


