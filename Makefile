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
# Add explicit object files for parser.tab.cpp and lexer.yy.cpp
GENERATED_OBJECTS = $(BLDDIR)/parser.tab.o $(BLDDIR)/lexer.yy.o
OBJECTS = $(CPP_OBJECTS) $(GENERATED_OBJECTS)

# Bison and Flex
BISON = bison
BISON_FLAGS = -d -o $(BLDDIR)/parser.tab.cpp
FLEX = flex
FLEX_FLAGS = -o $(BLDDIR)/lexer.yy.cpp

# Stamp file to ensure bison runs first
BISON_STAMP = $(BLDDIR)/bison.stamp

.PHONY: all clean

# Main target
all: $(BLDDIR) $(TARGET)

# Link everything
$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

# Ensure all objects depend on Bison stamp (forces correct build order)
$(OBJECTS): $(BISON_STAMP)

# Run Bison
$(BISON_STAMP): $(SRCDIR)/parser.ypp
	$(BISON) $(BISON_FLAGS) $<
	@touch $@

# Compile regular C++ source files
$(BLDDIR)/%.o: $(SRCDIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

# Compile Flex-generated file
$(BLDDIR)/lexer.yy.cpp: $(SRCDIR)/lexer.lpp $(BISON_STAMP)
	$(FLEX) $(FLEX_FLAGS) $<

# Compile Flex-generated object
$(BLDDIR)/lexer.yy.o: $(BLDDIR)/lexer.yy.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

# Compile Bison-generated parser explicitly (because of the dot in the filename)
$(BLDDIR)/parser.tab.o: $(BLDDIR)/parser.tab.cpp $(BLDDIR)/parser.tab.hpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

# Create build directory if it doesn't exist
$(BLDDIR):
	mkdir -p $(BLDDIR)

# Clean up build artifacts
clean:
	rm -rf $(BLDDIR) $(TARGET)
