#include "TCPChatServer.h"

TCPChatServer::TCPChatServer(ChatLobby& chat_int) : chat_interface(chat_int)
{

}
TCPChatServer::~TCPChatServer(void)
{

}
bool TCPChatServer::init(uint16_t port)
{
	status = -1;

	listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (listenSocket == INVALID_SOCKET)
	{
		chat_interface.DisplayString("SERVER: LISTEN SOCKET CREATION FAILED");
		return false;
	}

	sa.sin_family = AF_INET;
	sa.sin_port = htons(port);			/////// BIG ENDIAN TO LITTLE ENDIAN!!!!!!
	sa.sin_addr.s_addr = INADDR_ANY;	/////// DONT LET CHANGING THE CAPITALIZATION 

	if (bind(listenSocket, (SOCKADDR *)&sa, sizeof(sa)) == SOCKET_ERROR)
	{
		chat_interface.DisplayString("SERVER: BIND FAILED");
		return false;
	}

	FD_ZERO(&clientsSet);

	if (listen(listenSocket, 5) == SOCKET_ERROR)
	{
		chat_interface.DisplayString("SERVER: LISTEN FAILED");
		return false;
	}

	FD_SET(listenSocket, &clientsSet);

	for (int i = 0; i < 5; i++)
	{
		indexes[i] = -1;
	}

	chat_interface.DisplayString("SERVER: NOW HOSTING SERVER");

	myTimes.tv_sec = 1;
	myTimes.tv_usec = 500;
	return true;
}
bool TCPChatServer::run(void)
{
	fd_set readSet = clientsSet;
	int numReady = select(0, &readSet, NULL, NULL, &myTimes);
	if (FD_ISSET(listenSocket, &readSet))
	{
		SOCKADDR from;
		int fromSize;
		fromSize = sizeof(from);
		int index = NUMCLIENTS;

		for (int i = 0; i < NUMCLIENTS; i++)
		{
			if (indexes[i] == -1)
			{
				index = i;
				break;
			}
		}

		clientSocket[index] = accept(listenSocket, &from, &fromSize);
		if (clientSocket[index] == SOCKET_ERROR)
		{
			if (status == 2)
				chat_interface.DisplayString("SERVER: SERVER DISCONNECTED");
			else
				chat_interface.DisplayString("SERVER: CLIENT CONNECT ERROR");

			return false;
		}
		FD_SET(clientSocket[index], &clientsSet);
		indexes[index] = 0; //reserved
	}

	for (int i = 0; i < NUMCLIENTS+1; i++)
	{
		if (FD_ISSET(clientSocket[i], &readSet))
		{
			int bytesReceived = 0;
			int SizeOrError = 0;
			uint16_t messageLength = 0;

			SizeOrError = recv(clientSocket[i], (char*)&messageLength, sizeof(uint16_t), 0);

			if (SizeOrError < 1)
			{
				if (status == 1)
					chat_interface.DisplayString("SERVER: SERVER DISCONNECTED");
				else
					chat_interface.DisplayString("SERVER: WRONG SIZE RECEIVED");
				return false;
			}

			if (status == 1)
			{
				chat_interface.DisplayString("SERVER: DISCONNECTED");
				return false;
			}

			if (messageLength < 1)
			{
				if (status == 1)
					chat_interface.DisplayString("CLIENT: SERVER DISCONNECTED");
				else
					chat_interface.DisplayString("SERVER: SHUTDOWN");
				return false;
			}

			char* buffer = new char[messageLength];


			SizeOrError = tcp_recv_whole(clientSocket[i], buffer, messageLength, 0);

			if (SizeOrError < 1)
			{
				chat_interface.DisplayString("SERVER: WRONG SIZE RECEIVED 2");
				return false;
			}

			if (status == 1)
			{
				chat_interface.DisplayString("SERVER: DISCONNECTED");
				delete[] buffer;
				return false;
			}

			switch (buffer[0])
			{
			case cl_reg:
			{
				int index = NUMCLIENTS;
				std::string name;
				name.resize(17);
				memcpy((char*)name.c_str(), &buffer[1], 17);

				for (int j = 0; j < NUMCLIENTS; j++)
				{
					if (indexes[j] == 0)
					{
						index = j;
						indexes[j] = 1;
						break;
					}
				}

				if (index == NUMCLIENTS)
				{
					char boffur[3];
					for (int j = 0; j < 3; j++)
						boffur[j] = '\0';
					uint16_t size = 1;
					memcpy(boffur, (char*)&size, sizeof(uint16_t));
					uint8_t size2 = sv_full;
					memcpy(&boffur[2], (char*)&size2, 1);

					if (send(clientSocket[index], (char*)&boffur, 3, 0) < 1)
					{
						chat_interface.DisplayString("SERVER: CL_REG SEND FAILED");
						return false;
					}

					FD_CLR(clientSocket[index], &clientsSet);
					
				}
				else
				{
					char boffur[4];
					for (int j = 0; j < 4; j++)
						boffur[j] = '\0';

					uint16_t size = 2;
					memcpy(boffur, (char*)&size, sizeof(uint16_t));
					uint8_t size2 = sv_cnt;
					memcpy(&boffur[2], (char*)&size2, 1);
					uint8_t id = (uint8_t)index;
					memcpy(&boffur[3], (char*)&id, 1);

					if (send(clientSocket[index], (char*)&boffur, 4, 0) < 1)
					{
						chat_interface.DisplayString("SERVER: ACCEPT SEND FAILED");
						return false;
					}

					clientsNames[(uint8_t)index] = name;
					indexes[index] = 1; //Registered

					//send name and ID to all others
					for (int j = 0; j < NUMCLIENTS; j++)
					{
						if (indexes[j] == 1 && j != i) // send only to those who are still connected
						{
							char gimly[21];
							for (int k = 0; k < 21; k++)
								gimly[k] = '\0';
							uint16_t newSize = 19;
							memcpy(gimly, (char*)&newSize, sizeof(uint16_t));
							uint8_t newSize2 = sv_add;
							memcpy(&gimly[2], (char*)&newSize2, 1);
							uint8_t id = (uint8_t)index;
							memcpy(&gimly[3], (char*)&id, 1);
							memcpy(&gimly[4], (char*)&name.c_str()[0], 17);

							if (send(clientSocket[j], (char*)&gimly, 21, 0) < 1)
							{
								chat_interface.DisplayString("SERVER: FAILED SENDING ADD TO CLIENTS");
								return false;
							}
						}
					}
				}

				break;
			}
			case cl_get:
			{
				//send sv_list
				int counter = 0;
				for (int check = 0; check < clientsNames.size(); check++)
				{
					if (indexes[check] == 1)
						counter++;
				}
				uint16_t newSize = (counter * 18) + 2;
				char* Balin = new char[newSize+2];
				
				memcpy(Balin, (char*)&newSize, sizeof(uint16_t));
				uint8_t newSize2 = sv_list;
				memcpy(&Balin[2], (char*)&newSize2, 1);
				uint8_t number = (uint8_t)counter;
				memcpy(&Balin[3], (char*)&number, 1);

				for (int check = 0; check < clientsNames.size(); check++)
				{
					if (indexes[check] == 1)
					{
						memcpy(&Balin[4 + 18 * check], (char*)&check, 1); /// id
						memcpy(&Balin[5 + 18 * check], (char*)clientsNames[check].c_str(), 17); //name
					}
				}

				if (send(clientSocket[i], Balin, newSize + 2, 0) < 1)
				{
					chat_interface.DisplayString("SERVER: FAILED SENDING SV_LIST TO CLIENT");
					return false;
				}

				delete[] Balin;

				break;
			}
			case sv_cl_msg:
			{
				int size = messageLength + 2;
				char* thorin = new char[size];

				for (int x = 0; x < size; x++)
					thorin[x] = '\0';

				memcpy(thorin, (char*)&messageLength, sizeof(uint16_t));
				memcpy(&thorin[2], buffer, messageLength);

				for (int j = 0; j < NUMCLIENTS; j++)
				{
					if (indexes[j] == 1) // send only to those who are still connected
					{
						if (send(clientSocket[j], thorin, messageLength+2, 0) < 1)
						{
							chat_interface.DisplayString("SERVER: FAILED SENDING MSG TO CLIENTS");
							return false;
						}
					}
				}
				delete[] thorin;
				break;
			}
			case sv_cl_close:
			{
				int size = 4;
				char* dori = new char[size];

				for (int x = 0; x < size; x++)
					dori[x] = '\0';

				memcpy(dori, (char*)&messageLength, sizeof(uint16_t));
				memcpy(&dori[2], buffer, 2);
				uint8_t mess = sv_remove;
				memcpy(&dori[2], (char*)&mess, sizeof(uint8_t));

				for (int j = 0; j < NUMCLIENTS; j++)
				{
					if (indexes[j] == 1 && j != i) // send only to those who are still connected
					{
						if (send(clientSocket[j], dori, messageLength + 2, 0) < 1)
						{
							chat_interface.DisplayString("SERVER: FAILED SENDING REMOVE TO CLIENTS");
							return false;
						}
					}
				}

				indexes[i] = -1;
				FD_CLR(clientSocket[i], &clientsSet);
				FD_CLR(clientSocket[i], &readSet);
				delete[] dori;
				break;
			}
			}

			delete[] buffer;
		}
	}


	return true;
}
bool TCPChatServer::stop(void)
{
	status = 1;
	char gloin[3];
	int length = 1;
	memcpy(gloin, (char*)&length, sizeof(uint16_t));

	uint8_t mess = sv_cl_close;
	memcpy(&gloin[2], (char*)&mess, sizeof(uint8_t));

	for (int j = 0; j < NUMCLIENTS; j++)
	{
		if (indexes[j] == 1) // send only to those who are still connected
		{
			if (send(clientSocket[j], (char*)&gloin, 3, 0) < 1)
			{
				chat_interface.DisplayString("SERVER: FAILED SENDING CLOSE_MSG TO CLIENTS");
				return false;
			}
			indexes[j] = -1;
		}
	}

	for (int check = 0; check < NUMCLIENTS+1; check++)
	{
		FD_CLR(clientSocket[check], &clientsSet);
		shutdown(clientSocket[check], SD_BOTH);
		closesocket(clientSocket[check]);
	}
	FD_CLR(listenSocket, &clientsSet);
	shutdown(listenSocket, SD_BOTH);
	closesocket(listenSocket);
	clientsNames.clear();
}