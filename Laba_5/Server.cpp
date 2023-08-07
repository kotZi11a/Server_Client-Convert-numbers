#include <iostream>
#include <winsock2.h>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <thread>
#include <sstream>

#pragma comment(lib, "ws2_32.lib")

using namespace std;

HANDLE event = CreateEventW(
	NULL,
	FALSE,
	TRUE,
	L"Global\\Event"
);

HANDLE semafor = CreateSemaphoreA(
	NULL,
	3,
	3,
	NULL
);

HANDLE semafor_client = CreateSemaphoreW(
	NULL,
	3,
	3,
	L"Global\\Sem"
);

condition_variable g_cv;
vector<SOCKET> g_clients;
char buf_num[1024];
char buf_sys[10];
stringstream ss;


void printDisconnect(SOCKET clientSocket) 
{
	cout << "~User[" << clientSocket << "] disconnected!\n";
}

void printConnect(SOCKET clientSocket) 
{
	cout << "~User[" << clientSocket << "] connected!\n";
	string socket = to_string((int)clientSocket);
	send(clientSocket, socket.c_str(), socket.size(), 0);
}

string convertNum(int number, char sys)
{
	string res;
	if (buf_sys[0] == '2')
	{
		while (number / 2 != 0)
		{
			int ost = number % 2;
			res.append(to_string(ost));
			number /= 2;
		}
		res.append(to_string(number));
		reverse(res.begin(), res.end());
	}
	else
	{
		while (number / 16 != 0)
		{
			int ost = number % 16;
			if (ost < 10)
			{
				res.append(to_string(ost));
			}
			else
			{
				res += (char)(ost - 10) + 'A';
			}
			number /= 16;
		}
		if (number < 10)
		{
			res.append(to_string(number));
		}
		else
		{
			res += (char)(number - 10) + 'A';
		}
		reverse(res.begin(), res.end());
	}
	return res;
}

void HandleClient(SOCKET clientSocket)
{

	// Add the client to the list
	{
		g_clients.push_back(clientSocket);
		printConnect(clientSocket);
	}


	while (true) {
		num_ac:
		int counter_buf = recv(clientSocket, buf_num, sizeof(buf_num), 0);
		if (counter_buf == SOCKET_ERROR) {

			cout << "recieve failed: " << WSAGetLastError() << endl;
			break;
		}
		string num = buf_num;
		if (num == "--wait")
		{
			SetEvent(event);
			ResetEvent(event);
			ReleaseSemaphore(semafor_client, 1, NULL);
			ReleaseSemaphore(semafor, 1, NULL);
			cout << "Wait....\n";
			WaitForSingleObject(event, INFINITE);
			SetEvent(event);
			cout << "Continue client[" << clientSocket << "]: \n";
			goto num_ac;
		}
		
		int number = 0;
		for (int i = counter_buf - 1, k = 0; i > -1; i--, k++)
		{
			number += (buf_num[i] - '0') * pow(10, k);
		}
		cout << "Number: " << number << endl;

		counter_buf = recv(clientSocket, buf_sys, sizeof(buf_sys), 0);
		if (counter_buf == SOCKET_ERROR) {

			cout << "recieve failed: " << WSAGetLastError() << endl;
			
			break;
		}

		string res = convertNum(number, buf_sys[0]);
		cout << "Result: " << res << endl;

		counter_buf = send(clientSocket, res.c_str(), res.size(), 0);
		if (counter_buf == SOCKET_ERROR) {
			cout << "send failed: " << WSAGetLastError() << endl;
			break;
		}

	}

	{

		g_clients.erase(std::remove(g_clients.begin(), g_clients.end(), clientSocket), g_clients.end());
		printDisconnect(clientSocket);
		ReleaseSemaphore(semafor_client, 1, NULL);
		ReleaseSemaphore(semafor, 1, NULL);
		if (g_clients.size() == 1)\
			SetEvent(event);
	}

	closesocket(clientSocket);
}

int main()
{
	// Initialize Winsock
	WSADATA wsaData;
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		cout << "WSAStartup failed: " << iResult << std::endl;
		return 1;
	}

	// Create a socket for listening
	SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (listenSocket == INVALID_SOCKET) {
		cout << "socket failed: " << WSAGetLastError() << std::endl;
		WSACleanup();
		return 1;
	}

	// Bind the socket to a specific IP address and port
	sockaddr_in serverAddress;
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_addr.s_addr = INADDR_ANY;
	serverAddress.sin_port = htons(8080);

	iResult = bind(listenSocket, (sockaddr*)&serverAddress, sizeof(serverAddress));
	if (iResult == SOCKET_ERROR) {
		cout << "bind failed: " << WSAGetLastError() << std::endl;
		closesocket(listenSocket);
		WSACleanup();
		return 1;
	}

	// Listen for incoming connections
	iResult = listen(listenSocket, SOMAXCONN);
	if (iResult == SOCKET_ERROR) {
		cout << "listen failed: " << WSAGetLastError() << std::endl;
		closesocket(listenSocket);
		WSACleanup();
		return 1;
	}
	
	// Accept incoming connections and create a new thread for each client
	while (true) {
		WaitForSingleObject(semafor, INFINITE);
		SOCKET clientSocket = accept(listenSocket, NULL, NULL);
		if (clientSocket == INVALID_SOCKET) {
			cout << "accept failed: " << WSAGetLastError() << std::endl;
			closesocket(listenSocket);
			WSACleanup();
			return 1;
		}

		// Create a new thread for the client
		thread t(HandleClient, clientSocket);
		t.detach();
	}

	// Cleanup Winsock
	closesocket(listenSocket);
	WSACleanup();

	return 0;
}