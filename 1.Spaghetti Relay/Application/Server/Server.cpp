#include "Server.h"

Server::Server()
{

}

Server::~Server()
{

}

int Server::init(uint16_t port)
{
	status = -5;
	serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (serverSocket == INVALID_SOCKET)
	{
		return SETUP_ERROR;
	}
	sa.sin_family = AF_INET;
	sa.sin_port = htons(port);			/////// BIG ENDIAN TO LITTLE ENDIAN!!!!!!
	sa.sin_addr.s_addr = INADDR_ANY;	/////// DONT LET CHANGING THE CAPITALIZATION 

	if (bind(serverSocket, (SOCKADDR *)&sa, sizeof(sa)) == SOCKET_ERROR)
	{
		return BIND_ERROR;
	}

	if (listen(serverSocket, 2) == SOCKET_ERROR)
	{
		return SETUP_ERROR;
	}

	fromSize = sizeof(from);
	incomingSocket = accept(serverSocket, &from, &fromSize);

	if (incomingSocket == SOCKET_ERROR)
	{
		if (status != SHUTDOWN)
			return CONNECT_ERROR;
		else
			return SHUTDOWN;
	}

	return SUCCESS;
}

int Server::readMessage(char* buffer, int32_t size)
{
	int bytesReceived = 0;
	int SizeOrError = 0;
	int messageLength = 0;
	recv(incomingSocket, (char*)&messageLength, sizeof(char), 0);

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
		SizeOrError = recv(incomingSocket, buffer + bytesReceived, messageLength - bytesReceived, 0);
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

int Server::sendMessage(char* data, int32_t length)
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

	if (send(incomingSocket, aux, length+1, 0) < 1)
	{
		if (status != SHUTDOWN)
			return DISCONNECT;
		else
			return SHUTDOWN;
	}
	delete[] aux;
	return SUCCESS;
}

void Server::stop()
{
	status = SHUTDOWN;

	shutdown(serverSocket, SD_BOTH);
	closesocket(serverSocket);

	shutdown(incomingSocket, SD_BOTH);
	closesocket(incomingSocket);
}
