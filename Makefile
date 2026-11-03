CXX      ?= g++
CXXFLAGS ?= -O2 -std=c++17 -Wall -Wextra -pedantic
CPPFLAGS ?= -Iengine
TARGET    = tsp_solver
SRCS      = $(wildcard engine/*.cpp engine/*/*.cpp)
OBJS      = $(SRCS:.cpp=.o)
HEADERS   = $(wildcard engine/*.hpp engine/*/*.hpp)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS)

%.o: %.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

run: $(TARGET)
	./run.sh

clean:
	rm -f $(TARGET) $(OBJS)

.PHONY: all run clean
