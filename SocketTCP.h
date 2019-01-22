#pragma once
#ifdef _WIN32
#include <winsock2.h>
#else
#define SOCKET int
#define INVALID_SOCKET -1
#endif
#include <string>
#include <memory>


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
	SocketTCP(SocketTCP::Mode mode = Mode::Connect);
	SocketTCP::State TCPConnect(const std::string& remoteAddress, unsigned short remotePort, int timeout = 5);
    size_t TCPReveiveUtilClosed(std::string &buffer);
	SocketTCP::State TCPSend(const char* data, int size) const;
	SocketTCP::State TCPSendString(const std::string& s) const;
    SocketTCP::State TCPReceive(void *data, int size, long &received) const;
	SocketTCP::State TCPReceiveChar(char *c) const;
	size_t			 TCPReceiveUntil(std::string& line, const std::string& end = "\r\n") const;
	SocketTCP::State TCPListen(const std::string &addr, std::uint16_t port, int queue_len = 5);
	std::unique_ptr<SocketTCP> TCPAccept();
	/*Non blocking API*/
	/******TODO - no need for it currently*********/
	SocketTCP(SocketTCP &socket) = delete;
	SocketTCP& operator=(SocketTCP& sock) = delete;
	~SocketTCP();
private:
	SocketTCP(SOCKET sckfd, Mode mode_, State state_, int timeout_) :timeout(timeout_), sock(sckfd),state(state_),mode(mode_) {}
#ifdef _WIN32
	WSADATA wsa {};
#endif
	int timeout = 5;
    SOCKET sock = INVALID_SOCKET;
	State state = State::Disconnected;
	Mode mode = Mode::Connect;
	size_t read(char * buffer, int size) const;
	bool isReadyToRead() const;
};
