#include "TCPChatClient.h"


TCPChatClient::TCPChatClient(ChatLobby& chat_int) : chat_interface(chat_int)
{

}
TCPChatClient::~TCPChatClient(void)
{

}
bool TCPChatClient::init(std::string name, std::string ip_address, uint16_t port)
{
	status = -1;
	clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (clientSocket == INVALID_SOCKET)
	{
		chat_interface.DisplayString("CLIENT: SETUP ERROR");
		return false;
	}

	sa.sin_family = AF_INET;
	sa.sin_port = htons(port);
	sa.sin_addr.s_addr = inet_addr(ip_address.c_str());

	if (sa.sin_addr.s_addr == INADDR_NONE)
	{
		chat_interface.DisplayString("CLIENT: ADDRESS ERROR");
		return false;
	}
	if (connect(clientSocket, (SOCKADDR*)&sa, sizeof(sa)) == SOCKET_ERROR)
	{
		if (status == 1)
			chat_interface.DisplayString("CLIENT: SERVER DISCONNECTED");
		else
			chat_interface.DisplayString("CLIENT: SHUTDOWN");
		return false;
	}
	char boffur[20];
	for (int i = 0; i < 20; i++)
		boffur[i] = '\0';

	uint16_t size = 18;
	memcpy(boffur, (char*)&size, sizeof(uint16_t));
	uint8_t size2 = cl_reg;
	memcpy(&boffur[2], (char*)&size2, 1);
	memcpy(&boffur[3], name.c_str(), strlen(name.c_str()));

	if (send(clientSocket, (char*)&boffur, 20, 0) < 1)
	{
		chat_interface.DisplayString("CLIENT: INIT SEND FAILED");
		return false;
	}

	myName = name;

	chat_interface.DisplayString("CLIENT: CONNECTED TO SERVER");

	return true;
}
bool TCPChatClient::run(void)
{
	int bytesReceived = 0;
	int SizeOrError = 0;
	uint16_t messageLength = 0;

	SizeOrError = recv(clientSocket, (char*)&messageLength, sizeof(uint16_t), 0);

	if (SizeOrError < 1)
	{
		if (status == 1)
			chat_interface.DisplayString("CLIENT: SERVER DISCONNECTED");
		else
			chat_interface.DisplayString("CLIENT: WRONG SIZE RECEIVED");
		return false;
	}

	if (status == 1)
	{
		chat_interface.DisplayString("CLIENT: DISCONNECTED");
		return false;
	}

	if (messageLength < 1)
	{
		//stop();
		if (status == 1)
			chat_interface.DisplayString("CLIENT: SERVER DISCONNECTED");
		else
			chat_interface.DisplayString("CLIENT: SHUTDOWN");
		return false;
	}

	char* buffer = new char[messageLength];
	

	SizeOrError = tcp_recv_whole(clientSocket, buffer, messageLength, 0);

	if (SizeOrError < 1)
	{
		chat_interface.DisplayString("CLIENT: WRONG SIZE RECEIVED 2");
		return false;
	}

	if (status == 1)
	{
		chat_interface.DisplayString("CLIENT: DISCONNECTED");
		delete[] buffer;
		return false;
	}

	switch (buffer[0])
	{
	case sv_cnt:
	{
		memcpy((char*)&myId, &buffer[1], 1);
		if (myId < 0)
		{
			chat_interface.DisplayString("CLIENT: WRONG ID");
			delete[] buffer;
			return false;
		}

		char boffur[3];

		uint16_t size = 1;
		memcpy(boffur, (char*)&size, 2);
		uint8_t size2 = cl_get;
		memcpy(&boffur[2], (char*)&size2, 1);

		if (send(clientSocket, (char*)&boffur, 3, 0) < 1)
		{
			chat_interface.DisplayString("CLIENT: GET SEND FAILED");
			return false;
		}

		chat_interface.DisplayString("CLIENT: REGISTERED IN SERVER");

		break;
	}
	case sv_full:
	{
		chat_interface.DisplayString("CLIENT: SERVER IS FULL");
		delete[] buffer;
		return false;
	}
	case sv_list:
	{
		uint8_t sizelist = buffer[1];

		for (uint8_t i = 0; i < sizelist; i++)
		{
			uint8_t helId = buffer[2 + (18 * i)]; //id
				
			std::string nameHelp;
			nameHelp.resize(MAX_NAME_LENGTH);
			memcpy((char*)nameHelp.c_str(), &buffer[3 + (18 * i)], MAX_NAME_LENGTH);
			myNameList[helId] = nameHelp;
			chat_interface.AddNameToUserList(nameHelp.c_str(), (unsigned int)helId);


			nameHelp.clear();
		}

		chat_interface.DisplayString("CLIENT: RECEIVED USER LIST");

		break;
	}
	case sv_add:
	{
		uint8_t ID;
		memcpy((char*)&ID, &buffer[1], 1);
		std::string liname;
		liname.clear();
		liname.resize(MAX_NAME_LENGTH);
		memcpy((char*)liname.c_str(), &buffer[2], MAX_NAME_LENGTH);

		chat_interface.AddNameToUserList(&liname.c_str()[0], (unsigned int)ID);
		
		char* addition = new char[strlen(liname.c_str()) + 16];

		myNameList[ID] = liname;

		sprintf(addition, "CLIENT: %s JOINED", liname.c_str());
		chat_interface.DisplayString(&addition[0]);
		liname.clear();
		delete[] addition;
		break;
	}
	case sv_remove:
	{
		uint8_t ID;
		memcpy((char*)&ID, &buffer[1], 1);
		chat_interface.RemoveNameFromUserList((unsigned int)ID);

		char* addition = new char[strlen(myNameList[ID].c_str()) + 14];

		sprintf(addition, "CLIENT: %s LEFT", myNameList[ID].c_str());

		myNameList.erase(myNameList.find(ID));
		chat_interface.DisplayString(&addition[0]);
		delete[] addition;

		break;
	}
	case sv_cl_msg:
	{
		uint8_t ID;
		memcpy((char*)&ID, &buffer[1], 1);
		std::string message;
		message.resize(messageLength - 2);
		message.clear();
		memcpy((char*)message.c_str(), &buffer[2], messageLength - 2);

		chat_interface.AddChatMessage(&message.c_str()[0], (unsigned int)ID);
		message.clear();
		break;
	}
	case sv_cl_close:
	{
		
		chat_interface.DisplayString("CLIENT: SERVER DISCONNECTED");
		status = 2;
		for (unsigned int i = 0; i < 4; i++)
			chat_interface.RemoveNameFromUserList(i);

		return false;
	}
	}

	delete[] buffer;

	return true;
}
bool TCPChatClient::send_message(std::string message)
{
	uint16_t messSize = (uint16_t)strlen(message.c_str()) + 3; //1 for null terminator, 1 for type, 1 for iD
	char* buffer = new char[messSize + 2];

	memcpy(&buffer[0], (char*)&messSize, sizeof(uint16_t));
	uint8_t type = sv_cl_msg;
	memcpy(&buffer[2], (char*)&type, 1);
	memcpy(&buffer[3], (char*)&myId, 1);
	memcpy(&buffer[4], (char*)&message.c_str()[0], (size_t)strlen(message.c_str()));
	buffer[messSize + 1] = '\0';

	if (send(clientSocket, buffer, messSize+2, 0) < 1)
	{
		if (status == 2)
			chat_interface.DisplayString("CLIENT: CAN'T SEND MESSAGES WHEN SERVER IS DOWN");
		else
			chat_interface.DisplayString("CLIENT: MESSAGE SEND FAILED");
		delete[] buffer;
		return false;
	}

	delete[] buffer;
	return true;
}
bool TCPChatClient::stop(void)
{
	
	if (clientSocket != INVALID_SOCKET)
	{
		if (status != 2)
		{
			char boffur[5];

			uint16_t size = 2;
			memcpy(boffur, (char*)&size, sizeof(uint16_t));
			uint8_t size2 = sv_cl_close;
			memcpy(&boffur[2], (char*)&size2, 1);
			memcpy(&boffur[3], (char*)&myId, 1);
			boffur[4] = '\0';

			status = 1;
			if (send(clientSocket, (char*)&boffur, 5, 0) < 1)
			{
				chat_interface.DisplayString("CLIENT: STOP FAILED");
				return false;
			}
		}

		myName.clear();
		myNameList.clear();
		shutdown(clientSocket, SD_BOTH);
		closesocket(clientSocket);
		
	}

	return true;
}
