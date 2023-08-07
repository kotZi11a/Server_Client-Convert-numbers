#include <iostream>
#include <winsock2.h>
#include <condition_variable>
#include <thread>
#include <string>
#include <sstream>

#pragma warning(disable: 4996)
#pragma comment(lib, "ws2_32.lib")
char buffer_s[1024];

using namespace std;

HANDLE event = OpenEventW(
	SEMAPHORE_ALL_ACCESS,
	TRUE,
	L"Global\\Event"
);

HANDLE semafor = OpenSemaphoreW(
	SEMAPHORE_ALL_ACCESS,
	TRUE,
	L"Global\\Sem"
);

char check_num(string number)
{
	if (number == "--close")
	{
		return 'c';
	}
	if (number == "--wait")
	{
		return 'w';
	}
	for (int i = 0; i < number.length(); i++)
		if (number[i] < '0' || number[i] > '9')
			return 'e';
	return 'o';
}

char check_sys(string number)
{
	if (number == "--close")
	{
		return 'c';
	}
	if (number == "--wait")
	{
		return 'w';
	}
	if(number == "2" || number == "16")
		return 'o';
	return 'e';
}


bool number_converting(SOCKET socket)
{
	WaitForSingleObject(semafor, INFINITY);
	while (true) {
		char buffer[1024];

		string number;
		string system;
		char status;
		bool correct = true;
		while (correct) {
			std::cout << "Write number to convert: ";
			cin >> number;
			status = check_num(number);
			switch (status)
			{
			case 'c':
				return 0;
				break;
			case 'e':
				std::cout << "Not correct number. Use integer number or --close for close program\n";
				break;
			case 'o':
				correct = false;
				break;
			case 'w':
				int counter_buf = send(socket, number.c_str(), number.size(), 0);
				ResetEvent(event);
				cout << "Wait....\n";
				WaitForSingleObject(event, INFINITE);
				SetEvent(event);
				if (counter_buf == SOCKET_ERROR)
				{
					std::cout << "send failed: " << WSAGetLastError() << endl;
					return 0;
				}
				break;
			}
		}
		int counter_buf = send(socket, number.c_str(), number.size(), 0);
		if (counter_buf == SOCKET_ERROR)
		{
			std::cout << "send failed: " << WSAGetLastError() << endl;
			return 0;
		}

		correct = true;

		while (correct) {
			std::cout << "Write system for convert(2 / 16): ";
			cin >> system;
			status = check_sys(system);
			switch (status)
			{
			case 'c':
				return 0;
				break;
			case 'e':
				std::cout << "Not correct number. Use integer number\n";
				break;
			case 'o':
				correct = false;
				break;
			case 'w':
				correct = false;
				break;
			default:
				std::cout << "status error\n";
				break;
			}
		}
		counter_buf = send(socket, system.c_str(), system.size(), 0);
		if (counter_buf == SOCKET_ERROR)
		{
			std::cout << "send failed: " << WSAGetLastError() << endl;
			return 0;
		}


		counter_buf = recv(socket, buffer, sizeof(buffer), 0);
		if (counter_buf == SOCKET_ERROR) {

			std::cout << "recieve failed: " << WSAGetLastError() << endl;
			return 0;
		}
		buffer[counter_buf] = '\0';
		string res(buffer);
		std::cout << "Result: " << res << endl;

	}
	return 1;
}


int main()
{
	// Initialize Winsock
	WSADATA wsaData;
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		std::cout << "WSAStartup failed: " << iResult << endl;
		return 1;
	}

	// Create a socket for connecting to the server
	SOCKET lsocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (lsocket == INVALID_SOCKET) {
		std::cout << "socket failed: " << WSAGetLastError() << endl;
		WSACleanup();
		return 1;
	}

	// Connect to the server
	sockaddr_in serverAddress;
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1");
	serverAddress.sin_port = htons(8080);

	
	//std::cout << "Wait connect...\n";
	//WaitForSingleObject(semafor, INFINITY);

	WaitForSingleObject(semafor, INFINITE);
	iResult = connect(lsocket, (sockaddr*)&serverAddress, sizeof(serverAddress));
	if (iResult == SOCKET_ERROR) {
		std::cout << "connect failed: " << WSAGetLastError() << endl;
		closesocket(lsocket);
		WSACleanup();
		return 1;
	}
	bool close = true;
	
	recv(lsocket, buffer_s, sizeof(buffer_s), 0);
	std::cout << "~You are User[" << buffer_s << "]! " << endl;


	number_converting(lsocket);

	

	// Cleanup Winsock
	closesocket(lsocket);
	WSACleanup();

	return 0;
}