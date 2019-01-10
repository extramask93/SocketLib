#include "SocketException.h"
#include "SocketTCP.h"
#include <string>
#include <Ws2tcpip.h>
#include <sstream>
SocketTCP::SocketTCP()
{
	const auto err = WSAStartup(MAKEWORD(2, 2), &this->wsa);
	if(err != 0)
	{
		throw SocketException{"WSAStartup failed with code: "+ std::to_string(err)};
	}
}

SocketTCP::State SocketTCP::TCPConnect(const std::string& remoteAddress,
	unsigned short remotePort, int timeout)
{
	std::stringstream s;
	s << remotePort;
	struct addrinfo hints{};
	struct addrinfo *result, *resultPointer;

	hints.ai_family = PF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = 0;
	hints.ai_protocol = 0;

	const int errorCode =
		getaddrinfo(remoteAddress.c_str(), s.str().c_str(), &hints, &result);
	if (errorCode != 0)
	{
        throw SocketException(std::string("Error occurred during DNS resolving, system message: "));
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

		closesocket(sock);
	}

	if (resultPointer == nullptr)
	{
		throw SocketException("Cannot establish connection to the server, error code: "
			+ std::to_string(WSAGetLastError()));
	}
	::freeaddrinfo(result);
	return state;
}

SocketTCP::State SocketTCP::TCPSend(const char *data, int size) const
{
	int total = 0;
	int bytesleft = size; 

	while (total < size) {
		int n = send(sock, data + total, bytesleft, 0);
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

SocketTCP::State SocketTCP::TCPReceive(void * data, int size, int & received) const
{
	received = recv(sock, static_cast<char*>(data), size, 0);
	return State::Done;
}

SocketTCP::State SocketTCP::TCPReceiveChar(char* c) const
{
	if (read(c, 1) > 0)
	{
		return State::Done;
	}
	return State::NotReady;
}

void SocketTCP::ReadAll(std::string& s) const
{
	char buff;
	s.clear();
	while(TCPReceiveChar(&buff) == State::Done)
	{
		s += buff;
	}
}

size_t SocketTCP::read(char* buffer, int size) const
{
	int bytesRead = 0;
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
	timeout.tv_sec = 10;
	const int selectCode = select(static_cast<int>(sock + 1), &recieveFd, nullptr, nullptr, &timeout);
	return selectCode > 0;

}
size_t SocketTCP::TCPReceiveUntil(std::string & line, const std::string & end) const
{

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
	closesocket(sock);
	state = State::Disconnected;
}
