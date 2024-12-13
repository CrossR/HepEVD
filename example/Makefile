CXXFLAGS = -O3 -std=c++17 -I.. -Wall -Wextra -Wshadow -Werror -pthread

all: basic server client debugging

basic : basic.cpp Makefile
	$(CXX) -o basic $(CXXFLAGS) basic.cpp

server : server.cpp Makefile
	$(CXX) -o server $(CXXFLAGS) server.cpp

client : client.cpp Makefile
	$(CXX) -o client $(CXXFLAGS) client.cpp

debugging: debugging.cpp Makefile
	$(CXX) -o debugging $(CXXFLAGS) debugging.cpp

clean:
	rm -f basic server client debugging
