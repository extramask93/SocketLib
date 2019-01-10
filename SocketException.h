#pragma once
#include <stdexcept>
class SocketException : public std::runtime_error
{
public:
	SocketException(const char * message) : std::runtime_error(message) {}
	SocketException(const std::string& message) : std::runtime_error(message.c_str()) {}
};

