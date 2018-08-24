#include "../platform.h"
#include "../definitions.h"

#include <iostream>
using namespace std;

class Client
{
	int status;
	SOCKET clientSocket;
	SOCKADDR_IN sa;

public:
	Client();
	~Client();
	int init(uint16_t port, char* address);
	int readMessage(char* buffer, int32_t size);
	int sendMessage(char* data, int32_t length);
	void stop();
};