CXX = g++

K ?= 8
R ?= 2
RANDOMS ?= 0 

CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -DK=$(K) -DR=$(R) -DRANDOMS=$(RANDOMS)

SRC = main.cpp greedy.cpp zfun.cpp
OBJ = $(SRC:.cpp=.o)
DEPS = zfun.h

TARGET = greedy

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^

%.o: %.cpp $(DEPS)
	$(CXX) $(CXXFLAGS) -c $<

clean:
	rm -f $(OBJ) $(TARGET)