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

//void handleClient(SOCKET clientSocket, char* stillwork_flag);
//
//void handleMessage(char* buff);


int get_ext(const char* file) {
	if (strstr(file, ".jpg") != NULL)
		return 1;
	if (strstr(file, ".css") != NULL)
		return 2;
	return 3; // cac loai file con lai
}

void handleMessage(char*& buff, int &buffsize)
{

	istringstream data(buff);
	vector<string> words((istream_iterator<string>(data)), istream_iterator<string>());

	// chua response
	ostringstream resp;
	// file_name chua ten cua file tra ve , dung de luu trong response 
	string file_name = "";
	int ReturnCode = 200;
	FILE *f = NULL ; // file

	if (words[0] == "GET") {
		// lay ten file
		file_name = words[1];
		if (file_name == "/")
		{	
			file_name = "/index.html";
		}

		file_name.insert(0,"..");

		// mo file duo dang nhi phan
		f = fopen(file_name.c_str(), "rb");

		// neu nhu mo file khong duoc thi  -> 404.html, mo lai file do
		if (f == NULL) {
			//cout << strerror(errno); 
			ReturnCode = 404;
			file_name = "../404.html";
			f = fopen(file_name.c_str(), "rb");
		}
	}
	else if (words[0] == "POST") {

		string request_str = data.str();

		// xac thuc username va password
		int pos_username = request_str.find("username");
		int pos_end = request_str.find("Í");
		int pos_password = request_str.find("password");

		string username = request_str.substr(pos_username + 9, pos_password - pos_username - 10);
		string pass = request_str.substr(pos_password + 9, pos_end - pos_password - 9);

		if (username == "admin" && pass == "admin") {
			file_name = "../info.html";
		}
		else {
			file_name = "../404.html";
		}

		f = fopen(file_name.c_str(), "rb");

	}

	// kiem tra loai file
	int ext = get_ext(file_name.c_str());

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

	// nao header - phan content type
	switch (ext) {
		case 1:
			resp << "Content-Type: image/jpg\r\n";
			break;
		case 2:
			resp << "Content-Type: text/css\r\n";
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
	fread(bufftemp, sizeof(char),  fileLength, f);
	memcpy(buff + headerLength, bufftemp, fileLength);

	// tra ve size cua buffer
	buffsize = headerLength + fileLength;

	// giai phong vung nho, dong file
	delete[] bufftemp;
	fclose(f);
}

void handleMessage1(char*& buff) {
	istringstream data(buff);
	vector<string> words((istream_iterator<string>(data)), istream_iterator<string>());

	// chua response
	ostringstream resp;
	// file_name chua ten cua file tra ve , dung de luu trong response 
	string file_name = "";
	int ReturnCode;
	ifstream f;

	if (words[0] == "GET") {
		// lay ten file
		file_name = words[1];
		if (file_name == "/")
		{	
			file_name = "/index.html";
		}

		file_name.insert(0,"..");
		 //const char* a = file_name.c_str() +1;
		// mo file duo dang nhi phan
		f.open(file_name,ifstream::binary);

		// neu nhu mo file khong duoc thi  -> 404.html, mo lai file do
		if (!f.is_open()) {
			cout << strerror(errno); 
			file_name = "../404.html";
			f.open(file_name,ifstream::binary);
		}
	}
	//else if (words[0] == "POST") {

	//	string request_str = data.str();

	//	// xac thuc username va password
	//	int pos_username = request_str.find("username");
	//	int pos_end = request_str.find("Í");
	//	int pos_password = request_str.find("password");

	//	string username = request_str.substr(pos_username + 9, pos_password - pos_username - 10);
	//	string pass = request_str.substr(pos_password + 9, pos_end - pos_password - 9);

	//	if (username == "admin" && pass == "admin") {
	//		file_name = "../info.html";
	//	}
	//	else {
	//		file_name = "../404.html";
	//	}

	//	f = fopen(file_name.c_str(), "rb");

	//}

	ReturnCode = 200;

	//// nap header truoc
	//// response cho phuong thuc GET
	//resp << "HTTP/1.1 " << ReturnCode << " OK\r\n";
	//resp << "Cache-Control: no-cache, private\r\n";
	//////resp << "Last-Modified: " << ;
	////	// check xem co phai la anh hay khong 
	//if (file_name == "../hvh.jpg" or file_name == "../plh.jpg") {
	//	resp << "Content-Type: image/jpg\r\n";
	//}
	//resp << "Content-Transfer-Encoding: binary\r\n";
	////resp << "Connection: close\r\n";
	//resp << "Content-Length: " << fileLength << "\r\n";
	////resp << "Content-Type: text/html\r\n";
	//resp << "\r\n";

	//// dua header response vao trong buff
	//strcpy(buff, resp.str().c_str());

	// tinh toan do dai cua file 
	f.seekg(0, ios::end);
	size_t length = f.tellg();
	f.seekg(0, ios::beg);


	// dua du lieu vao trong buff
	char* bufftemp = new char[length];
	*bufftemp = {0};
	f.read(bufftemp, length ) ;

	f.close();

}

void handleClient(SOCKET clientSocket, char* stillwork_flag)
{
	//transmit file
	int buffsize = 200000;
	char* buff = new char[buffsize];
	int r;
	do {
		r = recv(clientSocket, buff, buffsize, 0);
		if (r > 0)
		{
			//r = recv(clientSocket, buff, buffsize, 0);

			cout << "received: " << buff << endl;
			handleMessage(buff, buffsize);

			int v = send(clientSocket, buff, buffsize, 0);
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
			delete[]buff;
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

	delete[]buff;
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

	//FILE* f = fopen("../plh.jpg", "r+b");
	//fseek(f, 0, SEEK_END);
	//int fileLength = ftell(f);
	//rewind(f);
	//char* buffer  = (char*)malloc(sizeof(char) * fileLength);
	//int b = sizeof(char);
	//int a = fread(buffer, 1,  fileLength, f);
	//fclose(f);

	//free(buffer);

	//ifstream fin("../plh.jpg");
	//ostringstream ostrm;
	//ostrm << fin.rdbuf();
	//cout << fin.rdbuf();
	////cout << ostrm;

	//const string tmp = ostrm.str();

	//char* cstr = new char [2303932]; 
	//memcpy(cstr, tmp.c_str(), tmp.size());
	////
	////for (size_t i = 0; i < 4; i++)
	////{
	////	char c = tmp[i];
	////	printf("%c", &c);
	////}

	//char c = buffer[74];

	//free(cstr);
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

		cout <<"###############################\n############\n#####################" <<stillwork_flag_ptr.size()<<"\n#########################3\n" << endl;
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


