server.o: server.cpp
	g++ -c server.cpp

server: server.o
	g++ server.o -o server -lrt

client.o: client.cpp
	g++ -c client.cpp

client: client.o
	g++ client.o -o client

tests.o: tests.cpp
	g++ -c tests.cpp

tests: tests.o
	g++ tests.o -o tests

all: server client tests

.PHONY: clean

clean:
	rm *.o server client tests

