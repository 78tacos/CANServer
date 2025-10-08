CXX = g++
CXXFLAGS = -std=c++20 -Wall

all: server

server: server.cpp
	    $(CXX) $(CXXFLAGS) -o server server.cpp

clean:
	    rm -f server
