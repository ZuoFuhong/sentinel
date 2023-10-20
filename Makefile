CXX = g++
CXXFLAGS = -std=c++11 -O2 -Wall

SRCS = main.cc server.cc
TARGET = sentinel

all: $(SRCS)
	$(CXX) $(CXXFLAGS) $(SRCS) -o $(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: all clean

