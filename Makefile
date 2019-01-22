all: SocketTCP.o main.o
	g++ -Wall -std=c++17 SocketTCP.o main.o -o a.out
main.o:
	g++ -Wall -c -std=c++17 main.cpp -o main.o
SocketTCP.o:
	g++ -Wall -c -std=c++17 SocketTCP.cpp -o SocketTCP.o
clean:
	rm *.o a.out