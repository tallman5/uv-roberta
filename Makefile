# Define the compiler and the compiler flags
CXX=g++
CXXFLAGS=-std=c++11 -Wall

# Define the target executable
TARGET=hello_pi

# Build target
all: $(TARGET)

# How to build the target
$(TARGET): main.cpp
	$(CXX) $(CXXFLAGS) -o $(TARGET) main.cpp

# Clean target
clean:
	rm -f $(TARGET)
