CXXFLAGS = -O2 -std=c++17 -I.. -Wall -Wextra -pthread

all: basic

basic : basic.cpp ../httplib.h Makefile
	$(CXX) -o basic $(CXXFLAGS) basic.cpp

clean:
	rm basic
