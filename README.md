Game Networking
===============

#### 1. Spaghetti Relay

![Spaghetti](https://github.com/Cabrra/cabrra.github.io/raw/master/Images/networking/spaghetti.png)

The Spaghetti Relay lab is intended to introduce basic networking concepts involved in binding, listening, and accepting server connections and connecting via clients, as well as sending and receiving simple messages via TCP/IP.
The front-end deals with two dynamic linked libraries (DLLs), Client.dll and Server.dll, through the ClientWrapper and ServerWrapper interfaces. These wrappers instantiate and call the Client and Server classes.

#### 2. Real Time Chat (Pokemon)

![Pokemon Chat](https://github.com/Cabrra/cabrra.github.io/raw/master/Images/networking/realtimechat.png)

This project implements the client module and sends/receives specific message data with the server. This also handles closing down the established communications at networking termination. The server module implements multiplexing logic designed around the management of multiple client I/O. Reinforcement of the sending and receiving as responds to the messages sent by your client and send back the messages received by the client.

#### 3. Meatball Tennis (Pong)

![Pong](https://github.com/Cabrra/cabrra.github.io/raw/master/Images/networking/meatball.png)

Client/server “pong” application using UDP datagram sockets. The server accepts connection requests from up to two clients. When a client connects to the server, it requests to either be the server-side client (on the listen server) or the standalone client. This dictates whether the client is Player 1 (left) or Player 2 (right). Clients send input messages to the server, and the server sends a snapshot every game tick.

As part of this assignment students must determine how to use UDP and conserve bandwidth. The exact method of doing so in code is left up to the student.

![Pong game](https://github.com/Cabrra/cabrra.github.io/raw/master/Images/networking/meatball2.png)

+ BUGS:
	+ 1) If you start and stop a 3rd running client, the two other clients will crash.
	+ 2) If you close the server the client window for player two does not close in all instances.

## Built With

* [Visual Studio](https://visualstudio.microsoft.com/) 					- For C++ development

## Contributing

Please read [CONTRIBUTING.md](https://github.com/Cabrra/Contributing-template/blob/master/Contributing-template.md) for details on the code of conduct, and the process for submitting pull requests to me.

## Authors

* **Tim Turcich** - Base code
* **Jagoba "Jake" Marcos** - [Cabrra](https://github.com/Cabrra)

## License

This project is licensed under the MIT license - see the [LICENSE](LICENSE) file for details

## Acknowledgments

* Full Sail University - Game Development Department
* Darryl Malcomb - Course director
