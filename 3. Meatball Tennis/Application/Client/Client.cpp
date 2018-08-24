// Client.cpp : This file handles all client network functions.
#include "Client.h"
#include "../NetworkMessage.h"

static int counter = 0;

// Initializes the client; connects to the server.
int Client::init(char* address, uint16_t port, uint8_t _player)
{
	sequencing = 0;
	state.player0.keyUp = state.player0.keyDown = false;
	state.player1.keyUp = state.player1.keyDown = false;
	state.gamePhase = WAITING;
	player = (int)_player;
	clSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (clSocket == INVALID_SOCKET)
	{
		return SETUP_ERROR;
	}

	SOCKADDR_IN sa;
	sa.sin_family = AF_INET;
	sa.sin_port = htons(port);
	sa.sin_addr.s_addr = inet_addr(address);

	if (sa.sin_addr.s_addr == INADDR_NONE)
	{
		return ADDRESS_ERROR;
	}
	if (connect(clSocket, (SOCKADDR*)&sa, sizeof(sa)) == SOCKET_ERROR)
	{
		return DISCONNECT;
	}

	NetworkMessage myMessaOu = NetworkMessage(IO::_OUTPUT);
	NetworkMessage myMessaIn = NetworkMessage(IO::_INPUT);
	uint8_t type = CL_CONNECT;
	myMessaOu.writeByte(type);
	myMessaOu.writeByte(_player);

	sendNetMessage(clSocket, myMessaOu);

	recvNetMessage(clSocket, myMessaIn);
	uint16_t revUI = myMessaIn.readShort();

	if (revUI > sequencing)
	{
		sequencing = revUI;
		uint8_t revType = myMessaIn.readByte();

		if (revType == SV_FULL)
		{
			active = false;
			shutdown(clSocket, SD_BOTH);
			closesocket(clSocket);
			return SHUTDOWN;
		}
	}

	active = true;

	return SUCCESS;
}

// Receive and process messages from the server.
int Client::run()
{

	NetworkMessage myMessaIn = NetworkMessage(IO::_INPUT);
	while (active == true && state.gamePhase != DISCONNECTED)
	{
		myMessaIn.reset(IO::_INPUT);
		if (recvNetMessage(clSocket, myMessaIn) < 1)
		{
			if (active == true)
				return DISCONNECT;
			else
				return SHUTDOWN;
		}
		if (myMessaIn.getLength() > 0)
		{
			uint16_t revUI = myMessaIn.readShort();

			if (revUI > sequencing)
			{
				sequencing = revUI;
				uint8_t revType = myMessaIn.readByte();

				if (revType == SV_CL_CLOSE)
					stop();
				else if (revType == SV_SNAPSHOT)
				{
					counter++;
					GameState recState;

					state.gamePhase = myMessaIn.readByte();
					state.ballX = myMessaIn.readShort();
					state.ballY = myMessaIn.readShort();
					state.player0.y = myMessaIn.readShort();
					state.player0.score = myMessaIn.readShort();
					state.player1.y = myMessaIn.readShort();
					state.player1.score = myMessaIn.readShort();

					if (counter % 10 == 0)
						sendAlive();
				}
				else
					return MESSAGE_ERROR;
			}
		}
	}

	return DISCONNECT;
}

// Clean up and shut down the client.
void Client::stop()
{
	sendClose();
	active = false;
	shutdown(clSocket, SD_BOTH);
	closesocket(clSocket);
	state.gamePhase = DISCONNECTED;
}

// Send the player's input to the server.
int Client::sendInput(int8_t keyUp, int8_t keyDown, int8_t keyQuit)
{
	if (keyQuit)
	{
		stop();
		return SHUTDOWN;
	}

	cs.enter();
	if (player == 0)
	{
		state.player0.keyUp = keyUp;
		state.player0.keyDown = keyDown;
	}
	else
	{
		state.player1.keyUp = keyUp;
		state.player1.keyDown = keyDown;
	}
	cs.leave();

	NetworkMessage myMessaOu = NetworkMessage(IO::_OUTPUT);
	uint8_t type = CL_KEYS;
	myMessaOu.writeByte(type);
	if (player == 0)
	{
		myMessaOu.writeByte(state.player0.keyUp);
		myMessaOu.writeByte(state.player0.keyDown);
	}
	else
	{
		myMessaOu.writeByte(state.player1.keyUp);
		myMessaOu.writeByte(state.player1.keyDown);
	}
	sendNetMessage(clSocket, myMessaOu);

	return SUCCESS;
}

// Copies the current state into the struct pointed to by target.
void Client::getState(GameState* target)
{
	memcpy(target, &state, sizeof(state));
}

// Sends a SV_CL_CLOSE message to the server (private, suggested)
void Client::sendClose()
{
	NetworkMessage myMessaOu = NetworkMessage(IO::_OUTPUT);
	uint8_t type = SV_CL_CLOSE;
	myMessaOu.writeByte(type);

	sendNetMessage(clSocket, myMessaOu);
}

// Sends a CL_ALIVE message to the server (private, suggested)
int Client::sendAlive()
{
	NetworkMessage myMessaOu = NetworkMessage(IO::_OUTPUT);
	uint8_t type = CL_ALIVE;
	myMessaOu.writeByte(type);

	sendNetMessage(clSocket, myMessaOu);

	return SUCCESS;
}
