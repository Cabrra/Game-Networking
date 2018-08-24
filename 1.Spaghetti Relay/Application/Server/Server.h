#include <WinSock2.h>
#pragma comment(lib, "ws2_32.lib")
#include <Windows.h>

#include "ServerWrapper.h"

#include <iostream>
using namespace std;

class Server
{
	int status;
	SOCKET serverSocket;
	SOCKADDR_IN sa;
	SOCKADDR from;
	int fromSize;
	SOCKET incomingSocket;

public:
	Server();
	~Server();
	int init(uint16_t port);
	int readMessage(char* buffer, int32_t size);
	int sendMessage(char* data, int32_t length);
	void stop();
};