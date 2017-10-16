CXX := g++

ifeq ($(debug), y)
    CXXFLAGS := -g
else
    CXXFLAGS := -O2 -DNDEBUG
endif

CXXFLAGS := $(CXXFLAGS) -Wall -Werror -std=c++1z

LUADIR := $(HOME)/workspace/lua

INCLUDE := -I$(LUADIR)/src
LIBS := -L$(LUADIR)/src -llua -lm

OBJS := $(patsubst %.cpp, %.o, $(wildcard *.cpp))
TARGET := luacpp

.PHONY: all clean deps

all: deps $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LIBS)

deps:
	$(MAKE) posix -C $(LUADIR)

.cpp.o:
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $< -o $@

clean:
	rm -f $(TARGET) *.o
