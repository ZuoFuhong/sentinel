CXX = g++
CXXFLAGS = -std=c++11 -Wall

SRCS = main.cc server.cc
OBJS = $(SRCS:.cc=.o)
EXEC = sentinel

all: $(EXEC)

$(EXEC): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(EXEC)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(EXEC)

.PHONY: all clean

