#include "SocketException.h"
#include "SocketTCP.h"
#include <string>
#ifdef _MSC_VER
#define CLOSE(sock) closesocket(sock)
#include <Ws2tcpip.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#define CLOSE(sock) close(sock)
#endif


#define CONNECT_REQUIRED {if(sock == INVALID_SOCKET) {throw SocketException{"Socket not connected, please connect before use."};}}

SocketTCP::SocketTCP()
{
#ifdef _MSC_VER
const auto err = WSAStartup(MAKEWORD(2, 2), &this->wsa);

if(err != 0)
{
    throw SocketException{"WSAStartup failed with code: "+ std::to_string(err)};
}
#endif
}

SocketTCP::State SocketTCP::TCPConnect(const std::string& remoteAddress,
	unsigned short remotePort, int timeout_)
{
	struct addrinfo hints{};
	struct addrinfo *result, *resultPointer;
	timeout = timeout_;
	hints.ai_family = PF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = 0;
	hints.ai_protocol = 0;

	const int errorCode =
		getaddrinfo(remoteAddress.c_str(), std::to_string(remotePort).c_str(), &hints, &result);
	if (errorCode != 0)
	{
        throw SocketException(std::string("Error occurred during DNS resolving, code: ") + std::to_string(errorCode));
	}

	for (resultPointer = result; resultPointer != nullptr; resultPointer = resultPointer->ai_next)
	{
		sock = socket(resultPointer->ai_family, resultPointer->ai_socktype, resultPointer->ai_protocol);
		if (sock == INVALID_SOCKET)
		{
			continue;
		}

		if (connect(sock, resultPointer->ai_addr, static_cast<int>(resultPointer->ai_addrlen)) != -1)
		{
			break;
		}

        CLOSE(sock);
	}

	if (resultPointer == nullptr)
	{
		throw SocketException("Cannot establish connection to the server, error code: "
            /*+ std::to_string(WSAGetLastError())*/);
	}
	::freeaddrinfo(result);
    return State::Done;
}

size_t SocketTCP::TCPReveiveUtilClosed(std::string &buff)
{
    CONNECT_REQUIRED;
    char buffer;
    size_t bytesRead = 0;
    buff.clear();
    while (TCPReceiveChar(&buffer) == State::Done) {
        buff.push_back(buffer);
        ++bytesRead;
    }
    return bytesRead;
}

SocketTCP::State SocketTCP::TCPSend(const char *data, int size) const
{
	CONNECT_REQUIRED;
	int total = 0;
	int bytesleft = size; 
    long n = 0;
	while (total < size) {
		n = send(sock, data + total, bytesleft, 0);
		if (n == -1) { throw SocketException{"Error while sending data to the remote host"}; }
		total += n;
		bytesleft -= n;
	}
	return State::Done;
}

SocketTCP::State SocketTCP::TCPSendString(const std::string& s) const
{
	return TCPSend(s.c_str(), static_cast<int>(s.length()));
}

SocketTCP::State SocketTCP::TCPReceive(void * data, int size, long & received) const
{
	CONNECT_REQUIRED;
	received = recv(sock, static_cast<char*>(data), size, 0);
	return State::Done;
}

SocketTCP::State SocketTCP::TCPReceiveChar(char* c) const
{
	CONNECT_REQUIRED;
	const auto bytes = read(c, 1);
	if (bytes > 0)
	{
		return State::Done;
	}
	if(bytes == 0 )
	{
		return State::Disconnected;
	}
	return State::NotReady;
}

size_t SocketTCP::read(char* buffer, int size) const
{
	CONNECT_REQUIRED;
    long bytesRead = 0;
	if (!isReadyToRead())
	{
		throw SocketException{ "Timed out reception" };
	}
	bytesRead = ::recv(sock, buffer, size,0);
	if (bytesRead < 0)
	{
		throw SocketException("Unable to read data from remote host");
	}

	return static_cast<size_t>(bytesRead);
}

bool SocketTCP::isReadyToRead() const
{
	fd_set recieveFd;
	struct timeval timeout {};
	FD_ZERO(&recieveFd);
	FD_SET(sock, &recieveFd);
	timeout.tv_sec = this->timeout;
	const int selectCode = select(static_cast<int>(sock + 1), &recieveFd, nullptr, nullptr, &timeout);
	return selectCode > 0;

}
size_t SocketTCP::TCPReceiveUntil(std::string & line, const std::string & end) const
{
	CONNECT_REQUIRED;
	char buffer;
	size_t bytesRead = 0;
	line.clear();
	while (TCPReceiveChar(&buffer) == State::Done) {
		line.push_back(buffer);
		++bytesRead;
		const auto res = line.rfind(end);
		if (res != std::string::npos)
		{
			line = line.substr(0, line.length() - end.length());
			break;
		}
	}
	return bytesRead;
}

SocketTCP::~SocketTCP()
{
	//no need for wsaclean, os will take care of it
    CLOSE(sock);
	sock = INVALID_SOCKET;
}
