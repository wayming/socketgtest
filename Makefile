# Makefile for gtest examples

GOOGLE_TEST_LIB = gtest
GOOGLE_TEST_ROOT = ./googletest-release-1.10.0/install

G++ = g++
G++_FLAGS = -c -g -O0 -Wall -I $(GOOGLE_TEST_ROOT)/include
LD_FLAGS = -L $(GOOGLE_TEST_ROOT)/lib64 -l $(GOOGLE_TEST_LIB) -l pthread

OBJECTS = main.o socket_test.o
TARGET = socket_test

all: $(TARGET)

$(TARGET): $(OBJECTS)
	g++ -o $(TARGET) $(OBJECTS) $(LD_FLAGS)

%.o : %.cpp
	$(G++) $(G++_FLAGS) $<

clean:
	rm -f $(TARGET) $(OBJECTS)
                    
.PHONY: all clean
