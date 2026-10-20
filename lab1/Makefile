CXX      ?= g++
CXXFLAGS ?= -O2 -std=c++17 -Wall -Wextra -pedantic
TARGET    = tsp_solver

all: $(TARGET)

$(TARGET): src/main.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

run: $(TARGET)
	./run.sh

clean:
	rm -f $(TARGET)

.PHONY: all run clean
