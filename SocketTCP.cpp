#include "SocketException.h"
#include "SocketTCP.h"
#include <string>
#ifdef _WIN32
#define CLOSE(sock) closesocket(sock)
#include <ws2tcpip.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#define CLOSE(sock) close(sock)
#endif


#define CONNECT_REQUIRED {if(sock == INVALID_SOCKET) {throw SocketException{"Socket not connected, please connect before use."};}}

SocketTCP::SocketTCP(SocketTCP::Mode mode) : mode(mode)
{
#ifdef _WIN32
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
        throw SocketException(std::string("getaddrinfo: ") + gai_strerror(errorCode));
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
		throw SocketException("Cannot establish connection to the server, error code: ");
	}
	::freeaddrinfo(result);
    return State::Done;
}
SocketTCP::State SocketTCP::TCPListen(const std::string &addr, std::uint16_t port,int queue_len) {
 	struct addrinfo hints {};
    struct addrinfo *result, *rp;
    int s;

    hints.ai_family = AF_INET;    /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_STREAM; /* Datagram socket */
    hints.ai_flags = 0;    /* For wildcard IP address */
    hints.ai_protocol = 0;          /* Any protocol */

    s = getaddrinfo(NULL, std::to_string(port).c_str(), &hints, &result);
    if (s != 0) {
	   throw SocketException(std::string("getaddrinfo: ") + gai_strerror(s));
    }

    for (rp = result; rp != NULL; rp = rp->ai_next) {
        sock = socket(rp->ai_family, rp->ai_socktype,rp->ai_protocol);
        if (sock == -1)
            continue;
        if (bind(sock, rp->ai_addr, rp->ai_addrlen) == 0)
            break;                  /* Success */
        close(sock);
    }
    if (rp == NULL) {               /* No address succeeded */
		   throw SocketException("Could not bind");
    }
    freeaddrinfo(result);           /* No longer needed */
	if(listen(sock, queue_len) == -1) {
		throw SocketException {"Listen failed"};
	}
	return State::Done;
}
std::unique_ptr<SocketTCP>	SocketTCP::TCPAccept() {
	struct sockaddr peer_addr;
    socklen_t peer_addr_size;
	SOCKET client = accept(sock,&peer_addr,&peer_addr_size);
	return std::unique_ptr<SocketTCP>(new SocketTCP{client,mode,state,timeout});
}
size_t SocketTCP::TCPReveiveN(std::string &s, size_t n)
{
    CONNECT_REQUIRED;
    char buffer;
    size_t bytesRead = 0;
    s.clear();
    while (TCPReceiveChar(&buffer) == State::Done && bytesRead < n) {
        s.push_back(buffer);
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
	if(!isReadyToRead()) {
		throw SocketException("Reception timed out");
	}
	const auto bytes = read(c, 1);
	if (bytes > 0)
	{
		return State::Done;
	}
	if(bytes == 0)
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
