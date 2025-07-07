# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++17 -Wall -I./include -I./build
LDFLAGS =

# Project structure
SRCDIR = src
INCDIR = include
BLDDIR = build
TARGET = witnessc

# Source files
CPP_SOURCES = $(wildcard $(SRCDIR)/*.cpp)
GENERATED_SOURCES = $(BLDDIR)/parser.tab.cpp $(BLDDIR)/lexer.yy.cpp

# Object files
CPP_OBJECTS = $(patsubst $(SRCDIR)/%.cpp, $(BLDDIR)/%.o, $(CPP_SOURCES))
GENERATED_OBJECTS = $(patsubst $(BLDDIR)/%.cpp, $(BLDDIR)/%.o, $(GENERATED_SOURCES))
OBJECTS = $(CPP_OBJECTS) $(GENERATED_OBJECTS)

# Bison and Flex
BISON = bison
BISON_FLAGS = -d -o $(BLDDIR)/parser.tab.cpp
FLEX = flex
FLEX_FLAGS = -o $(BLDDIR)/lexer.yy.cpp

# Stamp file to ensure bison runs first
BISON_STAMP = $(BLDDIR)/bison.stamp

.PHONY: all clean

all: $(BLDDIR) $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $^ $(LDFLAGS)

# Make all objects depend on the bison stamp file
$(OBJECTS): $(BISON_STAMP)

$(BISON_STAMP): $(SRCDIR)/parser.ypp
	$(BISON) $(BISON_FLAGS) $(SRCDIR)/parser.ypp
	@touch $(BISON_STAMP)

$(BLDDIR)/%.o: $(SRCDIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(BLDDIR)/%.o: $(BLDDIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(BLDDIR)/lexer.yy.cpp: $(SRCDIR)/lexer.lpp $(BISON_STAMP)
	$(FLEX) $(FLEX_FLAGS) $<

$(BLDDIR):
	mkdir -p $(BLDDIR)

clean:
	rm -rf $(BLDDIR) $(TARGET) 