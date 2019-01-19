#pragma once
#ifdef _WIN32
#include <winsock2.h>
#else
#define SOCKET int
#define INVALID_SOCKET -1
#endif
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
    size_t TCPReveiveUtilClosed(std::string &buffer);
	SocketTCP::State TCPSend(const char* data, int size) const;
	SocketTCP::State TCPSendString(const std::string& s) const;
    SocketTCP::State TCPReceive(void *data, int size, long &received) const;
	SocketTCP::State TCPReceiveChar(char *c) const;
	size_t			 TCPReceiveUntil(std::string& line, const std::string& end = "\r\n") const;
	/*Non blocking API*/
	/******TODO - no need for it currently*********/
	SocketTCP(SocketTCP &socket) = delete;
	SocketTCP& operator=(SocketTCP& sock) = delete;
	~SocketTCP();
private:
	int timeout = 5;
#ifdef _WIN32
	WSADATA wsa {};
#endif
    SOCKET sock = INVALID_SOCKET;
	State state = State::Disconnected;
	size_t read(char * buffer, int size) const;
	bool isReadyToRead() const;
};
