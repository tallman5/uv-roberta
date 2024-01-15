CC = g++
CFLAGS = -Wall -Wextra
LIBS = -lpigpio

SRCDIR = src/rob
SRCS = $(wildcard $(SRCDIR)/*.cpp)
OBJS = $(SRCS:.cpp=.o)

TARGET = rob

all: $(TARGET)

$(TARGET): $(OBJS) src/rob/rob_main.cpp
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

%.o: %.cpp
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(TARGET)
