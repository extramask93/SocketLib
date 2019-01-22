#include "SocketTCP.h"
#include "SocketException.h"
#include <iostream>


int main() {
    try {
        auto sock = SocketTCP{SocketTCP::Mode::Listen};
        sock.TCPListen("0.0.0.0",2000);
        auto client = sock.TCPAccept();
        client->TCPSendString("Welcome");
    }
    catch(SocketException &ee) {
        std::cout<<ee.what();
    }
    return 0;
}