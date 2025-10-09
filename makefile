CXX := g++
CXXFLAGS := -O2 -std=c++17 -Wall -Wextra -pedantic

all: user1 user2

user1: user1.cpp
	$(CXX) $(CXXFLAGS) -o user1 user1.cpp

user2: user2.cpp
	$(CXX) $(CXXFLAGS) -o user2 user2.cpp

clean:
	rm -f user1 user2
