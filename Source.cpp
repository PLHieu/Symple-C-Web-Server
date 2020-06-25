#define _CRT_SECURE_NO_WARNINGS
#include <winsock2.h>
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
#include <errno.h>

#pragma comment (lib, "ws2_32.lib")

#define DEFAULT_HTTP_PORT "80"

using namespace std;

int get_ext(char* file) {
	if (strstr(file, ".jpg") != NULL)
		return 1;
	if (strstr(file, ".css") != NULL)
		return 2;
	if (strstr(file, ".html") != NULL)
		return 3;
	return 4; // cac loai file con lai
}

int handleMessage(char*& buff, int& buffsize)
{
	char temp[100];
	memcpy(temp, buff, 100);

	// chua response
	ostringstream resp;

	char* requestType = strtok(temp, " ");
	char* filename = strtok(NULL, " ");
	int ReturnCode = 200;
	FILE* f = NULL; // file

	if (strcmp(requestType, "GET") == 0) {
		// lay ten file
		if (strcmp(filename, "/") == 0)
		{
			strcpy(filename, "/index.html");
		}

		 for (int i = strlen(filename); i >= 0; i--) {
		 	filename[i + 2] = filename[i];
		 }
		 filename[0] = '.';
		 filename[1] = '.';

		// mo file duo dang nhi phan
		f = fopen(filename, "rb");

		// neu nhu mo file khong duoc thi  -> 404.html
		if (f == NULL) {
			//cout << strerror(errno); 
			ReturnCode = 404;
			strcpy(filename, "../404.html");
			f = fopen(filename, "rb");
		}
	}
	else if (strcmp(requestType, "POST") == 0) {

		char username[5];
		char password[5];
		// xac thuc username va password;
		char* pos_username = strstr(buff, "username=");
		memcpy(username, pos_username + 9, 5);
		char* pos_password = strstr(buff, "&password=");
		memcpy(password, pos_password + 10, 5);

		if (strncmp(username, "admin", 5) == 0 && strncmp(password, "admin", 5) == 0) {
			strcpy(filename, "../info.html");
			ReturnCode = 301;

			resp << "HTTP/1.1 " << ReturnCode << " Moved Permanently\r\n";
			resp << "Location: " << filename << "\r\n";
			resp << "Content-Transfer-Encoding: binary\r\n";
			resp << "Content-Length: " << 0 << "\r\n";
			resp << "\r\n";
			// nap du lieu vao trong buff
			strcpy(buff, resp.str().c_str());
			buffsize = strlen(buff);
			return 0;
		}
		else {
			strcpy(filename, "../404.html");
			ReturnCode = 404;
		}

		f = fopen(filename, "rb");
	}
	else
	{
		return -1;
	}

	// tinh toan length cua file 
	fseek(f, 0, SEEK_END);
	int fileLength = ftell(f);
	rewind(f);
	 
	// nap header cho response
	if (ReturnCode == 404) {
		resp << "HTTP/1.1 404 Not Found \r\n";
	}
	else {
		resp << "HTTP/1.1 " << ReturnCode << " OK\r\n";
	}
	resp << "Cache-Control: no-cache, private\r\n";

	// kiem tra loai file
	//int ext = get_ext(file_name.c_str());
	int ext = get_ext(filename);

	// nao header - phan content type
	switch (ext) {
	case 1:
		resp << "Content-Type: image/jpg\r\n";
		break;
	case 2:
		resp << "Content-Type: text/css\r\n";
		break;
	case 3:
		resp << "Content-Type: text/html\r\n";
		break;
	}

	resp << "Content-Transfer-Encoding: binary\r\n";
	resp << "Content-Length: " << fileLength << "\r\n";
	resp << "\r\n";

	// nap du lieu vao trong buff
	strcpy(buff, resp.str().c_str());

	// tinh toan kich thuoc cua buff luc nay
	int headerLength = strlen(buff);

	char* bufftemp = new char[fileLength];
	fread(bufftemp, sizeof(char), fileLength, f);
	memcpy(buff + headerLength, bufftemp, fileLength);

	// tra ve size cua buffer
	buffsize = headerLength + fileLength;
		
	// giai phong vung nho, dong file
	fclose(f);
	delete[] bufftemp;
	return 0;
}

void handleClient(SOCKET clientSocket, char* stillwork_flag)
{
	//cout << "Connection established at socket: "<<clientSocket << endl;
	//transmit file
	int buffsize = 3000000;
	char* buff = new char[buffsize];
	int r;
	do {
		r = recv(clientSocket, buff, buffsize, 0);
		if (r > 0)
		{
			//cout << "\nreceived: " << buff << endl;
			int hmr = handleMessage(buff, buffsize);
			int v = 0;
			if (hmr==0)
			{
				v = send(clientSocket, buff, buffsize, 0);
			}
			if (v == SOCKET_ERROR)
			{
				cout << "\nsend failed" << endl;
			}
			/*else
			{
				cout << "sent: " << buff << endl;
			}*/
		}
		else if (r == 0)
		{
			//cout << "Connection at socket "<<clientSocket<<" closing" << endl;
		}
		else//r<0
		{
			cout << "receive fail" << endl;
			closesocket(clientSocket);
			*stillwork_flag = false;
			delete[]buff;
			return;
		}
	} while (r > 0);

	//done, shutdown connection
	int k = shutdown(clientSocket, SD_SEND);
	if (k == SOCKET_ERROR)
	{
		cout << "Shutdown connection at socket "<<clientSocket<<" got error!" << endl;
	}
	//cout << "Connection at socket "<<clientSocket<<" closed" << endl;

	closesocket(clientSocket);
	*stillwork_flag = 0;
	delete[]buff;
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
	//CHANGE SERVER IP ADDRESS HERE
	res_adinf->ai_addr->sa_data[2] = 127;
	res_adinf->ai_addr->sa_data[3] = 0;
	res_adinf->ai_addr->sa_data[4] = 0;
	res_adinf->ai_addr->sa_data[5] = 1;
	k = bind(listenSocket, res_adinf->ai_addr, (int)res_adinf->ai_addrlen);
	if (k == SOCKET_ERROR)
	{
		cout << "Bind socket with address fail. Error: " << WSAGetLastError() << endl;
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
	cout << "Server started!" << endl;

	SOCKET clientSocket;
	vector<char*>stillwork_flag_ptr;//0 or 1
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
