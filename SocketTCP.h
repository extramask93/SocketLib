#pragma once
#include <WinSock2.h>
#include <string>

class IPAddress;

class SocketTCP
{
public:
	enum class State {
		Done, NotReady, Partial, Disconnected, Error
	};
	SocketTCP();
	SocketTCP::State TCPConnect(const std::string& remoteAddress, unsigned short remotePort, int timeout = 0);
	SocketTCP::State TCPSend(const char* data, int size) const;
	SocketTCP::State TCPSendString(const std::string& s) const;
	SocketTCP::State TCPReceive(void *data, int size, int &received) const;
	SocketTCP::State TCPReceiveChar(char *c) const;
	void ReadAll(std::string& s) const;
	size_t read(char * buffer, int size) const;
	bool isReadyToRead() const;
	size_t TCPReceiveUntil(std::string& line, const std::string& end = "\r\n") const;
	SocketTCP(SocketTCP &socket) = delete;
	SocketTCP& operator=(SocketTCP& sock) = delete;
	~SocketTCP();
private:
	SOCKET sock = INVALID_SOCKET;
	WSADATA wsa {};
	State state = State::Disconnected;
};