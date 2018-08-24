// Server.cpp : Contains all functions of the server.
#include "Server.h"

// Initializes the server. (NOTE: Does not wait for player connections!)
int Server::init(uint16_t port)
{
	initState();
	sequence = 0;

	svSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	unsigned long value = 1;// set to 0 to make it blocking again

	if (ioctlsocket(svSocket, FIONBIO, &value) == SOCKET_ERROR)
		return SETUP_ERROR;
	if (svSocket == INVALID_SOCKET)
		return SETUP_ERROR;

	SOCKADDR_IN sa;

	sa.sin_family = AF_INET;
	sa.sin_port = htons(port);			/////// BIG ENDIAN TO LITTLE ENDIAN!!!!!!
	sa.sin_addr.s_addr = INADDR_ANY;	/////// DONT LET CHANGING THE CAPITALIZATION 

	if (bind(svSocket, (SOCKADDR *)&sa, sizeof(sa)) == SOCKET_ERROR)
		return BIND_ERROR;

	active = true; 
	return SUCCESS;
}

// Updates the server; called each game "tick".
int Server::update()
{
	NetworkMessage myMessaIn = NetworkMessage(IO::_INPUT);
	sockaddr_in exeded;

	myMessaIn.reset(IO::_INPUT);

	int myRet = recvfromNetMessage(svSocket, myMessaIn, &exeded);
	int myError = WSAGetLastError();

	if (myRet > 0 )
	{
		//parse message
		parseMessage(exeded, myMessaIn);
	}
	else if (myRet == -1 && myError == EWOULDBLOCK)
	{
		if (playerTimer[0] > 50)
			disconnectClient(0);
		if (playerTimer[1] > 50)
			disconnectClient(1);

		updateState();
		sendState();
		sequence++;
	}
	else
	{
		if (active == true)
			return DISCONNECT;
		else
			return SHUTDOWN;
	}


	return SUCCESS;
}

// Stops the server.
void Server::stop()
{
	sendClose();
	active = false; 

	shutdown(svSocket, SD_BOTH);
	closesocket(svSocket);
}

// Parses a message and responds if necessary. (private, suggested)
int Server::parseMessage(sockaddr_in& source, NetworkMessage& message)
{
	uint8_t revType = message.readByte();

	switch (revType)
	{
	case CL_CONNECT:
	{
		if (noOfPlayers == 2)
			sendFull(source);
		else
		{
			uint8_t revPlay = message.readByte();
			connectClient(revPlay, source);
			
			sendOkay(source);
		}
		break;
	}
	case CL_ALIVE:
	{
		if (source.sin_addr.s_addr == playerAddress[0].sin_addr.s_addr && source.sin_port == playerAddress[0].sin_port)
			playerTimer[0] = 0;
		if (source.sin_addr.s_addr == playerAddress[1].sin_addr.s_addr && source.sin_port == playerAddress[1].sin_port)
			playerTimer[1] = 0;
		break;
	}
	case SV_CL_CLOSE:
	{
		if (source.sin_addr.s_addr == playerAddress[0].sin_addr.s_addr && source.sin_port == playerAddress[0].sin_port)
			disconnectClient(0);
		if (source.sin_addr.s_addr == playerAddress[1].sin_addr.s_addr && source.sin_port == playerAddress[1].sin_port)
			disconnectClient(1);
		break;
	}
	case CL_KEYS:
	{
		uint8_t up = message.readByte();
		uint8_t down = message.readByte();

		if (source.sin_addr.s_addr == playerAddress[0].sin_addr.s_addr && source.sin_port == playerAddress[0].sin_port)
		{
			state.player0.keyUp = up;
			state.player0.keyDown = down;

		}
		if (source.sin_addr.s_addr == playerAddress[1].sin_addr.s_addr && source.sin_port == playerAddress[1].sin_port)
		{
			state.player1.keyUp = up;
			state.player1.keyDown = down; 
		}
		break;
	}
	}

	return SUCCESS;
}

// Sends the "SV_OKAY" message to destination. (private, suggested)
int Server::sendOkay(sockaddr_in& destination)
{
	NetworkMessage myMessaOu = NetworkMessage(IO::_OUTPUT);
	myMessaOu.writeShort(sequence);
	uint8_t type = SV_OKAY;
	myMessaOu.writeByte(type);

	sendMessage(destination, myMessaOu);
	sequence++;
	return SUCCESS;
}

// Sends the "SV_FULL" message to destination. (private, suggested)
int Server::sendFull(sockaddr_in& destination)
{
	NetworkMessage myMessaOu = NetworkMessage(IO::_OUTPUT);
	myMessaOu.writeShort(sequence);
	uint8_t type = SV_FULL;
	myMessaOu.writeByte(type);

	sendMessage(destination, myMessaOu);
	sequence++;
	return SUCCESS;
}

// Sends the current snapshot to all players. (private, suggested)
int Server::sendState()
{
	NetworkMessage myMessaOu = NetworkMessage(IO::_OUTPUT);
	myMessaOu.writeShort(sequence);
	uint8_t type = SV_SNAPSHOT;
	myMessaOu.writeByte(type);
	myMessaOu.writeByte(state.gamePhase);
	myMessaOu.writeShort(state.ballX);
	myMessaOu.writeShort(state.ballY);
	myMessaOu.writeShort(state.player0.y);
	myMessaOu.writeShort(state.player0.score);
	myMessaOu.writeShort(state.player1.y);
	myMessaOu.writeShort(state.player1.score);

	sendMessage(playerAddress[0], myMessaOu);
	sendMessage(playerAddress[1], myMessaOu);
	sequence++;

	return SUCCESS;
}

// Sends the "SV_CL_CLOSE" message to all clients. (private, suggested)
void Server::sendClose()
{
	NetworkMessage myMessaOu = NetworkMessage(IO::_OUTPUT);
	myMessaOu.writeShort(sequence);
	uint8_t type = SV_CL_CLOSE;
	myMessaOu.writeByte(type);
		
	sendMessage(playerAddress[0], myMessaOu);
	sendMessage(playerAddress[1], myMessaOu);
	sequence++;
}

// Server message-sending helper method. (private, suggested)
int Server::sendMessage(sockaddr_in& destination, NetworkMessage& message)
{
	sendtoNetMessage(svSocket, message, &destination);
	
	return SUCCESS;
}

// Marks a client as connected and adjusts the game state.
void Server::connectClient(int player, sockaddr_in& source)
{
	playerAddress[player] = source;
	playerTimer[player] = 0;

	noOfPlayers++;
	if (noOfPlayers == 1)
		state.gamePhase = WAITING;
	else
		state.gamePhase = RUNNING;
}

// Marks a client as disconnected and adjusts the game state.
void Server::disconnectClient(int player)
{
	playerAddress[player].sin_addr.s_addr = INADDR_NONE;
	playerAddress[player].sin_port = 0;

	noOfPlayers--;
	if (noOfPlayers == 1)
		state.gamePhase = WAITING;
	else
		state.gamePhase = DISCONNECTED;
}

// Updates the state of the game.
void Server::updateState()
{
	// Tick counter.
	static int timer = 0;

	// Update the tick counter.
	timer++;

	// Next, update the game state.
	if (state.gamePhase == RUNNING)
	{
		// Update the player tick counters (for ALIVE messages.)
		playerTimer[0]++;
		playerTimer[1]++;

		// Update the positions of the player paddles
		if (state.player0.keyUp)
			state.player0.y -= 5;
		if (state.player0.keyDown)
			state.player0.y += 5;

		if (state.player1.keyUp)
			state.player1.y -= 5;
		if (state.player1.keyDown)
			state.player1.y += 5;
		
		// Make sure the paddle new positions are within the bounds.
		if (state.player0.y < 0)
			state.player0.y = 0;
		else if (state.player0.y > FIELDY - PADDLEY)
			state.player0.y = FIELDY - PADDLEY;

		if (state.player1.y < 0)
			state.player1.y = 0;
		else if (state.player1.y > FIELDY - PADDLEY)
			state.player1.y = FIELDY - PADDLEY;

		//just in case it get stuck...
		if (ballVecX)
			storedBallVecX = ballVecX;
		else
			ballVecX = storedBallVecX;

		if (ballVecY)
			storedBallVecY = ballVecY;
		else
			ballVecY = storedBallVecY;

		state.ballX += ballVecX;
		state.ballY += ballVecY;

		// Check for paddle collisions & scoring
		if (state.ballX < PADDLEX)
		{
			// If the ball has struck the paddle...
			if (state.ballY + BALLY > state.player0.y && state.ballY < state.player0.y + PADDLEY)
			{
				state.ballX = PADDLEX;
				ballVecX *= -1;
			}
			// Otherwise, the second player has scored.
			else
			{
				newBall();
				state.player1.score++;
				ballVecX *= -1;
			}
		}
		else if (state.ballX >= FIELDX - PADDLEX - BALLX)
		{
			// If the ball has struck the paddle...
			if (state.ballY + BALLY > state.player1.y && state.ballY < state.player1.y + PADDLEY)
			{
				state.ballX = FIELDX - PADDLEX - BALLX - 1;
				ballVecX *= -1;
			}
			// Otherwise, the first player has scored.
			else
			{
				newBall();
				state.player0.score++;
				ballVecX *= -1;
			}
		}

		// Check for Y position "bounce"
		if (state.ballY < 0)
		{
			state.ballY = 0;
			ballVecY *= -1;
		}
		else if (state.ballY >= FIELDY - BALLY)
		{
			state.ballY = FIELDY - BALLY - 1;
			ballVecY *= -1;
		}
	}

	// If the game is over...
	if ((state.player0.score > 10 || state.player1.score > 10) && state.gamePhase == RUNNING)
	{
		state.gamePhase = GAMEOVER;
		timer = 0;
	}
	if (state.gamePhase == GAMEOVER)
	{
		if (timer > 30)
		{
			initState();
			state.gamePhase = RUNNING;
		}
	}
}

// Initializes the state of the game.
void Server::initState()
{
	playerAddress[0].sin_addr.s_addr = INADDR_NONE;
	playerAddress[1].sin_addr.s_addr = INADDR_NONE;
	playerTimer[0] = playerTimer[1] = 0;
	noOfPlayers = 0;

	state.gamePhase = DISCONNECTED;

	state.player0.y = 0;
	state.player1.y = FIELDY - PADDLEY - 1;
	state.player0.score = state.player1.score = 0;
	state.player0.keyUp = state.player0.keyDown = false;
	state.player1.keyUp = state.player1.keyDown = false;

	newBall(); // Get new random ball position

	// Get a new random ball vector that is reasonable
	ballVecX = (rand() % 10) - 5;
	if ((ballVecX >= 0) && (ballVecX < 2))
		ballVecX = 2;
	if ((ballVecX < 0) && (ballVecX > -2))
		ballVecX = -2;

	ballVecY = (rand() % 10) - 5;
	if ((ballVecY >= 0) && (ballVecY < 2))
		ballVecY = 2;
	if ((ballVecY < 0) && (ballVecY > -2))
		ballVecY = -2;


}

// Places the ball randomly within the middle half of the field.
void Server::newBall()
{
	// (randomly in 1/2 playable area) + (1/4 playable area) + (left buffer) + (half ball)
	state.ballX = (rand() % ((FIELDX - 2 * PADDLEX - BALLX) / 2)) +
						((FIELDX - 2 * PADDLEX - BALLX) / 4) + PADDLEX + (BALLX / 2);

	// (randomly in 1/2 playable area) + (1/4 playable area) + (half ball)
	state.ballY = (rand() % ((FIELDY - BALLY) / 2)) + ((FIELDY - BALLY) / 4) + (BALLY / 2);
}
