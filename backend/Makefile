CXX = g++
CXXFLAGS = -std=c++17 -O3 -Wall -Wextra

ifeq ($(OS),Windows_NT)
    LDFLAGS = -lws2_32 -static
    TARGET = gcd_server.exe
else
    LDFLAGS = -pthread
    TARGET = gcd_server
endif

SRCS = main.cpp
HEADERS = httplib.h json.hpp gcd_solver.hpp

all: $(TARGET)

$(TARGET): $(SRCS) $(HEADERS)
	$(CXX) $(CXXFLAGS) -o $@ $(SRCS) $(LDFLAGS)

clean:
ifeq ($(OS),Windows_NT)
	del /f $(TARGET) 2>nul
else
	rm -f $(TARGET)
endif

.PHONY: all clean
