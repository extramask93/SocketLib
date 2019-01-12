#pragma once
#include <WinSock2.h>
#include <string>
/*this library supports only blocking connections and windows only*/


class IPAddress;
class SocketTCP
{
public:
	enum class State {
		Done, NotReady, Disconnected
	};
	/*currently not supported TODO*/
	enum class Mode
	{
		Listen, Connect
	};
	/************************/
	SocketTCP();
	/*Blocking API*/
	SocketTCP::State TCPConnect(const std::string& remoteAddress, unsigned short remotePort, int timeout = 5);
	SocketTCP::State TCPSend(const char* data, int size) const;
	SocketTCP::State TCPSendString(const std::string& s) const;
	SocketTCP::State TCPReceive(void *data, int size, int &received) const;
	SocketTCP::State TCPReceiveChar(char *c) const;
	size_t			 TCPReceiveUntil(std::string& line, const std::string& end = "\r\n") const;
	/*Non blocking API*/
	/******TODO - no need for it currently*********/
	SocketTCP(SocketTCP &socket) = delete;
	SocketTCP& operator=(SocketTCP& sock) = delete;
	~SocketTCP();
private:
	int timeout = 5;
	SOCKET sock = INVALID_SOCKET;
	WSADATA wsa {};
	State state = State::Disconnected;
	size_t read(char * buffer, int size) const;
	bool isReadyToRead() const;
};