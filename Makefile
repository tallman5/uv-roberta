# Define the compiler and the compiler flags
CXX=g++
CXXFLAGS=-std=c++11 -Wall
LDFLAGS=-lpigpio -pthread -lrt

# Define the directory for the source files
SRCDIR=src

# Define the target executable
TARGET=rob

# Find all .cpp files in the SRCDIR
SOURCES=$(shell find $(SRCDIR) -name '*.cpp')
# Define object files
OBJECTS=$(SOURCES:.cpp=.o)

# Build target
all: $(TARGET)

# Rule to make the executable
$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJECTS) $(LDFLAGS)

# Rule to make the object files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean target
clean:
	rm -f $(TARGET) $(OBJECTS)
