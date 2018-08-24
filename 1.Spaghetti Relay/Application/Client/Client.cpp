#include "Client.h"

Client::Client()
{

}

Client::~Client()
{

}

int Client::init(uint16_t port, char* address)
{
	status = -5;
	clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (clientSocket == INVALID_SOCKET)
	{
		return SETUP_ERROR;
	}

	sa.sin_family = AF_INET;
	sa.sin_port = htons(port);			/////// BIG ENDIAN TO LITTLE ENDIAN!!!!!!
	sa.sin_addr.s_addr = inet_addr(address); // don't use a L instead of a 1

	if (sa.sin_addr.s_addr == INADDR_NONE)
	{
		return ADDRESS_ERROR;
	}

	if (connect(clientSocket, (SOCKADDR*)&sa, sizeof(sa)) == SOCKET_ERROR)
	{
		if (status != SHUTDOWN)
			return DISCONNECT;
		else
			return SHUTDOWN;
	}

	return SUCCESS;
}

int Client::readMessage(char* buffer, int32_t size)
{
	int bytesReceived = 0;
	int SizeOrError = 0;
	int messageLength = 0;
	recv(clientSocket, (char*)&messageLength, sizeof(char), 0);

	if (messageLength > size)
	{
		//parameter too long
		return PARAMETER_ERROR;
	}

	if (messageLength < 1)
	{
		if (status != SHUTDOWN)
			return DISCONNECT;
		else
			return SHUTDOWN;
	}

	for (int32_t i = 0; i < size; i++)
		buffer[i] = '\0';

	while (bytesReceived < messageLength)
	{
		SizeOrError = recv(clientSocket, buffer + bytesReceived, messageLength - bytesReceived, 0);
		if (SizeOrError < 1)
		{
			if (status != SHUTDOWN)
				return DISCONNECT;
			else
				return SHUTDOWN;
		}

		bytesReceived += SizeOrError;
	}
	return SUCCESS;
}

int Client::sendMessage(char* data, int32_t length)
{
	if (status == SHUTDOWN)
		return DISCONNECT;
	if (length < 0 || length > 255)
	{
		return PARAMETER_ERROR;
	}

	char* aux = new char[length+1];
	aux[0] = length;

	for (int32_t i = 1; i < length; i++)
		aux[i] = data[i - 1];
	if (length > 0)
		aux[length] = '\0';

	if (send(clientSocket, aux, length+1, 0) < 1)
	{
		if (status != SHUTDOWN)
			return DISCONNECT;
		else
			return SHUTDOWN;
	}
	delete[] aux;
	return SUCCESS;
}

void Client::stop()
{
	status = SHUTDOWN;

	closesocket(clientSocket);
	shutdown(clientSocket, SD_BOTH);
}
