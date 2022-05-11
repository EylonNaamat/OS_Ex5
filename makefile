server.o: server.cpp
	g++ -c server.cpp

server: server.o
	g++ server.o -o server

client.o: client.cpp
	g++ -c client.cpp

client: client.o
	g++ client.o -o client

#tests.o: tests.cpp
#	g++ -c tests.c
#
#tests: tests.o
#	g++ tests.o -o tests
#
#stack.o: stack.cpp
#	g++ -c stack.c
#
#stack: stack.o
#	g++ stack.o -o stack

all: server client

.PHONY: clean

clean:
	rm *.o server client

