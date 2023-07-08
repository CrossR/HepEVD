#CXX = clang++
CXXFLAGS = -O2 -std=c++17 -I.. -Wall -Wextra -pthread

all: hits

hits : hits.cpp ../httplib.h Makefile
	$(CXX) -o hits $(CXXFLAGS) hits.cpp

clean:
	rm hits
